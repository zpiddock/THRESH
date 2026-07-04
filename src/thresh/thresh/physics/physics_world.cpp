//
// Created by Admin on 04/07/2026.
//

#include "physics_world.hpp"

#include "jolt_math.hpp"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "substratum/log.hpp"
#include "thresh/scene/ecs_types.hpp"

namespace thresh {

    namespace {
        auto jolt_trace(const char* fmt, ...) -> void {
            va_list args;
            va_start(args, fmt);
            char buffer[1024];
            vsnprintf(buffer, sizeof buffer, fmt, args);
            va_end(args);
            SUB_INFO("[jolt] {}", buffer);
        }
#ifdef JPH_ENABLE_ASSERTS
        auto jolt_assert_failed(const char* expr, const char* msg, const char* file, JPH::uint line) -> bool {
            SUB_ERROR("[jolt] assert {} ({}) at {}:{}", expr, msg ? msg : "", file, line);
            return true; // break into the debugger
        }
#endif
    }

    PhysicsWorld::PhysicsWorld() {

        static bool registered = false;
        if (!registered) {
            JPH::RegisterDefaultAllocator();
            JPH::Trace = jolt_trace;
            JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = jolt_assert_failed);
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
            registered = true;
        }

        m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        m_job_system = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            std::thread::hardware_concurrency() - 1);

        constexpr JPH::uint cMaxBodies = 65536;
        constexpr JPH::uint cNumBodyMutexes = 0;
        constexpr JPH::uint cMaxBodyPairs = 65536;
        constexpr JPH::uint cMaxContactConstraints = 10240;
        const auto cGravity = JPH::Vec3{0.f, -9.81f, 0.f};

        m_physics = std::make_unique<JPH::PhysicsSystem>();
        m_physics->Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            m_bp_layer_interface,
            m_object_vs_bp_filter,
            m_object_pair_filter);
        // Start with default gravity
        m_physics->SetGravity(cGravity);

        SUB_INFO("PhysicsWorld up: MaxBodies={}, Gravity=[{}, {}, {}]", cMaxBodies, cGravity.GetX(), cGravity.GetY(), cGravity.GetZ());
    }

    auto PhysicsWorld::step(const float timestep) -> void {

        m_physics->Update(timestep, 1, m_temp_allocator.get(), m_job_system.get());
    }

    auto PhysicsWorld::make_shape(flecs::entity entity) -> JPH::ShapeRefC {

        if (const auto* box = entity.try_get<BoxCollider>()) {
            return new JPH::BoxShape(phys::to_jph(box->half_extents));
        }
        if (const auto* sphere = entity.try_get<SphereCollider>()) {
            return new JPH::SphereShape(sphere->radius);
        }
        if (const auto* capsule = entity.try_get<CapsuleCollider>()) {
            return new JPH::CapsuleShape(capsule->half_height, capsule->radius);
        }
        return nullptr;
    }
} // thresh