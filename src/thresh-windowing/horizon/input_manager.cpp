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

    auto InputManager::is_key_down(const SDL_Scancode scancode) -> bool {

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

    auto InputManager::flush_key_state() -> void {
        std::memcpy(m_last_key_state.data(), m_current_key_state, m_last_key_state.size() * sizeof(bool));
    }
} // horizon