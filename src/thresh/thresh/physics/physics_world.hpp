//
// Created by Admin on 04/07/2026.
//

#pragma once
#include <memory>
#include "thresh/thresh.hpp"

#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "physics_layers.hpp"

namespace thresh {
    class PhysicsWorld {

        public:
            PhysicsWorld();
            ~PhysicsWorld() = default;

            PhysicsWorld(const PhysicsWorld&) = delete;
            auto operator=(const PhysicsWorld&) -> PhysicsWorld& = delete;

            // Fixed simulation timestep, e.g 60hz (~16.6ms)
            auto step(float timestep) -> void;

            [[nodiscard]] auto bodies() const -> JPH::BodyInterface& {
                return m_physics->GetBodyInterface();
            }

            [[nodiscard]] auto system() const -> JPH::PhysicsSystem& {
                return *m_physics;
            }

            [[nodiscard]] auto temp_allocator() const -> JPH::TempAllocator& {
                return *m_temp_allocator;
            }

        private:

            std::unique_ptr<JPH::TempAllocatorImpl>    m_temp_allocator;
            std::unique_ptr<JPH::JobSystemThreadPool>  m_job_system;
            phys::BroadPhaseLayerInterfaceImpl         m_bp_layer_interface;
            phys::ObjectVsBroadphaseLayerFilterImpl    m_object_vs_bp_filter;
            phys::ObjectLayerPairFilterImpl            m_object_pair_filter;
            std::unique_ptr<JPH::PhysicsSystem>        m_physics;
    };
} // thresh
