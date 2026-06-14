//
// Created by Admin on 14/06/2026.
//

#pragma once
#include "math.hpp"

namespace flux::gpu {

    inline constexpr auto MAX_POINT_LIGHTS = 4;

    struct alignas(16) CameraData {

        flux::float4x4 view;
        flux::float4x4 projection;
    };
    static_assert(sizeof(CameraData) == 128);

    struct alignas(16) PointLight {
        flux::float4 position;
        flux::float4 colour; // rgb - colour, a intensity
    };

    struct alignas(16) LightData {
        flux::float4 ambient;
        PointLight lights[MAX_POINT_LIGHTS];
        int32_t num_lights;
    };
    static_assert(sizeof(LightData) == 16 + MAX_POINT_LIGHTS * sizeof(PointLight) + 16);

    struct alignas(16) PushConstants {
        flux::float4x4    model;        // 64
        flux::float4      colour_tint;  // 16
        vk::DeviceAddress camera;       //  8 -> CameraData* in the shader
        vk::DeviceAddress lights;       //  8 -> LightData*
    };
    static_assert(sizeof(PushConstants) == 96);
}
