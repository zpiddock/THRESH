#include "substratum/log.hpp"
#include "thresh/engine.hpp"
#include "thresh/engine_config.hpp"
#include "thresh/renderer.hpp"
#include "thresh/camera.hpp"
#include "thresh/scene.hpp"
#include "thresh/components.hpp"
#include "thresh/material_system.hpp"
#include "thresh/asset_system.hpp"
#include "thresh/render_data.hpp"
#include "flux/device.hpp"
#include "cube_mesh.hpp"

auto main() -> int {
    SUB_INFO("THRESH Demo Game starting");

    substratum::Logger::set_level(substratum::LogLevel::Debug);

    auto engine = thresh::Engine({
        .application_name = "THRESH Demo Game",
        .window_title = "THRΞSH Demo",
        .window_width = 1280,
        .window_height = 720,
        .window_resizable = true,
        .window_maximized = false,
        .enable_validation = true,
        .enable_shader_hot_reload = true
    });

    // --- Camera ---
    engine.get_camera().set_position({0.0f, 0.0f, 3.0f});
    engine.get_camera().set_fov(70.0f);

    // --- Mesh ---
    auto &device = engine.get_renderer().get_device_ref();
    auto &assets = engine.get_asset_system();

    // --- Materials ---
    auto &mat_sys = engine.get_material_system();


    // --- Scene ---
    auto &scene = engine.get_scene();
    assets.load_gltf_scene("assets/models/CarConcept.glb", scene, mat_sys);

    // --- Lighting ---
    // Key light: bright white, upper-right
    auto key = scene.create_entity("Key Light");
    scene.add_component<thresh::TransformComponent>(key).position = {2.0f, 3.0f, 2.0f};
    auto &key_pl = scene.add_component<thresh::PointLightComponent>(key);
    key_pl.color = {1.0f, 0.95f, 0.9f};
    key_pl.intensity = 25.0f;
    key_pl.radius = 25.0f;

    // Fill light: cooler blue-ish, lower-left (fills in shadows)
    auto fill = scene.create_entity("Fill Light");
    scene.add_component<thresh::TransformComponent>(fill).position = {-3.0f, -1.0f, 3.0f};
    auto &fill_pl = scene.add_component<thresh::PointLightComponent>(fill);
    fill_pl.color = {0.7f, 0.85f, 1.0f};
    fill_pl.intensity = 12.0f;
    fill_pl.radius = 25.0f;

    // Rim/back light: warm accent from behind
    auto rim = scene.create_entity("Rim Light");
    scene.add_component<thresh::TransformComponent>(rim).position = {-1.0f, 2.0f, -3.0f};
    auto &rim_pl = scene.add_component<thresh::PointLightComponent>(rim);
    rim_pl.color = {1.0f, 0.8f, 0.6f};
    rim_pl.intensity = 15.0f;
    rim_pl.radius = 25.0f;

    // --- Run ---
    engine.run(); // Default render graph + camera + rendering handled internally

    SUB_INFO("THRESH Demo Game exited cleanly");
    return 0;
}
