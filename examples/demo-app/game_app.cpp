//
// Created by Admin on 06/04/2026.
//

#include "game_app.hpp"

#include "substratum/log.hpp"
#include "thresh/engine.hpp"

namespace demo {

    auto GameApp::startup() -> void {
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
