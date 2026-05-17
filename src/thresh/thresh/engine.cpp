//
// Created by Admin on 06/04/2026.
//

#include <SDL3/SDL.h>

#include "thresh.hpp"

#include "engine.hpp"
#include "flux/graphics_utils.hpp"
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

        WindowContext window_context = {.title = ctx.title,
                                                      .width = ctx.width,
                                                      .height = ctx.height,
                                                      .flags = ctx.window_flags};
        m_window = std::make_unique<Window>(window_context);

        m_graphics_utils = std::make_unique<flux::GraphicsUtils>();
        m_graphics_utils->vulkan_init(*m_window);
        m_graphics_utils->imgui_init();

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

        m_window->set_relative_mouse_mode(true);

        while (!m_window->shouldClose()) {
            m_input_manager->flush_key_state();
            // Poll events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                m_graphics_utils->imgui_process_event(event);
                switch (event.type) {
                    case SDL_EVENT_QUIT: {
                        m_window->setShouldClose(true);
                        break;
                    }
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                        m_graphics_utils->set_framebuffer_resized(true);
                        break;
                    }
                    default:
                        break;
                }
            }

            // Time calculations
            auto current_time = std::chrono::high_resolution_clock::now();
            const auto delta_time   = std::chrono::duration<float>(current_time - m_last_frame_time).count();
            m_last_frame_time = current_time;

            // Application update
            m_application->update(delta_time);

            if (m_active_scene) {
                m_active_scene->update(delta_time);
                if (auto cam = m_active_scene->compute_active_camera_data(m_graphics_utils->get_aspect_ratio())) {
                    m_graphics_utils->set_camera_data(*cam);
                }
            }

            // Rendering
            int fb_width, fb_height;
            SDL_GetWindowSizeInPixels(m_window->getWindow(), &fb_width, &fb_height);
            if (fb_width == 0 || fb_height == 0 || m_window->is_minimised()) {
                SDL_WaitEvent(nullptr);
                SDL_GetWindowSizeInPixels(m_window->getWindow(), &fb_width, &fb_height);
                continue;
            }
            if (m_graphics_utils->imgui_new_frame()) {
                m_application->debug_render();
            }
            m_graphics_utils->draw_frame();

            // m_application->render();
        }
        m_graphics_utils->shutdown();

        shutdown();
    }

    auto Engine::window() -> Window* {
        return m_window.get();
    }

    auto Engine::input() -> horizon::InputManager* {

        return m_input_manager.get();
    }

    auto Engine::graphics() -> flux::GraphicsUtils* {

        if (!m_graphics_utils) {
            m_graphics_utils = std::make_unique<flux::GraphicsUtils>();
        }
        return m_graphics_utils.get();
    }

    auto Engine::active_scene() -> Scene* {
        return m_active_scene.get();
    }

    auto Engine::transition_scene(std::unique_ptr<Scene> new_scene) -> void {
        m_active_scene = std::move(new_scene);
    }

    auto Engine::shutdown() -> void {
        m_application->shutdown();
        m_application = nullptr;
        m_input_manager.reset();
        m_graphics_utils->imgui_shutdown();
        m_graphics_utils.reset();
        substratum::VFS::shutdown();
        m_window.reset();
    }
} // thresh
