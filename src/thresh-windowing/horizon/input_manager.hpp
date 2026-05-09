//
// Created by Admin on 20/04/2026.
//

#pragma once
#include <vector>

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_scancode.h"

namespace horizon {

class InputManager {

    public:
        auto init() -> void;

        auto is_key_held(SDL_Scancode scancode) -> bool;

        auto key_just_pressed(SDL_Scancode scancode) -> bool;

        auto key_just_released(SDL_Scancode scancode) -> bool;

        auto was_key_pressed_last_frame(SDL_Scancode scancode) -> bool;

        auto is_mouse_button_held(uint32_t button) -> bool;

        auto mouse_button_just_pressed(uint32_t button) -> bool;

        auto mouse_button_just_released(uint32_t button) -> bool;

        auto get_mouse_state() -> std::pair<float, float>;

        auto get_relative_mouse_state() -> std::pair<float, float>;

        auto flush_key_state() -> void;
    private:
        const bool*       m_current_key_state = nullptr;
        std::vector<uint8_t> m_last_key_state    = {};

        uint32_t m_current_mouse_state = 0;
        uint32_t m_last_mouse_state = 0;

        float m_mouse_dx = 0.0f;
        float m_mouse_dy = 0.0f;
};

} // horizon
