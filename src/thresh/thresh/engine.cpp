//
// Created by Admin on 06/04/2026.
//

#include <expected>

#include "SDL3/SDL.h"
#include "glad/gl.h"

#include "engine.hpp"

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

        // Set SDL Context Flags
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, ctx.GL_CONTEXT_MAJOR_VERSION);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, ctx.GL_CONTEXT_MINOR_VERSION);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, ctx.GL_CONTEXT_PROFILE);
        // for now always assign GL Debug Flags
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

        m_window = std::make_unique<Window>(ctx.title.c_str(), ctx.width, ctx.height, ctx.window_flags);

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
                SDL_GL_SwapWindow(m_window->getWindow());
                continue;
            }
            ::glClearColor(1.f, 0.f, 0.f, 1.0f);
            ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            SDL_GL_SwapWindow(m_window->getWindow());
        }

        SDL_Quit();
    }

    auto Engine::get_window() -> Window* {
        return m_window.get();
    }
} // thresh