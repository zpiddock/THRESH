//
// Created by Admin on 08/05/2026.
//

#pragma once
//
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace helix {

    // Functions
    using glm::translate;
    using glm::rotate;
    using glm::scale;
    using glm::perspective;
    using glm::lookAt;
    using glm::inverse;
    using glm::normalize;
    using glm::dot;
    using glm::cross;
    using glm::length;
    using glm::mat4_cast;
    using glm::clamp;
    using glm::radians;
    using glm::degrees;
    using glm::max;
    using glm::min;
    using glm::angleAxis;
    using glm::sin;
    using glm::cos;
    using glm::tan;
    using glm::transpose;
    using glm::abs;
    using glm::value_ptr;
    using glm::decompose;

    // Constants
    using glm::pi;
    using glm::golden_ratio;

    // Types
    using float2 = glm::vec2;
    using float3 = glm::vec3;
    using float4 = glm::vec4;
    using float3x3 = glm::mat3;
    using float4x4 = glm::mat4;
    using quat = glm::quat;
    using int2 = glm::ivec2;
    using int3 = glm::ivec3;
    using int4 = glm::ivec4;
    using uint2 = glm::uvec2;
    using uint3 = glm::uvec3;
    using uint4 = glm::uvec4;

    struct AABB {
    // Use sentinel: an "empty" AABB has min > max. valid() detects this.
        helix::float3 min{ std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()};
        helix::float3 max{-std::numeric_limits<float>::max(),
                   -std::numeric_limits<float>::max(),
                   -std::numeric_limits<float>::max()};

        [[nodiscard]] auto valid() const -> bool {
            return min.x <= max.x && min.y <= max.y && min.z <= max.z;
        }

        auto expand(const float3& p) -> void {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }

        auto expand(const AABB& o) -> void {
            if (!o.valid()) return;
            min = glm::min(min, o.min);
            max = glm::max(max, o.max);
        }

        [[nodiscard]] auto center() const -> float3 { return (min + max) * 0.5f; }
        [[nodiscard]] auto extents() const -> float3 { return (max - min) * 0.5f; }
    };

    // Transform a local-space AABB into a world-space AABB by refitting the 8 corners.
    // Correct for any affine transform (translate/rotate/non-uniform-scale/shear).
    inline auto transform_aabb(const AABB& local, const float4x4& m) -> AABB {
        if (!local.valid()) return {};
        AABB out;
        const std::array<float3,8> corners {
            helix::float3{local.min.x, local.min.y, local.min.z},
            helix::float3{local.max.x, local.min.y, local.min.z},
            helix::float3{local.min.x, local.max.y, local.min.z},
            helix::float3{local.max.x, local.max.y, local.min.z},
            helix::float3{local.min.x, local.min.y, local.max.z},
            helix::float3{local.max.x, local.min.y, local.max.z},
            helix::float3{local.min.x, local.max.y, local.max.z},
            helix::float3{local.max.x, local.max.y, local.max.z},
        };
        for (const auto& c : corners) {
            const helix::float4 w = m * helix::float4{c, 1.0f};
            out.expand(float3{w});
        }
        return out;
    }

    // Branchless slab-test ray-AABB. Returns t_min if hit (>=0) else nullopt.
    // ray_origin/ray_dir in same space as aabb. ray_dir does NOT need to be unit length.
    inline auto ray_aabb_intersect(const helix::float3& ro, const helix::float3& rd, const helix::AABB& box)
        -> std::optional<float> {
        if (!box.valid()) return std::nullopt;
        const helix::float3 inv = helix::float3{1.0f} / rd;
        const helix::float3 t0  = (box.min - ro) * inv;
        const helix::float3 t1  = (box.max - ro) * inv;
        const helix::float3 tsm = helix::min(t0, t1);
        const helix::float3 tbg = helix::max(t0, t1);
        const float tmin = std::max({tsm.x, tsm.y, tsm.z, 0.0f});
        const float tmax = std::min({tbg.x, tbg.y, tbg.z});
        if (tmax < tmin) return std::nullopt;
        return tmin;
    }
}