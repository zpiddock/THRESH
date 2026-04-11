//
// Created by Admin on 05/04/2026.
//

#pragma once
#include <memory>
#include <string>

#include "SDL3/SDL_video.h"

namespace thresh {

    struct WindowContext {
        std::string title;
        int width;
        int height;
        SDL_WindowFlags flags;
    };

class Window {

    public:
        explicit Window(const WindowContext& ctx);

        virtual ~Window();

        virtual auto init_window(const WindowContext& ctx) -> void = 0;

        [[nodiscard]] auto getWindow() const -> SDL_Window*;
        auto setWindowTitle(const char* title) const -> bool;

        [[nodiscard]] auto shouldClose() const -> bool;
        auto setShouldClose(bool shouldClose) -> void;

    protected:
        WindowContext m_context = {};
        SDL_Window* m_window = nullptr;
        bool m_window_should_close = false;
};

} // thresh
