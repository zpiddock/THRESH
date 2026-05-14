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

        auto player = scene->create_entity()
        .set<Transform>({.position = {0.0f, 1.0f, 5.0f}})
        .set<Camera>({})
        .set<CameraController>({})
        .add<ActiveCamera>();

        auto box_mesh = thresh::Engine::get_instance().graphics()->register_mesh(flux::primitives::box());
        auto box_texture = thresh::Engine::get_instance().graphics()->register_texture("textures/checker.png");
        auto box_material = thresh::Engine::get_instance().graphics()->register_material(box_texture);
        auto red_material = thresh::Engine::get_instance().graphics()->register_material(box_texture, {1.0f, 0.0f, 0.0f, 1.0f});

        scene->create_entity("Test Cube").set<Transform>({.position = {0.0f, 0.0f, 0.0f}}).set<Mesh>({.handle = box_mesh, .material_handle = box_material});
        scene->create_entity("Test Cube 2").set<Transform>({.position = {2.0f, 0.0f, 0.0f}}).set<Mesh>({.handle = box_mesh, .material_handle = red_material});

        thresh::Engine::get_instance().transition_scene(std::move(scene));
    }

    auto GameApp::update(float /*delta_time*/) -> void {

        auto* input = thresh::Engine::get_instance().input();
        if (input->key_just_released(SDL_SCANCODE_ESCAPE)) {
            thresh::Engine::get_instance().window()->setShouldClose(true);
        }
    }

    auto GameApp::render() -> void {

        ImGui::ShowDemoWindow();
    }

    auto GameApp::shutdown() -> void {
        SUB_INFO("Shutting Down Demo Game, Flushing to disk, etc.");
    }

} // demo
