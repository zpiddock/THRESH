#include "thresh/engine.hpp"
#include "thresh/renderer.hpp"
#include "thresh/render_graph.hpp"
#include "horizon/window.hpp"
#include "horizon/input.hpp"
#include "substratum/file_watcher.hpp"
#include "substratum/log.hpp"

#include <chrono>
#include <stdexcept>
#include <filesystem>

namespace thresh {
    Engine::Engine(const EngineConfig &config)
        : m_config(config) {
        SUB_INFO("Initializing THRESH Engine");

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
            auto shader_dir = std::filesystem::current_path() / "shaders";
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

        // unique_ptrs destroy in reverse member order
        SUB_INFO("THRESH Engine shutdown complete");
    }

    auto Engine::run(GraphSetupCallback graph_setup, UpdateCallback update) -> void {
        SUB_INFO("Starting engine loop");

        // Let the user define their render graph
        graph_setup(*m_render_graph);

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
                // Swapchain was recreated - skip this frame
                continue;
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
                if (!m_render_graph->compile()) {
                    SUB_ERROR("Failed to recompile render graph");
                    m_renderer->end_frame();
                    continue;
                }
            }

            m_render_graph->execute(*cmd, m_renderer->get_current_frame_index(), snapshot.delta_time);

            m_renderer->end_frame();
        }

        SUB_INFO("Render thread ended");
    }
} // namespace thresh
