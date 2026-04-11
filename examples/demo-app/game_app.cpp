//
// Created by Admin on 06/04/2026.
//

#include "game_app.hpp"

#include "substratum/log.hpp"

namespace demo {
    auto GameApp::update(float delta_time) -> void {

    }

    void GameApp::shutdown() {

        SUB_INFO("Shutting Down Demo Game, Flushing to disk, etc.");
    }
} // demo