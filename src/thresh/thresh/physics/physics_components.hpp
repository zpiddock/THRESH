#pragma once

#include "physics_world.hpp"
#include "thresh/thresh.hpp"
#include "helix/math.hpp"
#include "Jolt/Core/Reference.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"

enum class MotionType {
    Static,
    Kinematic,
    Dynamic
};

struct RigidBody {
    MotionType motion_type = MotionType::Static;
    float mass = 1.f;
    float friction = 0.5f;
    float restitution = 0.f;
};

struct BoxCollider {
    helix::float3 half_extents{0.5f};
};
struct SphereCollider {
    float radius = 0.5f;
};
struct CapsuleCollider {
    float radius = 0.3f;
    float half_height = 0.5f;
};

struct CharacterController {
    float radius = 0.3f;
    float height = 1.35f;
    float eye_height = 1.6f;
    float move_speed = 5.f;
    float jump_speed = 6.f;

    bool freecam = false;
};

struct PhysicsBody {
    std::uint32_t body_handle = 0xffffffffu;
};

struct PhysicsClock {
    float accumulator = 0.f;
};

struct PhysicsDebugDraw {
    bool enabled = false;
};

struct CharacterBody {
    JPH::Ref<JPH::CharacterVirtual> character;
};

namespace thresh::phys {

    auto register_systems(flecs::world& world, PhysicsWorld& physics_world) -> void;
}
