//
// Created by Admin on 20/04/2026.
//

#pragma once
#include <vector>

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_scancode.h"

namespace horizon {

class InputManager {

    public:
        auto init() -> void;

        auto is_key_down(SDL_Scancode scancode) -> bool;

        auto key_just_pressed(SDL_Scancode scancode) -> bool;

        auto key_just_released(SDL_Scancode scancode) -> bool;

        auto was_key_pressed_last_frame(SDL_Scancode scancode) -> bool;

        auto flush_key_state() -> void;
    private:
        const bool*       m_current_key_state = nullptr;
        std::vector<uint8_t> m_last_key_state    = {};

};

} // horizon
