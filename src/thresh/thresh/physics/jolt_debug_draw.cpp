//
// Created by Admin on 04/07/2026.
//

#include "jolt_debug_draw.hpp"

#include "jolt_math.hpp"
#include "thresh/engine.hpp"

namespace thresh::phys {
    void JoltDebugDraw::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor,
        ECastShadow inCastShadow) {
        DebugRendererSimple::DrawTriangle(inV1, inV2, inV3, inColor, inCastShadow);
    }

    auto JoltDebugDraw::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg c) -> void {

        auto* dlr = Engine::get_instance().graphics()->debug_line_renderer();

        if (!dlr) return;

        dlr->submit_line(to_helix(inFrom), to_helix(inTo), {c.r / 255.f, c.g / 255.f, c.b / 255.f});
    }
}
