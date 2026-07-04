//
// Created by Admin on 04/07/2026.
//

#pragma once

#include "thresh/thresh.hpp"

#ifdef JPH_DEBUG_RENDERER
#include "Jolt/Renderer/DebugRendererSimple.h"

namespace thresh::phys {
    class JoltDebugDraw final : public JPH::DebugRendererSimple {
        public:
            auto DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor,
                ECastShadow inCastShadow) -> void override;

            auto DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) -> void override;

            auto DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor,
                float inHeight) -> void override {}
    };
}

#endif
