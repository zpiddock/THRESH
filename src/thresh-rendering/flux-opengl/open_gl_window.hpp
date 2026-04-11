//
// Created by Admin on 11/04/2026.
//

#pragma once
#include "horizon/window.hpp"

namespace flux {

class OpenGLWindow : public thresh::Window {
    public:
        explicit OpenGLWindow(const thresh::WindowContext& ctx)
            : Window(ctx) {
        }

        auto init_window(const thresh::WindowContext& ctx) -> void override;

        static auto create(const thresh::WindowContext& ctx) -> std::unique_ptr<OpenGLWindow>;

    private:

        SDL_GLContext m_gl_context = nullptr;
};

} // flux
