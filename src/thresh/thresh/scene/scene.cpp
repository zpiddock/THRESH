//
// Created by Admin on 03/05/2026.
//

#include "scene.hpp"

#include "thresh/thresh.hpp"
#include "ecs_types.hpp"
#include "scene_serializer.hpp"
#include "substratum/log.hpp"
#include "thresh/engine.hpp"

#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "thresh/physics/jolt_debug_draw.hpp"
#include "thresh/physics/jolt_math.hpp"

namespace thresh {
    Scene::Scene() {

        SUB_DEBUG("Scene created");
        m_world.import<flecs::stats>();
        m_world.set<flecs::Rest>({});

        // Physics World
        m_physics_world = std::make_unique<PhysicsWorld>();
        JPH::VerifyJoltVersionID();

        // Flecs Systems
        init();
    }

    Scene::~Scene() {
        SUB_DEBUG("Scene destroyed");
    }

    auto Scene::init() -> void {
        SUB_TRACE("Registering ECS systems");

        SceneSerializer::register_components(m_world);

        m_scene_root = m_world.entity("SceneRoot").add<SceneRoot>();
        m_scene_root.set<AmbientLight>({});

        m_world.system<Transform, CameraController, ActiveCamera>().kind(flecs::OnUpdate).each(
                [](flecs::iter& it, size_t, Transform& transform, CameraController& camera_controller, const ActiveCamera& active_camera) {

                    if (!camera_controller.movement_allowed) { return; }

                    const auto input = Engine::get_instance().input();
                    const auto delta_time = it.delta_time();

                    const auto [dx, dy] = input->get_relative_mouse_state();

                    camera_controller.yaw -= helix::radians(dx * camera_controller.mouse_sensitivity);
                    camera_controller.pitch -= helix::radians(dy * camera_controller.mouse_sensitivity);

                    camera_controller.pitch = helix::clamp(camera_controller.pitch, helix::radians(-89.f), helix::radians(89.f));

                    const auto q_yaw = helix::angleAxis(camera_controller.yaw, helix::float3(0.f, 1.f, 0.f));
                    const auto q_pitch = helix::angleAxis(camera_controller.pitch, helix::float3(1.f, 0.f, 0.f));

                    transform.rotation = q_yaw * q_pitch;

                    // movement vectors
                    const auto forward = helix::float3{-helix::sin(camera_controller.yaw), 0.f, -helix::cos(camera_controller.yaw)};
                    const auto right = helix::float3{helix::cos(camera_controller.yaw), 0.f, -helix::sin(camera_controller.yaw)};
                    constexpr auto world_up = helix::float3{0.f, 1.f, 0.f};

                    helix::float3 wish{0.f};

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

                    if (helix::length(wish) > 0.0001f) {
                        wish = helix::normalize(wish);
                        transform.position += wish * camera_controller.movement_speed * delta_time;
                    }
                }
        );

        m_world.system<const WorldTransform, const Mesh>("MeshRender").kind(flecs::OnStore).each(
            [](flecs::iter& it, size_t, const WorldTransform& transform, const Mesh& mesh) {

                auto* graphics = Engine::get_instance().graphics();

                const auto* material = graphics->resources().get_material_resource(mesh.material_handle);

                if (!material) {
                    return;
                }

                graphics->submit_draw_command({
                    .model           = transform.transform,
                    .base_colour     = helix::float4(1.f),
                    .mesh_handle     = mesh.handle,
                    .material_handle = mesh.material_handle,
                    .index_offset    = mesh.index_offset,
                    .index_count     = mesh.index_count,
                });
            }
        );

        m_world.system<const WorldTransform, const MeshRenderer>("ModelRender").kind(flecs::OnStore).each(
            [](flecs::iter&, size_t, const WorldTransform& transform, const MeshRenderer& renderer) {

                auto* graphics = Engine::get_instance().graphics();

                for (const auto& sm : renderer.submeshes) {
                    if (!graphics->resources().get_material_resource(sm.material_handle)) {
                        continue; // same unresolved-material guard as MeshRender
                    }
                    graphics->submit_draw_command({
                        .model           = transform.transform * sm.local, // CPU compose — flux untouched
                        .base_colour     = helix::float4(1.f),
                        .mesh_handle     = renderer.mesh_handle,
                        .material_handle = sm.material_handle,
                        .index_offset    = sm.index_offset,
                        .index_count     = sm.index_count,
                    });
                }
            }
        );

        m_world.system("PropogateWorldTransform").kind(flecs::PostUpdate)
        .run([this](flecs::iter& it) {
            auto walk  = [] (this auto& self, flecs::entity e, const helix::float4x4& parent_world) -> void {
                helix::float4x4 world = parent_world;
                if (const auto* t = e.try_get<Transform>()) {

                    world = parent_world * flux::math::compose_local(*t);
                    e.set<WorldTransform>({world});
                }
                e.children([&](flecs::entity child) {
                    self(child, world);
                });
            };

            walk(m_scene_root, helix::float4x4{1.f});
        });

        m_world.system("ComputeWorldAABB").kind(flecs::PostUpdate)
        .run([this](flecs::iter& it) {

            auto* gfx = Engine::get_instance().graphics();
            auto fold = [&](this auto& self, flecs::entity e) -> helix::AABB {
                helix::AABB aabb{};
                if (const auto* mesh = e.try_get<Mesh>()) {
                    if (const auto* mesh_handle = gfx->resources().get_mesh_resource(mesh->handle)) {
                        if (const auto* world_transform = e.try_get<WorldTransform>()) {
                            aabb.expand(helix::transform_aabb(mesh_handle->local_aabb, world_transform->transform));
                        }
                    }
                }
                if (const auto* renderer = e.try_get<MeshRenderer>()) {
                    // The cook stored the whole-model AABB on the merged mesh, so one lookup covers
                    // every submesh.
                    if (const auto* mesh_handle = gfx->resources().get_mesh_resource(renderer->mesh_handle)) {
                        if (const auto* world_transform = e.try_get<WorldTransform>()) {
                            aabb.expand(helix::transform_aabb(mesh_handle->local_aabb, world_transform->transform));
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

        m_world.set<PhysicsClock>({});
        m_world.set<PhysicsDebugDraw>({});

        m_world.system<const Transform, const RigidBody>("PhysicsBodyCreate")
        .without<PhysicsBody>()
        .kind(flecs::PreUpdate)
        .each([phys = m_physics_world.get()] (flecs::entity entity, const Transform& transform, const RigidBody& body) {
            const auto shape = phys->make_shape(entity);

            if (!shape) {
                return;
            }

            const bool dynamic = body.motion_type == MotionType::Dynamic;
            JPH::BodyCreationSettings settings(
                shape,
                phys::to_jph(transform.position),
                phys::to_jph(transform.rotation),
                body.motion_type == MotionType::Static    ? JPH::EMotionType::Static
                : body.motion_type == MotionType::Kinematic ? JPH::EMotionType::Kinematic
                : JPH::EMotionType::Dynamic,
                body.motion_type == MotionType::Static ? phys::layers::NON_MOVING : phys::layers::MOVING
            );
            settings.mFriction = body.friction;
            settings.mRestitution = body.restitution;
            if (dynamic) {
                settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                settings.mMassPropertiesOverride.mMass = body.mass;
            }

            const JPH::BodyID id = phys->bodies().CreateAndAddBody(
                settings,
                dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate
                );
            entity.set<PhysicsBody>({id.GetIndexAndSequenceNumber()});
        });

        m_world.observer<PhysicsBody>("PhysicsBodyDestroy")
        .event(flecs::OnRemove)
        .each([phys = m_physics_world.get()] (flecs::entity entity, PhysicsBody& body) {
            if (body.body_handle == 0xffffffffu) return;
            const JPH::BodyID id(body.body_handle);
            phys->bodies().RemoveBody(id);
            phys->bodies().DestroyBody(id);
        });

        m_world.system("PhysicsStep")
        .kind(flecs::OnValidate)
        .run([phys = m_physics_world.get()] (flecs::iter& it) {
            constexpr float STEP = 1.f / 60.f;
            auto& clock = it.world().ensure<PhysicsClock>();

            // A hitch or pause must not trigger a catch up death spiral
            clock.accumulator = std::min(clock.accumulator + it.world().delta_time(), 0.25f);

            auto kinematics = it.world().query<const Transform, const RigidBody, const PhysicsBody>();
            while (clock.accumulator >= STEP) {
                kinematics.each([&](flecs::entity e, const Transform& transform, const RigidBody& body, const PhysicsBody& physics_body) {
                    if (body.motion_type != MotionType::Kinematic) {
                        return;
                    }
                    phys->bodies().MoveKinematic(
                        JPH::BodyID(physics_body.body_handle),
                        phys::to_jph(transform.position),
                        phys::to_jph(transform.rotation),
                        STEP
                    );
                });
                phys->step(STEP);
                clock.accumulator -= STEP;
            }
        });

        m_world.system<Transform, const RigidBody, const PhysicsBody>("PhysicsWriteBack")
        .kind(flecs::OnValidate)
        .each([phys = m_physics_world.get()] (flecs::entity entity, Transform& transform, const RigidBody& body, const PhysicsBody& physics_body) {

            if (body.motion_type != MotionType::Dynamic) {
                return;
            }
            JPH::RVec3 pos;
            JPH::Quat rot{};
            phys->bodies().GetPositionAndRotation(JPH::BodyID(physics_body.body_handle), pos, rot);
            transform.position = phys::to_helix(pos);
            transform.rotation = phys::to_helix(rot);
        });

        m_world.system("PhysicsDebugDraw")
        .kind(flecs::OnStore)
        .run([phys = m_physics_world.get()] (flecs::iter& it) {
#ifdef JPH_DEBUG_RENDERER
            auto& debug_draw = it.world().ensure<PhysicsDebugDraw>();
            if (Engine::get_instance().input()->key_just_pressed(SDL_SCANCODE_F3)) {
                debug_draw.enabled = !debug_draw.enabled;
            }
            if (!debug_draw.enabled) { return; }

            static phys::JoltDebugDraw s_draw;

            JPH::BodyManager::DrawSettings settings;
            settings.mDrawShapeWireframe = true;

            phys->system().DrawBodies(settings, &s_draw);
#endif
        });
    }

    auto Scene::update(const float delta_time) -> void {

        if (m_world.progress(delta_time)) {

        }
    }

    auto Scene::compute_active_camera_data(const float aspect) -> std::optional<flux::gpu::CameraData> {

        std::optional<flux::gpu::CameraData> result = std::nullopt;

        const auto camera_query = m_world.query_builder<WorldTransform, Camera>().with<ActiveCamera>().build();

        camera_query.each([&](flecs::entity entity, const WorldTransform& transform, const Camera& camera) {

            if (result) return; // First result wins

            flux::gpu::CameraData data{};

            data.view = helix::inverse(transform.transform);
            data.projection = helix::perspective(camera.fov, aspect, camera.near_plane, camera.far_plane);

            result = data;
        });

        return result;
    }

    auto Scene::compute_active_light_data() -> std::optional<flux::gpu::LightData> {
        auto result = flux::gpu::LightData{};

        bool ambient_light_found = false;
        m_world.query_builder<const AmbientLight>().build().each([&](flecs::entity entity, const AmbientLight& light) {
            if (ambient_light_found) {
                SUB_WARN("Multiple ambient lights found, ignoring all but the first");
                return;
            }
            result.ambient = helix::float4(light.colour, light.intensity);
            ambient_light_found = true;
        });

        int light_count = 0;
        m_world.query_builder<const WorldTransform, const Light>().build()
        .each([&](flecs::entity entity, const WorldTransform& transform, const Light& light) {
            if (light_count >= flux::gpu::MAX_POINT_LIGHTS) {
                SUB_WARN("Too many point lights, ignoring all unregistered lights");
                return;
            }
            result.lights[light_count].position = transform.transform[3];
            result.lights[light_count].colour = helix::float4(light.colour, light.intensity);
            light_count++;
        });
        result.num_lights = light_count;

        return result;
    }

    auto Scene::world() -> flecs::world& {
        return m_world;
    }

    auto Scene::physics() -> PhysicsWorld& {
        return *m_physics_world;
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

        if (viewport_w <= 0 || viewport_h <= 0) {
            return std::nullopt;
        }

        const float aspect = static_cast<float>(viewport_w) / static_cast<float>(viewport_h);

        std::optional<helix::float4x4> view_opt, proj_opt;
        std::optional<helix::float3> camera_pos_opt;

        m_world.query_builder<const WorldTransform, const Camera>()
        .with<ActiveCamera>()
        .each([&](flecs::entity e, const WorldTransform& transform, const Camera& camera) {
            if (view_opt) return;
            view_opt = helix::inverse(transform.transform);
            auto proj = helix::perspective(camera.fov, aspect, camera.near_plane, camera.far_plane);
            proj_opt = proj;
            // Optional Y flip here if needed
            camera_pos_opt = helix::float3{transform.transform[3]};
        });

        if (!view_opt || !proj_opt || !camera_pos_opt) {
            return std::nullopt;
        }

        // NDC from pixel. Window coords: y-down. NDC: y-up after Vulkan Y-flip,
        // BUT since we already baked the Y-flip into `proj_opt`, we undo it here:
        // pixel y=0 (top) -> ndc y=+1, pixel y=h (bottom) -> ndc y=-1.
        const float ndc_x = (2.0f * static_cast<float>(mouse_x) / static_cast<float>(viewport_w)) - 1.0f;
        const float ndc_y = 1.0f - (2.0f * static_cast<float>(mouse_y) / static_cast<float>(viewport_h));

        const auto inv_vp = helix::inverse(*proj_opt * *view_opt);
        auto unproject = [&](float ndc_z) {
            const helix::float4 p = inv_vp * helix::float4{ndc_x, ndc_y, ndc_z, 1.0f};
            return helix::float3{p} / p.w;
        };
        const helix::float3 world_near = unproject(0.0f);  // Vulkan NDC z in [0,1]
        const helix::float3 world_far  = unproject(1.0f);
        const helix::float3 ray_dir    = helix::normalize(world_far - world_near);
        const helix::float3 ray_origin = *camera_pos_opt;

        // Test against every entity with a WorldAABB. For each, also need the entity itself.
        std::optional<PickHit> best;
        m_world.query_builder<const WorldAABB>()
        .with<Mesh>().or_()
        .with<MeshRenderer>()
        .build().each(
            [&](flecs::entity e, const WorldAABB& wa) {
                const auto broad = helix::ray_aabb_intersect(ray_origin, ray_dir, wa.aabb);
                if (!broad) return;

                // MeshRenderer entities refine to the closest submesh box; the hit carries its index.
                if (const auto* renderer = e.try_get<MeshRenderer>()) {
                    const auto* world_transform = e.try_get<WorldTransform>();
                    if (!world_transform) return;
                    for (std::size_t i = 0; i < renderer->submeshes.size(); ++i) {
                        const auto& sm = renderer->submeshes[i];
                        const auto box = helix::transform_aabb(sm.local_aabb,
                                                               world_transform->transform * sm.local);
                        const auto hit = helix::ray_aabb_intersect(ray_origin, ray_dir, box);
                        if (!hit) continue;
                        if (!best || *hit < best->flags) {
                            best = PickHit{e, *hit, static_cast<std::int32_t>(i)};
                        }
                    }
                    return;
                }

                if (!best || *broad < best->flags) {
                    best = PickHit{e, *broad};
                }
            });
        return best;
    }
} // thresh