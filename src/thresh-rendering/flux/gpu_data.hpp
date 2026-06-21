//
// Created by Admin on 14/06/2026.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "helix/math.hpp"
#include "vkbackend/descriptor_handle.hpp"

namespace flux::gpu {

    inline constexpr auto MAX_POINT_LIGHTS = 4;

    inline constexpr std::uint32_t MATERIAL_FLAG_ALPHA_MASK   = 1u << 0;
    inline constexpr std::uint32_t MATERIAL_FLAG_DOUBLE_SIDED = 1u << 1;
    inline constexpr std::uint32_t MATERIAL_FLAG_HAS_NORMAL   = 1u << 2;
    inline constexpr std::uint32_t MATERIAL_FLAG_HAS_EMISSIVE = 1u << 3;

    struct alignas(16) CameraData {

        helix::float4x4 view;
        helix::float4x4 projection;
    };
    static_assert(sizeof(CameraData) == 128);

    struct alignas(16) PointLight {
        helix::float4 position;
        helix::float4 colour; // rgb - colour, a intensity
    };

    struct alignas(16) LightData {
        helix::float4 ambient;
        PointLight lights[MAX_POINT_LIGHTS];
        int32_t num_lights;
    };
    static_assert(sizeof(LightData) == 16 + MAX_POINT_LIGHTS * sizeof(PointLight) + 16);

    struct alignas(16) PushConstants {
        helix::float4x4    model;        // 64
        helix::float4      colour_tint;  // 16
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

        helix::float4  base_colour_factor;
        helix::float3  emissive_factor;
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
        std::uint32_t _pad[2];   // explicit pad to 80
    };
    static_assert(sizeof(MaterialData) == 80);

    struct FramePushConstants {
        vk::DeviceAddress camera;
        vk::DeviceAddress lights;
        vk::DeviceAddress materials;
        DescriptorHandle  default_sampler;
    };
    static_assert(sizeof(FramePushConstants) == 32);

    inline constexpr uint32_t DRAW_PUSH_OFFSET = sizeof(FramePushConstants);

    struct alignas(16) DrawPushConstants {
        helix::float4x4 model;
        helix::float4   colour_tint;
        uint32_t material_handle;
    };
    static_assert(sizeof(DrawPushConstants) == 96);
    static_assert(sizeof(FramePushConstants) + sizeof(DrawPushConstants) <= 128, "Must stay below Vulkan maxPushDataSize");
}
