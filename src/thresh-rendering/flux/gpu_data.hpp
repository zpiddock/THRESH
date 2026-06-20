//
// Created by Admin on 14/06/2026.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "math.hpp"
#include "vkbackend/descriptor_handle.hpp"

namespace flux::gpu {

    inline constexpr auto MAX_POINT_LIGHTS = 4;

    inline constexpr std::uint32_t MATERIAL_FLAG_ALPHA_MASK   = 1u << 0;
    inline constexpr std::uint32_t MATERIAL_FLAG_DOUBLE_SIDED = 1u << 1;
    inline constexpr std::uint32_t MATERIAL_FLAG_HAS_NORMAL   = 1u << 2;
    inline constexpr std::uint32_t MATERIAL_FLAG_HAS_EMISSIVE = 1u << 3;

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
        DescriptorHandle  albedo_tex;   //  8 -> resource heap index
        DescriptorHandle  sampler;      //  8 -> sampler heap index
    };
    static_assert(sizeof(PushConstants) == 112);

    struct CompositePushConstants {

        DescriptorHandle  colour_image;
        DescriptorHandle  sampler;
    };
    static_assert(sizeof(CompositePushConstants) == 16);

    struct alignas(16) MaterialData {

        flux::float4  base_colour_factor;
        flux::float3  emissive_factor;
        float         metallic_factor;
        float         roughness_factor;
        float         normal_scale;
        float         occlusion_strength;
        float         alpha_cutoff;
        std::uint32_t base_colour_texture_handle;
        std::uint32_t normal_texture_handle;
        std::uint32_t emissive_texture_handle;
        std::uint32_t metallic_roughness_texture_handle; // G Chan: Roughness, B Chan: Metallic
        std::uint32_t occlusion_texture_handle;
        std::uint32_t flags;
    };
    static_assert(sizeof(MaterialData) == 80);
}
