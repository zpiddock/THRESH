//
// Created by Admin on 06/04/2026.
//

#include <expected>
#include <filesystem>

#include "SDL3/SDL.h"

#include "engine.hpp"
#include "flux.hpp"
#include "substratum/log.hpp"
#include "substratum/filesystem/vfs.hpp"

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
        if (!substratum::VFS::init(nullptr)) {
            return std::unexpected("VFS could not initialize!");
        }
        const auto assets_path = (std::filesystem::current_path() / "assets").string();
        substratum::VFS::mount(assets_path, "/", true);

        if (!m_application) {
            return std::unexpected("Application not set");
        }

        if (!SDL_Init(ctx.init_flags)) {
            SUB_FATAL("SDL could not initialize! SDL_Error: {}", SDL_GetError());
        }

        m_window = flux::create_window(ctx.API_TYPE, {.title = ctx.title,
                                                      .width = ctx.width,
                                                      .height = ctx.height,
                                                      .flags = ctx.window_flags});

        flux::init(ctx.API_TYPE);


        m_input_manager = std::make_unique<horizon::InputManager>();
        m_input_manager->init();

        m_application->startup();

        return true;
    }

    auto Engine::run() -> void {
        if (!m_application || !m_window) {
            return;
        }

        m_last_frame_time = std::chrono::high_resolution_clock::now();

        while (!m_window->shouldClose()) {
            m_input_manager->flush_key_state();
            // Poll events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_EVENT_QUIT: {
                        m_window->setShouldClose(true);
                        break;
                    }
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                        int32_t w, h;
                        SDL_GetWindowSizeInPixels(m_window->getWindow(), &w, &h);
                        flux::on_resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
                        break;
                    }
                    default:
                        break;
                }
            }

            // Time calculations
            auto current_time = std::chrono::high_resolution_clock::now();
            auto delta_time   = std::chrono::duration<float>(current_time - m_last_frame_time).count();
            m_last_frame_time = current_time;

            // Application update
            m_application->update(delta_time);

            // Rendering
            int fb_width, fb_height;
            SDL_GetWindowSizeInPixels(m_window->getWindow(), &fb_width, &fb_height);
            if (fb_width == 0 || fb_height == 0) {
                flux::swap_buffers(m_window.get());
                continue;
            }

            flux::clear_colour(1.f, 0.f, 0.f, 1.0f);
            flux::clear(flux::ClearFlags::Color | flux::ClearFlags::Depth);

            m_application->render();

            flux::swap_buffers(m_window.get());
        }

        shutdown();
    }

    auto Engine::window() -> Window* {
        return m_window.get();
    }

    auto Engine::input() -> horizon::InputManager* {
        return m_input_manager.get();
    }

    auto Engine::shutdown() -> void {
        flux::shutdown();

        m_application->shutdown();
        m_application = nullptr;
        substratum::VFS::shutdown();
        m_window.reset();
    }
} // thresh
