#pragma once

#include "thresh/engine_config.hpp"
#include "thresh/frame_snapshot.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

#ifdef _WIN32
#ifdef THRESH_EXPORTS
#define THRESH_API __declspec(dllexport)
#else
#define THRESH_API __declspec(dllimport)
#endif
#else
#define THRESH_API
#endif

namespace horizon {
    class Window;
    class Input;
} // namespace horizon

namespace substratum {
    class FileWatcher;
} // namespace substratum

namespace thresh {
    class Renderer;
    class RenderGraph;

    /**
     * Callback for setting up the render graph (define passes, resources, etc.)
     * Called once before the render loop begins.
     */
    using GraphSetupCallback = std::function<void(RenderGraph &graph)>;

    /**
     * Callback for game/app update logic.
     * Called each tick on the update thread.
     */
    using UpdateCallback = std::function<void(FrameSnapshot &snapshot, float delta_time)>;

    /**
     * THRESH engine core.
     *
     * Manages the window, input, renderer, and render graph.
     * Runs three threads: main (GLFW events), update (game logic), render (GPU).
     *
     * Usage:
     *   Engine engine(config);
     *   engine.run(
     *       [](RenderGraph& graph) { ... define passes ... },
     *       [](FrameSnapshot& snap, float dt) { ... update logic ... }
     *   );
     */
    class THRESH_API Engine {
    public:
        explicit Engine(const EngineConfig &config);
        ~Engine();

        Engine(const Engine &) = delete;
        Engine &operator=(const Engine &) = delete;

        /**
         * Start the engine loop.
         * Blocks until the window is closed or request_shutdown() is called.
         * @param graph_setup Called once to define the render graph
         * @param update Called each tick on the update thread
         */
        auto run(GraphSetupCallback graph_setup, UpdateCallback update) -> void;

        /**
         * Request a clean shutdown. Thread-safe.
         */
        auto request_shutdown() -> void;

        [[nodiscard]] auto get_renderer() -> Renderer &;
        [[nodiscard]] auto get_render_graph() -> RenderGraph &;
        [[nodiscard]] auto get_window() -> horizon::Window &;
        [[nodiscard]] auto get_input() -> horizon::Input &;

    private:
        auto update_thread_fn(UpdateCallback update) -> void;
        auto render_thread_fn() -> void;

        EngineConfig m_config;

        // Core systems (ordered for destruction)
        std::unique_ptr<horizon::Window> m_window;
        std::unique_ptr<horizon::Input> m_input;
        std::unique_ptr<Renderer> m_renderer;
        std::unique_ptr<RenderGraph> m_render_graph;
        std::unique_ptr<substratum::FileWatcher> m_shader_watcher;

        // Threading
        std::thread m_update_thread;
        std::thread m_render_thread;
        std::atomic<bool> m_running{false};

        // Triple-buffered frame snapshots (lock-free update -> render)
        static constexpr std::uint32_t SNAPSHOT_COUNT = 3;
        std::array<FrameSnapshot, SNAPSHOT_COUNT> m_snapshots{};
        std::atomic<std::uint32_t> m_latest_snapshot{0};

        // Resize tracking
        std::uint32_t m_last_fb_width = 0;
        std::uint32_t m_last_fb_height = 0;
    };
} // namespace thresh
