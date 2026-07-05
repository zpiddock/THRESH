//
// Created by Admin on 05/07/2026.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <filesystem>

#include "substratum/filesystem/vfs.hpp"
#include "thresh/physics/physics_components.hpp"
#include "thresh/scene/ecs_types.hpp"
#include "thresh/scene/scene.hpp"
#include "thresh/scene/scene_serializer.hpp"

namespace {

    // Scratch-dir VFS so save_scene has somewhere to write. Mounted read-side as
    // "/tests", write dir points at the same folder.
    struct VfsFixture {
        VfsFixture() {
            substratum::VFS::init(nullptr);
            const auto dir = (std::filesystem::temp_directory_path() / "thresh-tests").string();
            std::filesystem::create_directories(dir);
            substratum::VFS::mount(dir, "/tests");
            substratum::VFS::set_write_dir(dir);
        }
        ~VfsFixture() { substratum::VFS::shutdown(); }
    };
}

TEST_CASE("save_scene captures nested children and values survive a reload") {
    VfsFixture vfs;

    thresh::Scene scene;
    auto parent = scene.get_or_create_entity("Parent");
    parent.set<Transform>({.position = {1.f, 2.f, 3.f}});

    auto child = scene.world().entity("Child").child_of(parent);
    child.set<Transform>({.position = {0.f, 4.f, 0.f}});

    auto grandchild = scene.world().entity("GrandChild").child_of(child);
    grandchild.set<Transform>({.position = {0.f, 0.f, 5.f}, .scale = helix::float3{0.5f}});
    grandchild.set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 10.f});

    REQUIRE(thresh::SceneSerializer::save_scene(scene, "/roundtrip.thresh"));

    const auto json = substratum::VFS::read_file_string("/tests/roundtrip.thresh");
    REQUIRE_FALSE(json.empty());

    // Reload into a fresh world. These entities carry no MeshSource/MaterialSource,
    // so from_json is exactly load_scene minus the asset-resolution pass (which
    // needs a live renderer).
    thresh::Scene reloaded;
    REQUIRE(reloaded.world().from_json(json.c_str()) != nullptr);

    const auto found = reloaded.world().lookup("SceneRoot::Parent::Child::GrandChild");
    REQUIRE(found.is_valid());
    REQUIRE(found.has<Transform>());

    const auto& transform = found.get<Transform>();
    CHECK(transform.position.z == doctest::Approx(5.f));
    CHECK(transform.scale.x == doctest::Approx(0.5f));

    REQUIRE(found.has<RigidBody>());
    const auto& body = found.get<RigidBody>();
    CHECK(body.motion_type == MotionType::Dynamic);
    CHECK(body.mass == doctest::Approx(10.f));

    // Intermediate levels intact too, with their own values.
    const auto mid = reloaded.world().lookup("SceneRoot::Parent::Child");
    REQUIRE(mid.is_valid());
    CHECK(mid.get<Transform>().position.y == doctest::Approx(4.f));
}
