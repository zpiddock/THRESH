//
// Created by Admin on 06/04/2026.
//

#include <SDL3/SDL.h>

#include "thresh.hpp"

#include "engine.hpp"
#include "flux/graphics_utils.hpp"
#include "scene/scene_serializer.hpp"
#include "substratum/filesystem/vfs.hpp"
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
        SUB_INFO("Engine init: '{}' {}x{}", ctx.title, ctx.width, ctx.height);

        if (!substratum::VFS::init(nullptr)) {
            return std::unexpected("VFS could not initialize!");
        }
        const auto assets_path = (std::filesystem::current_path() / "assets").string();
        SUB_DEBUG("Mounting assets at '{}'", assets_path);
        substratum::VFS::mount(assets_path, "/", true);

        if (!m_application) {
            return std::unexpected("Application not set");
        }

        SUB_DEBUG("Initialising SDL (flags=0x{:x})", ctx.init_flags);
        if (!SDL_Init(ctx.init_flags)) {
            SUB_FATAL("SDL could not initialize! SDL_Error: {}", SDL_GetError());
        }

        WindowContext window_context = {.title = ctx.title,
                                                      .width = ctx.width,
                                                      .height = ctx.height,
                                                      .flags = ctx.window_flags};
        m_window = std::make_unique<Window>(window_context);

        SUB_DEBUG("Initialising graphics subsystem");
        m_graphics_utils = std::make_unique<helix::GraphicsUtils>();
        m_graphics_utils->vulkan_init(*m_window);
        m_graphics_utils->register_dummy_texture();
        m_graphics_utils->imgui_init();

        SUB_DEBUG("Initialising input manager");
        m_input_manager = std::make_unique<horizon::InputManager>();
        m_input_manager->init();

        SUB_DEBUG("Application startup");
        m_application->startup();

        SUB_INFO("Engine init complete");
        return true;
    }

    auto Engine::run() -> void {
        if (!m_application || !m_window) {
            SUB_ERROR("Engine::run() called without application or window");
            return;
        }

        SUB_INFO("Entering main loop");
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

            // Reset debug lines if they exist
            if (auto* dlr = m_graphics_utils->debug_line_renderer()) {
                dlr->clear();
            }

            // Application update
            m_application->update(delta_time);

            if (m_active_scene) {
                m_active_scene->update(delta_time);
                if (auto cam = m_active_scene->compute_active_camera_data(m_graphics_utils->get_aspect_ratio())) {
                    m_graphics_utils->set_camera_data(*cam);
                }
                if (auto lights = m_active_scene->compute_active_light_data()) {
                    m_graphics_utils->set_light_data(*lights);
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

    auto Engine::graphics() -> helix::GraphicsUtils* {

        if (!m_graphics_utils) {
            m_graphics_utils = std::make_unique<helix::GraphicsUtils>();
        }
        return m_graphics_utils.get();
    }

    auto Engine::assets() -> AssetsLoader & {
        if (!m_asset_manager) {
            m_asset_manager = std::make_unique<AssetsLoader>(*graphics());
        }
        return *m_asset_manager;
    }

    auto Engine::active_scene() -> Scene* {
        return m_active_scene.get();
    }

    auto Engine::load_scene(const std::string &path) -> std::unique_ptr<Scene> {

        return SceneSerializer::load_scene(assets(), path);
    }

    auto Engine::transition_scene(std::unique_ptr<Scene> new_scene) -> void {
        SUB_INFO("Transitioning scene (had_previous={})", m_active_scene != nullptr);
        m_active_scene = std::move(new_scene);
    }

    auto Engine::shutdown() -> void {
        SUB_INFO("Engine shutdown");
        m_application->shutdown();
        m_application = nullptr;
        m_asset_manager.reset();
        m_input_manager.reset();
        m_graphics_utils->imgui_shutdown();
        m_graphics_utils.reset();
        substratum::VFS::shutdown();
        m_window.reset();
        SUB_DEBUG("Engine shutdown complete");
    }
} // thresh
