#include "substratum/log.hpp"
#include "thresh/engine.hpp"
#include "thresh/engine_config.hpp"
#include "thresh/renderer.hpp"
#include "thresh/render_graph.hpp"
#include "thresh/camera.hpp"
#include "thresh/scene.hpp"
#include "thresh/components.hpp"
#include "thresh/material_system.hpp"
#include "thresh/forward_pass.hpp"
#include "thresh/render_data.hpp"
#include "flux/device.hpp"
#include "flux/mesh.hpp"
#include "horizon/window.hpp"
#include "horizon/input.hpp"
#include "cube_mesh.hpp"

#include <glm/glm.hpp>

#include <mutex>
#include <memory>
#include <filesystem>
#include <vector>

// Shared state between update and render threads
struct SharedRenderState {
    std::mutex mutex;
    thresh::FrameRenderData render_data;
};

auto main() -> int {
    SUB_INFO("THRESH Demo Game starting");

    substratum::Logger::set_level(substratum::LogLevel::Debug);

    auto engine_cfg = thresh::EngineConfig{
        .application_name = "THRESH Demo Game",
        .window_title = "THRΞSH Demo",
        .window_width = 1280,
        .window_height = 720,
        .window_resizable = true,
        .window_maximized = false,
        .enable_validation = true,
        .enable_shader_hot_reload = true
    };

    auto engine = thresh::Engine(engine_cfg);

    // --- Camera ---
    auto camera = thresh::Camera(engine.get_window());
    camera.set_position({0.0f, 0.0f, 3.0f});
    camera.set_fov(70.0f);
    engine.get_input().add_subscriber(&camera);

    // --- Core systems ---
    auto &device = engine.get_renderer().get_device_ref();

    auto material_system = thresh::MaterialSystem(device);

    auto shader_dir = std::filesystem::current_path() / "assets" / "shaders";
    auto forward_pass = thresh::ForwardPass({
        .device = &device,
        .material_system = &material_system,
        .shader_dir = shader_dir
    });

    // --- Mesh ---
    auto meshes = std::vector<std::unique_ptr<flux::Mesh>>();
    meshes.push_back(demo::create_cube_mesh(device));

    // --- Scene ---
    auto scene = thresh::Scene();
    auto sun = thresh::DirectionalLight{};

    // Create a PBR cube entity with the default material (index 0)
    {
        auto cube = scene.create_entity("PBR Cube");
        auto &transform = scene.add_component<thresh::TransformComponent>(cube);
        transform.position = {0.0f, 0.0f, 0.0f};
        transform.scale = {1.0f, 1.0f, 1.0f};

        auto &mesh_comp = scene.add_component<thresh::MeshComponent>(cube);
        mesh_comp.mesh_index = 0;

        auto &mat_comp = scene.add_component<thresh::MaterialComponent>(cube);
        mat_comp.material_handle = 0; // Default material
    }

    // Create a second cube to test multiple objects
    {
        auto cube2 = scene.create_entity("Cyan Cube");
        auto &transform = scene.add_component<thresh::TransformComponent>(cube2);
        transform.position = {2.5f, 0.0f, 0.0f};
        transform.scale = {0.75f, 0.75f, 0.75f};

        auto &mesh_comp = scene.add_component<thresh::MeshComponent>(cube2);
        mesh_comp.mesh_index = 0;

        // Create a cyan-ish metallic material
        auto cyan_mat = thresh::GpuMaterialData{};
        cyan_mat.base_color_factor = {0.0f, 0.941f, 1.0f, 1.0f}; // THRESH Cyan
        cyan_mat.metallic_factor = 0.9f;
        cyan_mat.roughness_factor = 0.2f;
        auto cyan_handle = material_system.create_material(cyan_mat);

        auto &mat_comp = scene.add_component<thresh::MaterialComponent>(cube2);
        mat_comp.material_handle = cyan_handle;
    }

    // Create a third cube — magenta, rough
    {
        auto cube3 = scene.create_entity("Magenta Cube");
        auto &transform = scene.add_component<thresh::TransformComponent>(cube3);
        transform.position = {-2.5f, 0.0f, 0.0f};
        transform.scale = {0.75f, 0.75f, 0.75f};

        auto &mesh_comp = scene.add_component<thresh::MeshComponent>(cube3);
        mesh_comp.mesh_index = 0;

        auto magenta_mat = thresh::GpuMaterialData{};
        magenta_mat.base_color_factor = {1.0f, 0.176f, 0.416f, 1.0f}; // THRESH Magenta
        magenta_mat.metallic_factor = 0.1f;
        magenta_mat.roughness_factor = 0.8f;
        auto magenta_handle = material_system.create_material(magenta_mat);

        auto &mat_comp = scene.add_component<thresh::MaterialComponent>(cube3);
        mat_comp.material_handle = magenta_handle;
    }

    // --- Shared render state ---
    auto shared_state = std::make_shared<SharedRenderState>();

    // Local copy of render data for the render thread to read without contention
    auto render_thread_data = std::make_shared<thresh::FrameRenderData>();

    engine.run(
        // --- Graph setup callback (called on initial build + resize) ---
        [&engine, &forward_pass, &meshes, shared_state, render_thread_data](thresh::RenderGraph &graph) {
            auto extent = engine.get_renderer().get_swapchain_extent();

            // Frame data provider: called each frame from the render thread
            // Copies shared render data under the lock, returns pointers
            auto provider = [shared_state, render_thread_data, &meshes]() -> thresh::ForwardPassFrameData {
                {
                    std::lock_guard lock(shared_state->mutex);
                    *render_thread_data = shared_state->render_data;
                }
                return {
                    .render_data = render_thread_data.get(),
                    .meshes = &meshes
                };
            };

            forward_pass.setup_graph(graph, extent, provider);
        },
        // --- Update callback (called each tick on update thread) ---
        [&camera, &engine, &scene, &sun, shared_state](thresh::FrameSnapshot &snapshot, float dt) {
            camera.update(dt);

            auto [fb_w, fb_h] = engine.get_window().get_framebuffer_size();
            auto aspect = (fb_h > 0) ? static_cast<float>(fb_w) / static_cast<float>(fb_h) : 1.0f;

            // Extract render data from the ECS scene
            auto render_data = scene.extract_render_data(camera, aspect, sun);

            // Write to shared state for the render thread
            {
                std::lock_guard lock(shared_state->mutex);
                shared_state->render_data = render_data;
            }

            // Also write to snapshot (for any systems that use it)
            snapshot.render_data = std::move(render_data);
        }
    );

    engine.get_input().remove_subscriber(&camera);

    SUB_INFO("THRESH Demo Game exited cleanly");
    return 0;
}
