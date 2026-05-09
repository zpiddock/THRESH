//
// Created by Admin on 06/04/2026.
//

#include "game_app.hpp"

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

        thresh::Engine::get_instance().transition_scene(std::move(scene));
    }

    auto GameApp::update(float /*delta_time*/) -> void {

        auto* input = thresh::Engine::get_instance().input();
        if (input->key_just_released(SDL_SCANCODE_ESCAPE)) {
            thresh::Engine::get_instance().window()->setShouldClose(true);
        }
    }

    auto GameApp::render() -> void {
    }

    auto GameApp::shutdown() -> void {
        SUB_INFO("Shutting Down Demo Game, Flushing to disk, etc.");
    }

} // demo
