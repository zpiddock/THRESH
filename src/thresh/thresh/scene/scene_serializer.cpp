//
// Created by shad0w on 20/05/2026.
//

#include "scene_serializer.hpp"

#include "glaze/glaze.hpp"

#include "ecs_types.hpp"
#include "substratum/filesystem/vfs.hpp"
#include "substratum/log.hpp"
#include "thresh/physics/physics_components.hpp"

namespace thresh {
    auto SceneSerializer::register_components(flecs::world& world) -> void {
        world.component<std::string>().opaque(flecs::String)
             .serialize([](const flecs::serializer* s, const std::string* data) {
                 const char* str = data->c_str();
                 return s->value(flecs::String, &str);
             })
             .assign_string([](std::string* data, const char* value) {
                 *data = value ? value : "";
             });

        register_math_components(world);

        world.component<Transform>()
            .member<helix::float3>("position")
            .member<helix::quat>("rotation")
            .member<helix::float3>("scale")
            .add(flecs::OnInstantiate, flecs::Override);

        world.component<Camera>()
             .member<float>("fov")
             .member<float>("near_plane")
             .member<float>("far_plane");

        world.component<CameraController>()
             .member<float>("yaw")
             .member<float>("pitch")
             .member<float>("movement_speed")
             .member<float>("mouse_sensitivity")
             .member<bool>("movement_allowed");

        world.component<AmbientLight>()
             .member<helix::float3>("colour")
             .member<float>("intensity");

        world.component<Light>()
             .member<helix::float3>("colour")
             .member<float>("intensity");

        world.component<MeshSource>()
             .member<std::string>("path");

        world.component<MaterialSource>()
             .member<std::string>("path");

        // Mesh is runtime-derived. Registered so flecs to_json/from_json handles
        // it cleanly values are overwritten on load by the resolution pass.
        world.component<Mesh>();          // Inherit trait dropped — nothing instantiates prefabs anymore

        world.component<MeshRenderer>();  // runtime-derived, no member reflection

        // ActiveCamera is a pure tag - registering the component is enough.
        world.component<ActiveCamera>();

        world.component<SceneRoot>();

        world.component<WorldTransform>();

        world.component<WorldAABB>();

        world.component<MotionType>();

        world.component<RigidBody>()
            .member<MotionType>("motion")
            .member<float>("mass")
            .member<float>("friction")
            .member<float>("restitution");

        world.component<BoxCollider>()
            .member<helix::float3>("half_extents");

        world.component<SphereCollider>()
            .member<float>("radius");

        world.component<CapsuleCollider>()
            .member<float>("radius")
            .member<float>("half_height");

        world.component<CharacterController>()
            .member<float>("height")
            .member<float>("radius")
            .member<float>("eye_height")
            .member<float>("move_speed")
            .member<float>("jump_speed")
            .member<bool>("freecam");

        world.component<PhysicsBody>();
    }

    auto SceneSerializer::save_scene(Scene& scene, const std::string& vfs_path)
        -> bool {

        const flecs::entity_to_json_desc_t desc = ECS_ENTITY_TO_JSON_INIT;

        glz::generic doc{};
        if (glz::read_json(doc, std::string{R"({"results":[]})"})) {
            SUB_ERROR("save_scene: failed to seed results document");
            return false;
        }
        auto& results = doc["results"].get_array();

        // Depth first from the root so entities at any depth are captured and
        // parents always precede their children in the file.
        bool ok = true;
        auto append = [&](this auto& self, flecs::entity e) -> void {
            const auto raw = e.to_json(&desc);
            if (!raw.c_str()) {
                SUB_ERROR("save_scene: entity_to_json failed for '{}'", e.path().c_str());
                ok = false;
                return;
            }
            glz::generic entry{};
            if (glz::read_json(entry, std::string{raw.c_str()})) {
                SUB_ERROR("save_scene: failed to parse json for '{}'", e.path().c_str());
                ok = false;
                return;
            }
            results.emplace_back(std::move(entry));
            e.children([&](flecs::entity child) { self(child); });
        };
        append(scene.root());

        if (!ok) {
            return false;
        }

        std::string merged;
        if (glz::write_json(doc, merged)) {
            SUB_ERROR("save_scene: failed to write merged scene json");
            return false;
        }

        const auto pretty_json = glz::prettify_json(merged);
        if (!substratum::VFS::write_file_string(vfs_path, pretty_json)) {
            SUB_ERROR("save_scene: failed to write '{}'", vfs_path);
            return false;
        }
        return true;
    }

    auto SceneSerializer::load_scene(AssetsLoader&      loader,
                                     const std::string& vfs_path)
        -> std::unique_ptr<Scene> {
        auto scene = std::make_unique<Scene>();

        auto& world = scene->world();

        const auto scene_json = substratum::VFS::read_file_string(vfs_path);
        if (scene_json.empty()) {
            SUB_ERROR("SceneSerializer::load_scene: file '{}' is empty or missing", vfs_path);
            return nullptr;
        }

        const auto* tail = world.from_json(scene_json.c_str());
        if (!tail) {
            SUB_ERROR("SceneSerializer::load_scene: failed to parse JSON file '{}'", vfs_path);
            return nullptr;
        }

        // Mesh is default constructed and would produce garbage data (UB) if not handled here
        world.each([&](flecs::entity e, const MeshSource& mesh, const MaterialSource& material) {
            const auto mesh_handle     = loader.resolve_mesh(mesh.path);
            const auto material_handle = loader.load_material(material.path);
            e.set<Mesh>({mesh_handle, material_handle});
            SUB_TRACE("Resolved mesh for '{}': mesh='{}'({}), mat='{}'({})",
                      e.name().c_str(), mesh.path, mesh_handle, material_handle, material.path);
        });

        // PhysicsBody & CharacterBody are default constructed and would produce garbage data (UB) is not handled here
        world.remove_all<PhysicsBody>();
        world.remove_all<CharacterBody>();

        SUB_INFO("Loaded scene from '{}'", vfs_path);
        return scene;
    }

    auto SceneSerializer::register_math_components(flecs::world& world)
        -> void {
        world.component<helix::float2>()
             .member<float>("x")
             .member<float>("y");

        world.component<helix::float3>()
             .member<float>("x")
             .member<float>("y")
             .member<float>("z");

        world.component<helix::float4>()
             .member<float>("x")
             .member<float>("y")
             .member<float>("z")
             .member<float>("w");

        world.component<helix::quat>()
             .member<float>("x")
             .member<float>("y")
             .member<float>("z")
             .member<float>("w");
    }
} // thresh
