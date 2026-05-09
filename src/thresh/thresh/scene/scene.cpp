//
// Created by Admin on 03/05/2026.
//

#include "scene.hpp"

#include "ecs_types.hpp"
#include "substratum/log.hpp"
#include "thresh/engine.hpp"

namespace thresh {
    Scene::Scene() {

        init();
    }

    Scene::~Scene() {
    }

    auto Scene::init() -> void {

        m_world.system<Transform, CameraController, ActiveCamera>().kind(flecs::OnUpdate).each(
                [](flecs::iter& it, size_t, Transform& transform, CameraController& camera_controller, const ActiveCamera& active_camera) {

                    auto input = Engine::get_instance().input();

                    if (input->is_key_held(SDL_SCANCODE_W)) {

                        transform.position += camera_controller.movement_speed;
                    }
                }
        );
    }

    auto Scene::update(const float delta_time) -> void {

        if (m_world.progress(delta_time)) {

        }
    }

    auto Scene::compute_active_camera_data(const float aspect) -> std::optional<flux::CameraData> {

        flux::CameraData data = {};

        const auto camera_query = m_world.query_builder<Transform, Camera>().with<ActiveCamera>().build();

        camera_query.each([&](flecs::entity entity, const Transform& transform, const Camera& camera) {
            constexpr auto identity = flux::float4x4{1.f};

            data.view = flux::math::inverse(flux::math::translate(identity, transform.position) * flux::math::mat4_cast(transform.rotation));
            data.projection = flux::math::perspective(camera.fov, aspect, camera.near_plane, camera.far_plane);

            data.projection[1][1] *= -1; // Y Flip
        });

        return data;
    }

    auto Scene::get_world() -> flecs::world& {
        return m_world;
    }

    auto Scene::create_entity(const std::string& name) -> flecs::entity {
        return m_world.entity(name.c_str());
    }

    auto Scene::create_entity() -> flecs::entity {
        return m_world.entity();
    }
} // thresh