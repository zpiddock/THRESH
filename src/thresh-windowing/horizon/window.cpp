//
// Created by Admin on 05/04/2026.
//

#include "window.hpp"

#include <string>

#include "SDL3/SDL_init.h"
#include "substratum/log.hpp"

namespace thresh {

    Window::Window(const WindowContext& ctx) : m_context(ctx) {

        SUB_DEBUG("Creating Window: ", ctx.title);
        init_window(ctx);
    }

    Window::~Window() {

        SUB_DEBUG("Destroying Window: " + std::string(SDL_GetWindowTitle(m_window)));
        SDL_Quit();
        SDL_DestroyWindow(m_window);
    }

    auto Window::init_window(const WindowContext& ctx) -> void {

        m_window = SDL_CreateWindow(ctx.title.c_str(), ctx.width, ctx.height, ctx.flags);
        if (!m_window) {
            SUB_FATAL("Could not create window: ", SDL_GetError());
        }
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