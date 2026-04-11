//
// Created by Admin on 05/04/2026.
//


#include "game_app.hpp"
#include "substratum/log.hpp"
#include "thresh/engine.hpp"

int main() {

    substratum::Logger::set_level(substratum::LogLevel::Trace);

    SUB_INFO("Demo Game Starting");

    auto& engine = thresh::Engine::create(new demo::GameApp());

    if (engine.init({.title = "TRESH DEMO", .width = 1280, .height = 720})) {

        SUB_INFO("Engine Initialized");
        engine.run();
        SUB_INFO("Demo Game Ending");
    } else {
        SUB_FATAL("Engine could not initialize!");
    }
    return 0;
}
