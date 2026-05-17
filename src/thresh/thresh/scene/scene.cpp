//
// Created by Admin on 03/05/2026.
//

#include "scene.hpp"

#include "ecs_types.hpp"
#include "substratum/log.hpp"
#include "thresh/engine.hpp"

namespace thresh {
    Scene::Scene() {

        m_world.set<flecs::Rest>({});

        init();
    }

    Scene::~Scene() {
    }

    auto Scene::init() -> void {

        m_world.system<Transform, CameraController, ActiveCamera>().kind(flecs::OnUpdate).each(
                [](flecs::iter& it, size_t, Transform& transform, CameraController& camera_controller, const ActiveCamera& active_camera) {

                    if (!camera_controller.movement_allowed) { return; }

                    const auto input = Engine::get_instance().input();
                    const auto delta_time = it.delta_time();

                    const auto [dx, dy] = input->get_relative_mouse_state();

                    camera_controller.yaw -= flux::math::radians(dx * camera_controller.mouse_sensitivity);
                    camera_controller.pitch -= flux::math::radians(dy * camera_controller.mouse_sensitivity);

                    camera_controller.pitch = flux::math::clamp(camera_controller.pitch, flux::math::radians(-89.f), flux::math::radians(89.f));

                    const auto q_yaw = flux::math::angleAxis(camera_controller.yaw, flux::float3(0.f, 1.f, 0.f));
                    const auto q_pitch = flux::math::angleAxis(camera_controller.pitch, flux::float3(1.f, 0.f, 0.f));

                    transform.rotation = q_yaw * q_pitch;

                    // movement vectors
                    const auto forward = flux::float3{-flux::math::sin(camera_controller.yaw), 0.f, -flux::math::cos(camera_controller.yaw)};
                    const auto right = flux::float3{flux::math::cos(camera_controller.yaw), 0.f, -flux::math::sin(camera_controller.yaw)};
                    constexpr auto world_up = flux::float3{0.f, 1.f, 0.f};

                    flux::float3 wish{0.f};

                    if (input->is_key_held(SDL_SCANCODE_W)) {

                        wish += forward;
                    }
                    if (input->is_key_held(SDL_SCANCODE_S)) {
                        wish -= forward;
                    }
                    if (input->is_key_held(SDL_SCANCODE_A)) {
                        wish -= right;
                    }
                    if (input->is_key_held(SDL_SCANCODE_D)) {
                        wish += right;
                    }
                    if (input->is_key_held(SDL_SCANCODE_SPACE)) {
                        wish += world_up;
                    }
                    if (input->is_key_held(SDL_SCANCODE_LSHIFT)) {
                        wish -= world_up;
                    }

                    if (flux::math::length(wish) > 0.0001f) {
                        wish = flux::math::normalize(wish);
                        transform.position += wish * camera_controller.movement_speed * delta_time;
                    }
                }
        );

        m_world.system<const Transform, const Mesh>().kind(flecs::OnUpdate).each(
            [](flecs::iter& it, size_t, const Transform& transform, const Mesh& mesh) {

                auto* graphics = Engine::get_instance().graphics();

                const auto* material = graphics->get_material_resource(mesh.material_handle);

                if (!material) {
                    return;
                }

                constexpr auto identity = flux::float4x4{1.f};
                const auto model = flux::math::translate(identity, transform.position)
                                                * flux::math::mat4_cast(transform.rotation)
                                                * flux::math::scale(identity, transform.scale);


                graphics->submit_draw_command({
                    .model           = model,
                    .base_colour     = material->albedo_tint,
                    .mesh_handle     = mesh.handle,
                    .material_handle = mesh.material_handle,
                });
            }
        );
    }

    auto Scene::update(const float delta_time) -> void {

        if (m_world.progress(delta_time)) {

        }
    }

    auto Scene::compute_active_camera_data(const float aspect) -> std::optional<flux::CameraData> {

        std::optional<flux::CameraData> result;

        const auto camera_query = m_world.query_builder<Transform, Camera>().with<ActiveCamera>().build();

        camera_query.each([&](flecs::entity entity, const Transform& transform, const Camera& camera) {

            if (result) return; // First result wins

            flux::CameraData data{};
            constexpr auto identity = flux::float4x4{1.f};

            data.view = flux::math::inverse(flux::math::translate(identity, transform.position) * flux::math::mat4_cast(transform.rotation));
            data.projection = flux::math::perspective(camera.fov, aspect, camera.near_plane, camera.far_plane);

            data.projection[1][1] *= -1; // Y Flip
            result = data;
        });

        return result;
    }

    auto Scene::get_world() -> flecs::world& {
        return m_world;
    }

    auto Scene::get_or_create_entity(const std::string& name) -> flecs::entity {
        return m_world.entity(name.c_str());
    }

    auto Scene::get_or_create_entity() -> flecs::entity {
        return m_world.entity();
    }
} // thresh