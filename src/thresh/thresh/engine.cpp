//
// Created by Admin on 06/04/2026.
//

#include <expected>

#include "SDL3/SDL.h"
#include "glad/gl.h"

#include "engine.hpp"

#include "flux-opengl/open_gl_graphics_api.hpp"
#include "flux-opengl/open_gl_window.hpp"
#include "substratum/log.hpp"

namespace thresh {
    auto Engine::create(Application* application) -> Engine& {
        get_instance().m_application = application;
        return get_instance();
    }

    auto Engine::get_instance() -> Engine& {
        static Engine m_instance;

        return m_instance;
    }

    auto Engine::init(const EngineContext& ctx) -> std::expected<bool, std::string> {

        if (!m_application) {
            return std::unexpected("Application not set");
        }

        if (!SDL_Init(ctx.init_flags)) {
            SUB_FATAL("SDL could not initialize! SDL_Error: {}", SDL_GetError());
        }

        switch (ctx.API_TYPE) {

            case flux::API_TYPE::OpenGL: {

                m_window = flux::OpenGLWindow::create({.title = ctx.title, .width = ctx.width, .height = ctx.height, .flags = ctx.window_flags});
                m_graphics_api = std::make_unique<flux::OpenGLGraphicsAPI>();
                break;
            }

            default: {
                std::unreachable();
                break;
            }
        }

        return true;
    }

    auto Engine::run() -> void {

        if (!m_application || !m_window) {
            return;
        }

        m_last_frame_time = std::chrono::high_resolution_clock::now();

        while (!m_window->shouldClose()) {

            // Poll events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {

                switch (event.type) {
                    case SDL_EVENT_QUIT: {
                        m_window->setShouldClose(true);
                        break;
                    }
                    default:
                        break;
                }
            }
            // Time calculations
            auto current_time = std::chrono::high_resolution_clock::now();
            auto delta_time   = std::chrono::duration<float>(current_time - m_last_frame_time).count();
            m_last_frame_time  = current_time;

            // Application updates
            m_application->update(delta_time);

            // Rendering
            // before issuing draw calls, check we're not minimised
            int fb_width, fb_height;
            SDL_GetWindowSizeInPixels(m_window->getWindow(), &fb_width, &fb_height);
            if (fb_width == 0 || fb_height == 0) {
                m_graphics_api->swap_buffers(m_window.get());
                continue;
            }
            m_graphics_api->clear_colour(1.f, 0.f, 0.f, 1.0f);
            m_graphics_api->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_graphics_api->swap_buffers(m_window.get());
        }

        m_application->shutdown();
        m_application = nullptr;
        m_window.reset();
    }

    auto Engine::get_window() -> Window* {
        return m_window.get();
    }
} // thresh