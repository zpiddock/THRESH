//
// Created by Admin on 06/04/2026.
//

#include "game_app.hpp"

#include "thresh/thresh.hpp"

#include "imgui.h"
#include "substratum/filesystem/vfs.hpp"
#include "substratum/log.hpp"
#include "thresh/engine.hpp"
#include "thresh/scene/ecs_types.hpp"
#include "thresh/scene/scene_serializer.hpp"


#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"

namespace demo {

    auto GameApp::startup() -> void {

        SUB_INFO("Starting Demo Game");
        const auto assets_path = (std::filesystem::current_path() / "test_assets").string();
        substratum::VFS::mount(assets_path, "/test");
        SUB_INFO(assets_path);
        substratum::VFS::set_write_dir(assets_path);

        thresh::Engine::get_instance().graphics()->enable_debug_line_renderer();
        m_debug_ui = std::make_unique<thresh::DebugUI>();

        auto scene = thresh::Engine::get_instance().load_scene("/test/scenes/scene.thresh");

        if (scene) {

            auto box_mesh = thresh::Engine::get_instance().graphics()->resources().register_mesh(flux::primitives::box());
            auto default_material = thresh::Engine::get_instance().assets().load_material("material/default.mat");

            // Manually creates a child of "Test Cube" with propagated world transform, 2 units above world transform of parent
            [[maybe_unused]] auto child = scene->get_or_create_entity("ChildBox")
                .child_of(scene->get_or_create_entity("Test Cube"))
                .set<Transform>({.position = {0,2,0}, .scale = helix::float3{0.5f}})
                .set<MeshSource>({.path = "primitive://box"})
                .set<MaterialSource>({.path = "material/default.mat"})
                .set<Mesh>({box_mesh, default_material});

           // thresh::Engine::get_instance().models().spawn(scene->world(), "models/ArmoredGirl.tasset", scene->root());

        }

        thresh::Engine::get_instance().transition_scene(std::move(scene));

        auto& physics = thresh::Engine::get_instance().active_scene()->physics();
            auto& bodies  = physics.bodies();

            const JPH::BodyCreationSettings sphere(
                new JPH::SphereShape(0.5f), JPH::RVec3(0, 10, 0), JPH::Quat::sIdentity(),
                JPH::EMotionType::Dynamic, thresh::phys::layers::MOVING);
            const JPH::BodyID id = bodies.CreateAndAddBody(sphere, JPH::EActivation::Activate);

            for (int i = 0; i < 60; ++i) physics.step(1.f / 60.f);
            SUB_INFO("smoke: sphere y after 1s = {}", bodies.GetPosition(id).GetY()); // expect ~5.1 (10 - g/2)
            bodies.RemoveBody(id);
            bodies.DestroyBody(id);
    }

    auto GameApp::update(float /*delta_time*/) -> void {
        auto imgui_enabled = thresh::Engine::get_instance().graphics()->is_imgui_enabled();

        auto* input = thresh::Engine::get_instance().input();
        if (input->key_just_released(SDL_SCANCODE_ESCAPE)) {
            thresh::Engine::get_instance().window()->setShouldClose(true);
        }
        if (input->key_just_pressed(SDL_SCANCODE_F1)) {
            thresh::Engine::get_instance().graphics()->imgui_enabled(!imgui_enabled);
            auto scene = thresh::Engine::get_instance().active_scene();
            auto player = scene->get_or_create_entity("Player");
            player.get_mut<CameraController>().movement_allowed = imgui_enabled;
            thresh::Engine::get_instance().window()->set_relative_mouse_mode(imgui_enabled);
        }
        if (input->key_just_released(SDL_SCANCODE_F5)) {
            if (auto* scene = thresh::Engine::get_instance().active_scene()) {
                thresh::SceneSerializer::save_scene(*scene, "/scenes/scene.thresh");
            }
        }

        if (imgui_enabled && !ImGui::GetIO().WantCaptureMouse && input->mouse_button_just_pressed(SDL_BUTTON_LEFT) && (!ImGuizmo::IsOver() || ImGuizmo::IsUsing())) {
            auto* scene = thresh::Engine::get_instance().active_scene();
            auto* window = thresh::Engine::get_instance().window();
            if (scene && window) {
                const auto [mx, my] = input->get_mouse_state();
                int w = 0, h = 0;
                window->get_frame_buffer_size(w, h);
                if (auto hit = scene->pick_entity(static_cast<int>(mx), static_cast<int>(my), w, h)) {
                    if (m_debug_ui) {
                        m_debug_ui->set_selected_entity(hit->entity, hit->submesh);
                    }
                } else {
                    if (m_debug_ui) {
                        m_debug_ui->set_selected_entity({});
                    }
                }
            }
        }
    }

    auto GameApp::render() -> void {
    }

    auto GameApp::debug_render() -> void {

        if (auto* scene = thresh::Engine::get_instance().active_scene()) {
            m_debug_ui->draw(*scene);
        }
    }

    auto GameApp::shutdown() -> void {
        SUB_INFO("Shutting Down Demo Game, Flushing to disk, etc.");
    }

} // demo
