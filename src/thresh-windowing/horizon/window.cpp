//
// Created by Admin on 05/04/2026.
//

#include "window.hpp"

#include <string>

#include "glad/gl.h"
#include "substratum/log.hpp"

namespace thresh {
    Window::Window(const std::string& title, const int width, const int height, const SDL_WindowFlags flags) {

        SUB_DEBUG("Creating Window: ", title);

        m_window = SDL_CreateWindow(title.c_str(), width, height, flags);
        m_gl_context = SDL_GL_CreateContext(m_window);

        SDL_GL_MakeCurrent(m_window, m_gl_context);

        int result = gladLoadGL(SDL_GL_GetProcAddress);
        if (result == 0){
            SUB_FATAL("Failed to initialize GLAD");
        } else {
            SUB_DEBUG("GLAD Initialized");
            SUB_TRACE("GLAD Version: {}.{}", GLAD_VERSION_MAJOR(result), GLAD_VERSION_MINOR(result));
        }
    }

    Window::~Window() {

        SUB_DEBUG("Destroying Window: " + std::string(SDL_GetWindowTitle(m_window)));
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