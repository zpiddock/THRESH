#pragma once

#include "thresh/thresh.hpp"
#include "helix/math.hpp"

namespace thresh::phys {

    // TODO: Extend to Vec4 if needed
    inline auto to_jph(const helix::float3& v) -> JPH::Vec3 {
        return {v.x, v.y, v.z};
    }

    inline auto to_helix(JPH::Vec3 v) -> helix::float3 {
        return {v.GetX(), v.GetY(), v.GetZ()};
    }

    // JPH Quat takes x,y,z,w
    // Helix (GLM) Quat takes w,x,y,z
    inline auto to_jph(const helix::quat& q) -> JPH::Quat {
        return {q.x, q.y, q.z, q.w};
    }

    inline auto to_helix(JPH::Quat q) -> helix::quat {
        return {q.GetW(), q.GetX(), q.GetY(), q.GetZ()};
    }
}