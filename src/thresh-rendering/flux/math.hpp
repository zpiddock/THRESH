//
// Created by Admin on 08/05/2026.
//

#pragma once
//
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace flux {

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
}

namespace flux::math {

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
}
