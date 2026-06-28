//
// Created by Admin on 06/04/2026.
//

#pragma once

#include <chrono>
#include <expected>
#include <memory>
#include <string>

#include "SDL3/SDL.h"

#include "application.hpp"
#include "asset/assets_loader.hpp"
#include "asset/model_loader.hpp"
#include "flux/renderer.hpp"
#include "horizon/input_manager.hpp"
#include "horizon/window.hpp"
#include "scene/scene.hpp"

namespace thresh {

    struct EngineContext {
        std::string title;
        int width;
        int height;

        SDL_InitFlags init_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

        SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

        flux::RenderConfig render_config{};
    };

class Engine {
    public:
        Engine() = default;

        Engine(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine& operator=(Engine&&) = delete;

        static auto get_instance() -> Engine&;

        static auto create(Application* application) -> Engine&;

        auto init(const EngineContext& ctx) -> std::expected<bool, std::string>;

        auto run() -> void;

        auto window() -> Window*;

        auto input() -> horizon::InputManager*;

        auto graphics() -> flux::Renderer*;

        auto assets() -> AssetsLoader&;

        auto models() -> ModelLoader&;

        auto active_scene() -> Scene*;

        auto load_scene(const std::string& path) -> std::unique_ptr<Scene>;

        auto transition_scene(std::unique_ptr<Scene> new_scene) -> void;

        auto shutdown() -> void;

    private:

        std::chrono::high_resolution_clock::time_point m_last_frame_time;

        Application*                           m_application = nullptr;
        std::unique_ptr<Window>                m_window;
        std::unique_ptr<flux::Renderer>        m_renderer;
        std::unique_ptr<horizon::InputManager> m_input_manager;
        std::unique_ptr<AssetsLoader>          m_asset_manager;
        std::unique_ptr<ModelLoader>           m_model_loader;

        std::unique_ptr<Scene> m_active_scene;
};

} // thresh
