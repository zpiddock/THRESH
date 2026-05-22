//
// Created by Admin on 03/05/2026.
//

#include "scene.hpp"

#include "ecs_types.hpp"
#include "scene_serializer.hpp"
#include "substratum/log.hpp"
#include "thresh/engine.hpp"

namespace thresh {
    Scene::Scene() {

        SUB_DEBUG("Scene created");
        // m_world.import<flecs::stats>();
        // m_world.set<flecs::Rest>({});

        init();
    }

    Scene::~Scene() {
        SUB_DEBUG("Scene destroyed");
    }

    auto Scene::init() -> void {
        SUB_TRACE("Registering ECS systems");

        SceneSerializer::register_components(m_world);

        m_scene_root = m_world.entity("SceneRoot").add<SceneRoot>();

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

        m_world.system<const WorldTransform, const Mesh>("MeshRender").kind(flecs::OnStore).each(
            [](flecs::iter& it, size_t, const WorldTransform& transform, const Mesh& mesh) {

                auto* graphics = Engine::get_instance().graphics();

                const auto* material = graphics->get_material_resource(mesh.material_handle);

                if (!material) {
                    return;
                }

                graphics->submit_draw_command({
                    .model           = transform.transform,
                    .base_colour     = material->albedo_tint,
                    .mesh_handle     = mesh.handle,
                    .material_handle = mesh.material_handle,
                });
            }
        );

        m_world.system("PropogateWorldTransform").kind(flecs::PostUpdate)
        .run([this](flecs::iter& it) {
            auto walk  = [] (this auto& self, flecs::entity e, const flux::float4x4& parent_world) -> void {
                flux::float4x4 world = parent_world;
                if (const auto* t = e.try_get<Transform>()) {

                    world = parent_world * flux::math::compose_local(*t);
                    e.set<WorldTransform>({world});
                }
                e.children([&](flecs::entity child) {
                    self(child, world);
                });
            };

            walk(m_scene_root, flux::float4x4{1.f});
        });

        m_world.system("ComputeWorldAABB").kind(flecs::PostUpdate)
        .run([this](flecs::iter& it) {

            auto* gfx = Engine::get_instance().graphics();
            auto fold = [&](this auto& self, flecs::entity e) -> flux::AABB {
                flux::AABB aabb{};
                if (const auto* mesh = e.try_get<Mesh>()) {
                    if (const auto* mesh_handle = gfx->get_mesh_resource(mesh->handle)) {
                        if (const auto* world_transform = e.try_get<WorldTransform>()) {
                            aabb.expand(flux::transform_aabb(mesh_handle->local_aabb, world_transform->transform));
                        }
                    }
                }
                e.children([&](flecs::entity child) {
                    aabb.expand(self(child));
                });

                if (aabb.valid()) {
                    e.set<WorldAABB>({aabb});
                } else if (e.has<WorldAABB>()) {
                    e.remove<WorldAABB>();
                }
                return aabb;
            };

            fold(m_scene_root);
        });
    }

    auto Scene::update(const float delta_time) -> void {

        if (m_world.progress(delta_time)) {

        }
    }

    auto Scene::compute_active_camera_data(const float aspect) -> std::optional<flux::CameraData> {

        std::optional<flux::CameraData> result;

        const auto camera_query = m_world.query_builder<WorldTransform, Camera>().with<ActiveCamera>().build();

        camera_query.each([&](flecs::entity entity, const WorldTransform& transform, const Camera& camera) {

            if (result) return; // First result wins

            flux::CameraData data{};
            constexpr auto identity = flux::float4x4{1.f};

            data.view = flux::math::inverse(transform.transform);
            data.projection = flux::math::perspective(camera.fov, aspect, camera.near_plane, camera.far_plane);

            result = data;
        });

        return result;
    }

    auto Scene::get_world() -> flecs::world& {
        return m_world;
    }

    auto Scene::get_or_create_entity(const std::string& name) -> flecs::entity {
        auto prev = m_world.set_scope(m_scene_root);
        auto entity = m_world.entity(name.c_str());
        m_world.set_scope(prev);
        return entity;
    }

    auto Scene::get_or_create_entity() -> flecs::entity {
        auto prev = m_world.set_scope(m_scene_root);
        auto entity = m_world.entity();
        m_world.set_scope(prev);
        return entity;
    }

    auto Scene::root() -> flecs::entity {

        return m_scene_root;
    }

    auto Scene::pick_entity(int mouse_x, int mouse_y, int viewport_w, int viewport_h) -> std::optional<PickHit> {


    }
} // thresh