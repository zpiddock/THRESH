//
// Created by Admin on 11/04/2026.
//

#include "open_gl_window.hpp"

#include "glad/gl.h"
#include "substratum/log.hpp"

namespace flux {
    auto OpenGLWindow::init_window(const thresh::WindowContext& ctx) -> void {

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        m_window = SDL_CreateWindow(ctx.title.c_str(), ctx.width, ctx.height, ctx.flags);
        m_gl_context = SDL_GL_CreateContext(m_window);
        SDL_GL_MakeCurrent(m_window, m_gl_context);

        if (!gladLoadGL(SDL_GL_GetProcAddress)) {

            SUB_FATAL("Could not load OpenGL Functions, Hard Crash!");
        }
    }

    auto OpenGLWindow::create(const thresh::WindowContext& ctx) -> std::unique_ptr<OpenGLWindow> {

        auto window = std::make_unique<OpenGLWindow>(ctx);
        window->init_window(ctx);
        return window;
    }
} // flux