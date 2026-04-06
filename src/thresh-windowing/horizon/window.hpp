//
// Created by Admin on 05/04/2026.
//

#pragma once
#include <string>

#include "SDL3/SDL_video.h"

namespace thresh {

class Window {

    public:
        Window(const std::string& title, int width, int height, SDL_WindowFlags flags);
        ~Window();
        [[nodiscard]] auto getWindow() const -> SDL_Window*;
        auto setWindowTitle(const char* title) const -> bool;

        [[nodiscard]] auto shouldClose() const -> bool;
        auto setShouldClose(bool shouldClose) -> void;

    private:
        SDL_Window* m_window = nullptr;
        SDL_GLContext m_gl_context = nullptr;
        bool m_window_should_close = false;
};

} // thresh
