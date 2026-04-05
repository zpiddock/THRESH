//
// Created by Admin on 05/04/2026.
//

#include "window.hpp"

namespace thresh {
    Window::Window(const char* title, const int width, const int height, const SDL_WindowFlags flags) {

        m_window = SDL_CreateWindow(title, width, height, flags);
    }

    Window::~Window() {

        SDL_DestroyWindow(m_window);
    }

    auto Window::getWindow() const -> SDL_Window* {
        return m_window;
    }

    auto Window::setWindowTitle(const char* title) const -> bool {
        return SDL_SetWindowTitle(m_window, title) == 0;
    }

    auto Window::shouldClose() const -> bool {
        return m_window_should_close;
    }

    auto Window::setShouldClose(bool shouldClose) -> void {
        m_window_should_close = shouldClose;
    }
} // thresh