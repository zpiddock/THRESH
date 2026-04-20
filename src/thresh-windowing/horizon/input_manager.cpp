//
// Created by Admin on 20/04/2026.
//

#include "input_manager.hpp"

#include <vector>
#include <cstring>

namespace horizon {
    auto InputManager::init() -> void {
        int keycount = 0;
        m_current_key_state = SDL_GetKeyboardState(&keycount);
        m_last_key_state.assign(keycount, false);
    }

    auto InputManager::is_key_held(const SDL_Scancode scancode) -> bool {

        return m_current_key_state[scancode];
    }

    auto InputManager::key_just_pressed(SDL_Scancode scancode) -> bool {

        return m_current_key_state[scancode] && !m_last_key_state[scancode];
    }

    auto InputManager::key_just_released(SDL_Scancode scancode) -> bool {

        return !m_current_key_state[scancode] && m_last_key_state[scancode];
    }

    auto InputManager::was_key_pressed_last_frame(const SDL_Scancode scancode) -> bool {

        return m_last_key_state[scancode];
    }

    auto InputManager::is_mouse_button_held(uint32_t button) -> bool {

        return m_current_mouse_state & SDL_BUTTON_MASK(button);
    }

    auto InputManager::mouse_button_just_pressed(uint32_t button) -> bool {

        return (m_current_mouse_state & SDL_BUTTON_MASK(button)) && !(m_last_mouse_state & SDL_BUTTON_MASK(button));
    }

    auto InputManager::mouse_button_just_released(uint32_t button) -> bool {

        return !(m_current_mouse_state & SDL_BUTTON_MASK(button)) && (m_last_mouse_state & SDL_BUTTON_MASK(button));
    }

    auto InputManager::get_mouse_state() -> std::pair<float, float> {
        float x = 0.f;
        float y = 0.f;
        SDL_GetMouseState(&x, &y);
        return {x, y};
    }

    auto InputManager::flush_key_state() -> void {
        std::memcpy(m_last_key_state.data(), m_current_key_state, m_last_key_state.size() * sizeof(bool));
        m_last_mouse_state = m_current_mouse_state;
        m_current_mouse_state = SDL_GetMouseState(nullptr, nullptr);

        SDL_GetRelativeMouseState(&m_mouse_dx, &m_mouse_dy);
    }
} // horizon