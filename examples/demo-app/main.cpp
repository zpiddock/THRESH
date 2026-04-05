//
// Created by Admin on 05/04/2026.
//

#include <SDL3/SDL.h>

#include "horizon/window.hpp"
#include "substratum/log.hpp"

int main() {

    SUB_INFO("Demo Game Starting");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SUB_FATAL("SDL could not initialize! SDL_Error: {}", SDL_GetError());
    }

    auto window = thresh::Window("TRHESH DEMO", 1280, 720, 0);

    if (window.getWindow() == nullptr) {

        SUB_FATAL("Window could not be created! SDL_Error: {}", SDL_GetError());
    }

    while (!window.shouldClose()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    window.setShouldClose(true);
                    break;
                }
                default:
                    break;
            }
        }
    }

    SDL_Quit();

    SUB_INFO("Demo Game Ending");
    return 0;
}
