//
// Created by Admin on 05/07/2026.
//

#include "physics_components.hpp"

#include "jolt_debug_draw.hpp"
#include "jolt_math.hpp"
#include "physics_world.hpp"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "thresh/engine.hpp"
#include "thresh/scene/ecs_types.hpp"

auto thresh::phys::register_systems(flecs::world& world, PhysicsWorld& physics_world) -> void  {

        world.set<PhysicsClock>({});
        world.set<PhysicsDebugDraw>({});

        world.system<const Transform, const RigidBody>("PhysicsBodyCreate")
        .without<PhysicsBody>()
        .kind(flecs::PreUpdate)
        .each([phys = &physics_world] (flecs::entity entity, const Transform& transform, const RigidBody& body) {
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

        world.observer<PhysicsBody>("PhysicsBodyDestroy")
        .event(flecs::OnRemove)
        .each([phys = &physics_world] (flecs::entity entity, PhysicsBody& body) {
            if (body.body_handle == 0xffffffffu) return;
            const JPH::BodyID id(body.body_handle);
            phys->bodies().RemoveBody(id);
            phys->bodies().DestroyBody(id);
        });

        world.system("PhysicsStep")
        .kind(flecs::OnValidate)
        .run([phys = &physics_world] (flecs::iter& it) {
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

        world.system<Transform, const RigidBody, const PhysicsBody>("PhysicsWriteBack")
        .kind(flecs::OnValidate)
        .each([phys = &physics_world] (flecs::entity entity, Transform& transform, const RigidBody& body, const PhysicsBody& physics_body) {

            if (body.motion_type != MotionType::Dynamic) {
                return;
            }
            JPH::RVec3 pos;
            JPH::Quat rot{};
            phys->bodies().GetPositionAndRotation(JPH::BodyID(physics_body.body_handle), pos, rot);
            transform.position = phys::to_helix(pos);
            transform.rotation = phys::to_helix(rot);
        });

    world.system<const Transform, const CharacterController>("CharacterCreate")
    .without<CharacterBody>()
    .kind(flecs::PreUpdate)
    .each([phys = &physics_world] (flecs::entity entity, const Transform& transform, const CharacterController& controller) {
        JPH::CharacterVirtualSettings settings;
        settings.mMaxSlopeAngle = JPH::DegreesToRadians(45.f);
        settings.mShape = JPH::RotatedTranslatedShapeSettings(
            JPH::Vec3(0, 0.5f * controller.height + controller.radius, 0),
            JPH::Quat::sIdentity(),
            new JPH::CapsuleShapeSettings(0.5f * controller.height, controller.radius))
        .Create().Get();
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -controller.radius);

        const auto feet = transform.position + helix::float3{0.f, controller.eye_height, 0.f};
        entity.set<CharacterBody>({
            new JPH::CharacterVirtual(
                &settings,
                phys::to_jph(feet),
                JPH::Quat::sIdentity(),
                &phys->system())
        });
    });

        world.system("PhysicsDebugDraw")
        .kind(flecs::OnStore)
        .run([phys = &physics_world] (flecs::iter& it) {
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