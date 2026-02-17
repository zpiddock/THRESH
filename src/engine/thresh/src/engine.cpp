#include "thresh/engine.hpp"
#include "thresh/renderer.hpp"
#include "thresh/render_graph.hpp"
#include "thresh/camera.hpp"
#include "thresh/scene.hpp"
#include "thresh/asset_system.hpp"
#include "thresh/material_system.hpp"
#include "thresh/forward_pass.hpp"
#include "thresh/render_data.hpp"
#include "horizon/window.hpp"
#include "horizon/input.hpp"
#include "substratum/file_watcher.hpp"
#include "substratum/vfs.hpp"
#include "substratum/log.hpp"

#include <chrono>
#include <mutex>
#include <stdexcept>
#include <filesystem>

namespace thresh {

    // Internal shared state used by the simple run() path to bridge update -> render threads
    struct Engine::InternalRenderState {
        std::mutex mutex;
        FrameRenderData render_data;
        FrameRenderData render_thread_copy;
    };

    Engine::Engine(const EngineConfig &config)
        : m_config(config) {
        SUB_INFO("Initializing THRESH Engine");

        // --- Virtual Filesystem ---
        if (!substratum::VFS::init(nullptr)) {
            SUB_FATAL("Failed to initialize VFS");
            throw std::runtime_error("Failed to initialize VFS");
        }

        if (config.vfs_mounts.empty()) {
            // Default: mount current working directory at root
            auto cwd = std::filesystem::current_path().string();
            substratum::VFS::mount(cwd, "/", true);
        } else {
            for (const auto &entry : config.vfs_mounts) {
                substratum::VFS::mount(entry.real_path, entry.mount_point, entry.append);
            }
        }

        // --- Window ---
        horizon::Window::Config window_config{};
        window_config.title = config.window_title;
        window_config.width = config.window_width;
        window_config.height = config.window_height;
        window_config.resizable = config.window_resizable;
        window_config.maximized = config.window_maximized;

        m_window = std::make_unique<horizon::Window>(window_config);

        // --- Input ---
        m_input = std::make_unique<horizon::Input>(*m_window);

        // --- Renderer ---
        m_renderer = std::make_unique<Renderer>(*m_window, config);

        // --- Render Graph ---
        m_render_graph = std::make_unique<RenderGraph>(*m_renderer);

        // --- Shader file watcher ---
        if (config.enable_shader_hot_reload) {
            auto shader_dir = config.shader_dir.empty()
                ? std::filesystem::current_path() / "assets" / "shaders"
                : config.shader_dir;
            if (std::filesystem::exists(shader_dir)) {
                substratum::FileWatcher::Config watcher_config{};
                watcher_config.directory = shader_dir;
                watcher_config.watch_subdirectories = true;
                watcher_config.extensions_filter = {".vert", ".frag", ".comp", ".geom", ".tesc", ".tese", ".glsl"};

                m_shader_watcher = std::make_unique<substratum::FileWatcher>(watcher_config);
                SUB_INFO("Shader hot reload enabled: watching {}", shader_dir.string());
            } else {
                SUB_WARN("Shader directory not found: {} - hot reload disabled", shader_dir.string());
            }
        }

        // Initialize lighting defaults
        m_sun = std::make_unique<DirectionalLight>();
        m_ambient = std::make_unique<AmbientLight>();

        auto [fb_w, fb_h] = m_window->get_framebuffer_size();
        m_last_fb_width = fb_w;
        m_last_fb_height = fb_h;

        SUB_INFO("THRESH Engine initialized");
    }

    Engine::~Engine() {
        m_running = false;

        if (m_update_thread.joinable()) {
            m_update_thread.join();
        }
        if (m_render_thread.joinable()) {
            m_render_thread.join();
        }

        if (m_renderer) {
            m_renderer->wait_idle();
        }

        // Destroy standard stack before renderer (forward_pass first due to member order)
        m_forward_pass.reset();
        m_material_system.reset();
        m_asset_system.reset();
        m_scene.reset();
        m_camera.reset();
        m_internal_state.reset();

        // unique_ptrs destroy in reverse member order

        // Shutdown VFS after all systems are destroyed
        substratum::VFS::shutdown();

        SUB_INFO("THRESH Engine shutdown complete");
    }

    // ── Standard stack lazy initialization ──────────────────────────────────────

    auto Engine::ensure_standard_stack() -> void {
        if (m_camera) return; // Already initialized

        SUB_INFO("Initializing standard rendering stack");

        auto &device = m_renderer->get_device_ref();

        m_camera = std::make_unique<Camera>(*m_window);
        m_scene = std::make_unique<Scene>();
        m_asset_system = std::make_unique<AssetSystem>(device);
        m_material_system = std::make_unique<MaterialSystem>(device);

        auto shader_dir = m_config.shader_dir.empty()
            ? std::filesystem::current_path() / "assets" / "shaders"
            : m_config.shader_dir;

        m_forward_pass = std::make_unique<ForwardPass>(ForwardPass::Config{
            .device = &device,
            .material_system = m_material_system.get(),
            .shader_dir = shader_dir
        });

        SUB_INFO("Standard rendering stack initialized");
    }

    // ── Standard stack getters ──────────────────────────────────────────────────

    auto Engine::get_camera() -> Camera & {
        ensure_standard_stack();
        return *m_camera;
    }

    auto Engine::get_scene() -> Scene & {
        ensure_standard_stack();
        return *m_scene;
    }

    auto Engine::get_material_system() -> MaterialSystem & {
        ensure_standard_stack();
        return *m_material_system;
    }

    auto Engine::get_asset_system() -> AssetSystem & {
        ensure_standard_stack();
        return *m_asset_system;
    }

    auto Engine::get_forward_pass() -> ForwardPass & {
        ensure_standard_stack();
        return *m_forward_pass;
    }

    auto Engine::set_sun(const DirectionalLight &sun) -> void {
        *m_sun = sun;
    }

    auto Engine::get_sun() const -> const DirectionalLight & {
        return *m_sun;
    }

    auto Engine::set_ambient(const AmbientLight &ambient) -> void {
        *m_ambient = ambient;
    }

    auto Engine::get_ambient() const -> const AmbientLight & {
        return *m_ambient;
    }

    // ── Full-control run ────────────────────────────────────────────────────────

    auto Engine::run(GraphSetupCallback graph_setup, UpdateCallback update) -> void {
        SUB_INFO("Starting engine loop");

        // Store graph setup callback for re-invocation on resize
        m_graph_setup = graph_setup;

        // Let the user define their render graph
        m_graph_setup(*m_render_graph);

        if (!m_render_graph->compile()) {
            SUB_FATAL("Failed to compile initial render graph");
            throw std::runtime_error("Failed to compile initial render graph");
        }

        m_running = true;

        // Spawn worker threads
        m_update_thread = std::thread([this, update = std::move(update)]() {
            update_thread_fn(std::move(update));
        });
        m_render_thread = std::thread([this]() {
            render_thread_fn();
        });

        // Main thread: GLFW event loop
        while (m_running) {
            m_window->poll_events();

            if (m_window->should_close()) {
                m_running = false;
                break;
            }

            // Detect resize
            auto [fb_w, fb_h] = m_window->get_framebuffer_size();
            if (fb_w != m_last_fb_width || fb_h != m_last_fb_height) {
                m_last_fb_width = fb_w;
                m_last_fb_height = fb_h;

                if (fb_w > 0 && fb_h > 0) {
                    m_renderer->request_resize(fb_w, fb_h);
                    m_render_graph->invalidate();
                }
            }

            // Poll shader file watcher
            if (m_shader_watcher && m_shader_watcher->is_valid()) {
                auto changes = m_shader_watcher->poll();
                if (!changes.empty()) {
                    SUB_INFO("Detected {} shader file change(s) - hot reload triggered", changes.size());
                    // TODO: trigger shader recompilation via ShaderProgram::check_and_reload()
                }
            }
        }

        // Wait for threads to finish
        if (m_update_thread.joinable()) {
            m_update_thread.join();
        }
        if (m_render_thread.joinable()) {
            m_render_thread.join();
        }
        m_update_thread = {};
        m_render_thread = {};

        m_renderer->wait_idle();
        SUB_INFO("Engine loop ended");
    }

    // ── Simplified run ──────────────────────────────────────────────────────────

    auto Engine::run(SimpleUpdateCallback update) -> void {
        ensure_standard_stack();

        // Register camera as input subscriber for mouse/keyboard
        m_input->add_subscriber(m_camera.get());

        // Create shared state for update -> render communication
        m_internal_state = std::make_unique<InternalRenderState>();

        // Capture pointers for callbacks
        auto *camera = m_camera.get();
        auto *scene = m_scene.get();
        auto *asset_system = m_asset_system.get();
        auto *forward_pass = m_forward_pass.get();
        auto *sun = m_sun.get();
        auto *ambient = m_ambient.get();
        auto *internal_state = m_internal_state.get();

        // Graph setup callback: wires the ForwardPass with a FrameDataProvider
        auto graph_setup = [this, forward_pass, asset_system, internal_state](RenderGraph &graph) {
            auto extent = m_renderer->get_swapchain_extent();

            auto provider = [internal_state, asset_system]() -> ForwardPassFrameData {
                {
                    std::lock_guard lock(internal_state->mutex);
                    internal_state->render_thread_copy = internal_state->render_data;
                }
                return {
                    .render_data = &internal_state->render_thread_copy,
                    .meshes = &asset_system->get_meshes()
                };
            };

            forward_pass->setup_graph(graph, extent, provider);
        };

        // Update callback: camera update + scene extraction + user callback
        auto update_fn = [camera, scene, sun, ambient, internal_state, update = std::move(update),
                          this](FrameSnapshot &snapshot, float dt) {
            camera->update(dt);

            // Call user's per-frame logic
            update(*scene, dt);

            auto [fb_w, fb_h] = m_window->get_framebuffer_size();
            auto aspect = (fb_h > 0) ? static_cast<float>(fb_w) / static_cast<float>(fb_h) : 1.0f;

            // Extract render data from the ECS scene
            auto render_data = scene->extract_render_data(*camera, aspect, *sun, *ambient);

            // Write to shared state for the render thread
            {
                std::lock_guard lock(internal_state->mutex);
                internal_state->render_data = render_data;
            }

            // Also write to snapshot
            snapshot.render_data = std::move(render_data);
        };

        // Delegate to the full-control run path
        run(std::move(graph_setup), std::move(update_fn));

        // Cleanup: unregister camera from input
        m_input->remove_subscriber(m_camera.get());
        m_internal_state.reset();
    }

    auto Engine::run() -> void {
        run([](Scene &, float) {});
    }

    // ── Core accessors ──────────────────────────────────────────────────────────

    auto Engine::request_shutdown() -> void {
        m_running = false;
    }

    auto Engine::get_renderer() -> Renderer & {
        return *m_renderer;
    }

    auto Engine::get_render_graph() -> RenderGraph & {
        return *m_render_graph;
    }

    auto Engine::get_window() -> horizon::Window & {
        return *m_window;
    }

    auto Engine::get_input() -> horizon::Input & {
        return *m_input;
    }

    // ── Thread functions ────────────────────────────────────────────────────────

    auto Engine::update_thread_fn(UpdateCallback update) -> void {
        SUB_INFO("Update thread started");

        using clock = std::chrono::high_resolution_clock;
        auto previous_time = clock::now();
        float total_time = 0.0f;
        std::uint32_t write_index = 0;

        while (m_running) {
            auto current_time = clock::now();
            float dt = std::chrono::duration<float>(current_time - previous_time).count();
            previous_time = current_time;
            total_time += dt;

            // Write to the next snapshot slot (triple-buffered, lock-free)
            write_index = (write_index + 1) % SNAPSHOT_COUNT;
            auto &snapshot = m_snapshots[write_index];
            snapshot.delta_time = dt;
            snapshot.total_time = total_time;
            snapshot.frame_number++;

            update(snapshot, dt);

            // Publish the latest snapshot index
            m_latest_snapshot.store(write_index, std::memory_order_release);

            // Sleep briefly to avoid busy spinning (target ~120 ticks/sec)
            std::this_thread::sleep_for(std::chrono::microseconds(8000));
        }

        SUB_INFO("Update thread ended");
    }

    auto Engine::render_thread_fn() -> void {
        SUB_INFO("Render thread started");

        while (m_running) {
            // Read the latest snapshot
            auto snap_index = m_latest_snapshot.load(std::memory_order_acquire);
            const auto &snapshot = m_snapshots[snap_index];

            auto cmd = m_renderer->begin_frame();
            if (!cmd) {
                // Swapchain was recreated or acquire failed - invalidate graph
                // so transient resources are rebuilt at the new extent
                m_render_graph->invalidate();
                continue;
            }

            // If the swapchain was just resized inside begin_frame(), the
            // render graph's transient resources (e.g. depth image) are stale.
            // Force a recompile before using them.
            if (m_renderer->did_resize()) {
                m_render_graph->invalidate();
            }

            // Set the backbuffer for this frame
            m_render_graph->set_backbuffer(
                m_renderer->get_current_image(),
                m_renderer->get_current_image_view(),
                m_renderer->get_swapchain_format(),
                m_renderer->get_swapchain_extent()
            );

            // Recompile the graph if it was invalidated (e.g., by resize)
            if (!m_render_graph->is_compiled()) {
                // Re-invoke graph setup so transient resources get new extents
                m_graph_setup(*m_render_graph);

                if (!m_render_graph->compile()) {
                    SUB_ERROR("Failed to recompile render graph");
                    m_renderer->end_frame();
                    continue;
                }
                SUB_DEBUG("Render graph recompiled after resize");
            }

            m_render_graph->execute(*cmd, m_renderer->get_current_frame_index(), snapshot.delta_time);

            m_renderer->end_frame();
        }

        SUB_INFO("Render thread ended");
    }
} // namespace thresh
