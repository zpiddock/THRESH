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

        auto assets = thresh::Engine::get_instance().assets();

        auto box_mesh = thresh::Engine::get_instance().graphics()->register_mesh(flux::primitives::box());

        auto brick_material = assets.load_material("material/brick.mat");
        auto default_material = assets.load_material("material/default.mat");
        auto red_checker = assets.load_material("material/red_checker.mat");

        scene->get_or_create_entity("Test Cube").set<Transform>({.position = {0.0f, 0.0f, 0.0f}}).set<Mesh>({.handle = box_mesh, .material_handle = default_material});
        scene->get_or_create_entity("Test Cube 2").set<Transform>({.position = {2.0f, 0.0f, 0.0f}}).set<Mesh>({.handle = box_mesh, .material_handle = red_checker});
        scene->get_or_create_entity("Test Cube 3").set<Transform>({.position = {-2.0f, 0.0f, 0.0f}}).set<Mesh>({.handle = box_mesh, .material_handle = brick_material});

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
    }

    auto GameApp::debug_render() -> void
    {
        ImGui::ShowDemoWindow();
    }

    auto GameApp::shutdown() -> void {
        SUB_INFO("Shutting Down Demo Game, Flushing to disk, etc.");
    }

} // demo
