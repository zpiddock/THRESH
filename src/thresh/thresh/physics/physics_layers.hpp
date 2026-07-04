#pragma once

// PCH already includes Jolt.h
#include <thresh/thresh.hpp>

#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"

namespace thresh::phys {

    namespace layers {
        static inline constexpr JPH::ObjectLayer NON_MOVING = 0; // STATIC
        static inline constexpr JPH::ObjectLayer MOVING     = 1; // DYNAMIC
        static inline constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    }
    namespace broadphase {
        static inline constexpr JPH::BroadPhaseLayer NON_MOVING{0}; // STATIC
        static inline constexpr JPH::BroadPhaseLayer MOVING    {1}; // DYNAMIC
        static inline constexpr JPH::uint            NUM_LAYERS{2};
    }

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
        public:
            [[nodiscard]] auto ShouldCollide(JPH::ObjectLayer layer_a, JPH::ObjectLayer layer_b) const -> bool override {
                return layer_a != layers::NON_MOVING || layer_b != layers::NON_MOVING;
            }
    };

    class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
        public:
            BroadPhaseLayerInterfaceImpl() {

                m_object_to_broadphase[layers::NON_MOVING] = broadphase::NON_MOVING;
                m_object_to_broadphase[layers::MOVING]     = broadphase::MOVING;
            }

            auto GetNumBroadPhaseLayers() const -> JPH::uint override {

                return broadphase::NUM_LAYERS;
            }

            auto GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const -> JPH::BroadPhaseLayer override {

                return m_object_to_broadphase[inLayer];
            }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
            auto GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const -> const char* override {

                switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {

                    case static_cast<JPH::BroadPhaseLayer::Type>(broadphase::NON_MOVING): return "STATIC";
                    case static_cast<JPH::BroadPhaseLayer::Type>(broadphase::MOVING): return "DYNAMIC";
                    default: return std::format("Invalid BroadPhase Layer {}", static_cast<JPH::BroadPhaseLayer::Type>(inLayer)).c_str();
                }
            }
#endif

        private:
            JPH::BroadPhaseLayer m_object_to_broadphase[layers::NUM_LAYERS];
    };

    class ObjectVsBroadphaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {

        public:
        [[nodiscard]] auto ShouldCollide(JPH::ObjectLayer layer_a, JPH::BroadPhaseLayer layer_b) const -> bool override {
            return layer_a != layers::NON_MOVING || layer_b != broadphase::NON_MOVING;
        }
    };
}
