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
    class Camera;
    class Scene;
    class MaterialSystem;
    class AssetSystem;
    class ForwardPass;
    struct DirectionalLight;
    struct AmbientLight;

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
     * Simplified update callback for the easy run() path.
     * Provides the scene and delta time — camera, rendering, etc. are handled internally.
     */
    using SimpleUpdateCallback = std::function<void(Scene &scene, float delta_time)>;

    /**
     * THRESH engine core.
     *
     * Manages the window, input, renderer, and render graph.
     * Runs three threads: main (GLFW events), update (game logic), render (GPU).
     *
     * Two run paths:
     *
     * **Full control:**
     *   Engine engine(config);
     *   engine.run(
     *       [](RenderGraph& graph) { ... define passes ... },
     *       [](FrameSnapshot& snap, float dt) { ... update logic ... }
     *   );
     *
     * **Simple (default render graph):**
     *   Engine engine(config);
     *   engine.get_camera().set_position({0, 0, 3});
     *   auto& scene = engine.get_scene();
     *   // ... set up scene entities ...
     *   engine.run();  // or engine.run([](Scene& s, float dt) { ... });
     */
    class THRESH_API Engine {
    public:
        explicit Engine(const EngineConfig &config);
        ~Engine();

        Engine(const Engine &) = delete;
        Engine &operator=(const Engine &) = delete;

        // ── Full-control run path ──────────────────────────────────────────

        /**
         * Start the engine loop with full control over the render graph and update logic.
         * Blocks until the window is closed or request_shutdown() is called.
         * @param graph_setup Called once to define the render graph
         * @param update Called each tick on the update thread
         */
        auto run(GraphSetupCallback graph_setup, UpdateCallback update) -> void;

        // ── Simplified run path ────────────────────────────────────────────

        /**
         * Start the engine loop with a default render graph (ForwardPass + PBR).
         * Camera, scene extraction, and rendering are handled internally.
         * @param update Called each tick with the scene and delta time
         */
        auto run(SimpleUpdateCallback update) -> void;

        /**
         * Start the engine loop with a default render graph and no per-frame logic.
         * Renders the scene as-is with the built-in camera.
         */
        auto run() -> void;

        // ── Standard stack accessors (lazy-init on first call) ─────────────

        /**
         * Get the built-in first-person camera.
         * Initializes the standard rendering stack on first call.
         */
        [[nodiscard]] auto get_camera() -> Camera &;

        /**
         * Get the built-in ECS scene.
         * Initializes the standard rendering stack on first call.
         */
        [[nodiscard]] auto get_scene() -> Scene &;

        /**
         * Get the built-in material system.
         * Initializes the standard rendering stack on first call.
         */
        [[nodiscard]] auto get_material_system() -> MaterialSystem &;

        /**
         * Get the built-in asset system.
         * Initializes the standard rendering stack on first call.
         */
        [[nodiscard]] auto get_asset_system() -> AssetSystem &;

        /**
         * Get the built-in forward rendering pass.
         * Initializes the standard rendering stack on first call.
         */
        [[nodiscard]] auto get_forward_pass() -> ForwardPass &;

        /**
         * Set the directional (sun) light for the simplified run path.
         */
        auto set_sun(const DirectionalLight &sun) -> void;

        /**
         * Get the current directional (sun) light.
         */
        [[nodiscard]] auto get_sun() const -> const DirectionalLight &;

        /**
         * Set the ambient light for the simplified run path.
         */
        auto set_ambient(const AmbientLight &ambient) -> void;

        /**
         * Get the current ambient light.
         */
        [[nodiscard]] auto get_ambient() const -> const AmbientLight &;

        // ── Core accessors ─────────────────────────────────────────────────

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
        auto ensure_standard_stack() -> void;

        EngineConfig m_config;

        // Core systems (ordered for destruction)
        std::unique_ptr<horizon::Window> m_window;
        std::unique_ptr<horizon::Input> m_input;
        std::unique_ptr<Renderer> m_renderer;
        std::unique_ptr<RenderGraph> m_render_graph;
        std::unique_ptr<substratum::FileWatcher> m_shader_watcher;

        // Standard rendering stack (lazy-init, ordered for destruction)
        // Destroyed bottom-to-top: forward_pass first (depends on device + material_system)
        std::unique_ptr<Camera> m_camera;
        std::unique_ptr<Scene> m_scene;
        std::unique_ptr<AssetSystem> m_asset_system;
        std::unique_ptr<MaterialSystem> m_material_system;
        std::unique_ptr<ForwardPass> m_forward_pass;

        // Internal shared state for simple run path
        struct InternalRenderState;
        std::unique_ptr<InternalRenderState> m_internal_state;

        // Lighting defaults for simple run path
        std::unique_ptr<DirectionalLight> m_sun;
        std::unique_ptr<AmbientLight> m_ambient;

        // Threading
        std::thread m_update_thread;
        std::thread m_render_thread;
        std::atomic<bool> m_running{false};

        // Triple-buffered frame snapshots (lock-free update -> render)
        static constexpr std::uint32_t SNAPSHOT_COUNT = 3;
        std::array<FrameSnapshot, SNAPSHOT_COUNT> m_snapshots{};
        std::atomic<std::uint32_t> m_latest_snapshot{0};

        // Graph setup callback (stored for re-invocation on resize)
        GraphSetupCallback m_graph_setup;

        // Resize tracking
        std::uint32_t m_last_fb_width = 0;
        std::uint32_t m_last_fb_height = 0;
    };
} // namespace thresh
