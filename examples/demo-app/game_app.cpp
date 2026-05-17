//
// Created by Admin on 06/04/2026.
//

#include "game_app.hpp"

#include "imgui.h"
#include "substratum/log.hpp"
#include "thresh/engine.hpp"
#include "thresh/scene/ecs_types.hpp"

namespace demo {

    auto GameApp::startup() -> void {

        SUB_INFO("Starting Demo Game");
        auto scene = std::make_unique<thresh::Scene>();

        auto player = scene->get_or_create_entity("Player")
        .set<Transform>({.position = {0.0f, 1.0f, 5.0f}})
        .set<Camera>({})
        .set<CameraController>({})
        .add<ActiveCamera>();

        auto box_mesh = thresh::Engine::get_instance().graphics()->register_mesh(flux::primitives::box());
        auto box_texture = thresh::Engine::get_instance().graphics()->register_texture("textures/checker.png");
        auto box_material = thresh::Engine::get_instance().graphics()->register_material(box_texture);
        auto red_material = thresh::Engine::get_instance().graphics()->register_material(box_texture, {1.0f, 0.0f, 0.0f, 1.0f});

        scene->get_or_create_entity("Test Cube").set<Transform>({.position = {0.0f, 0.0f, 0.0f}}).set<Mesh>({.handle = box_mesh, .material_handle = box_material});
        scene->get_or_create_entity("Test Cube 2").set<Transform>({.position = {2.0f, 0.0f, 0.0f}}).set<Mesh>({.handle = box_mesh, .material_handle = red_material});

        thresh::Engine::get_instance().transition_scene(std::move(scene));
    }

    auto GameApp::update(float /*delta_time*/) -> void {

        auto* input = thresh::Engine::get_instance().input();
        if (input->key_just_released(SDL_SCANCODE_ESCAPE)) {
            thresh::Engine::get_instance().window()->setShouldClose(true);
        }
        if (input->key_just_pressed(SDL_SCANCODE_F1)) {
            auto imgui_enabled = thresh::Engine::get_instance().graphics()->is_imgui_enabled();
            thresh::Engine::get_instance().graphics()->imgui_enabled(!imgui_enabled);
            auto scene = thresh::Engine::get_instance().active_scene();
            auto player = scene->get_or_create_entity("Player");
            player.get_mut<CameraController>().movement_allowed = imgui_enabled;
        }
    }

    auto GameApp::render() -> void {

        // ImGui::ShowDemoWindow();
    }

    auto GameApp::debug_render() -> void
    {
        ImGui::ShowDemoWindow();
    }

    auto GameApp::shutdown() -> void {
        SUB_INFO("Shutting Down Demo Game, Flushing to disk, etc.");
    }

} // demo
