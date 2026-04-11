//
// Created by Admin on 11/04/2026.
//

#include "open_gl_graphics_api.hpp"

#include "glad/gl.h"
#include "horizon/window.hpp"
#include "SDL3/SDL_video.h"

namespace flux {
    OpenGLGraphicsAPI::~OpenGLGraphicsAPI() = default;

    auto OpenGLGraphicsAPI::get_api_type() -> API_TYPE {
        return API_TYPE::OpenGL;
    }

    auto OpenGLGraphicsAPI::clear(const int flags) -> void {
        ::glClear(flags);
    }

    auto OpenGLGraphicsAPI::clear_colour(float r, float g, float b, float a) -> void {

        ::glClearColor(r, g, b, a);
    }

    auto OpenGLGraphicsAPI::swap_buffers(const thresh::Window* window) -> void {
        SDL_GL_SwapWindow(window->getWindow());
    }

    auto OpenGLGraphicsAPI::on_resize(uint32_t width, uint32_t height) -> void {
    }
} // flux