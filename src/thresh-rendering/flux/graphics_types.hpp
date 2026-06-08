//
// Created by Admin on 03/05/2026.
//
#pragma once

#include <vulkan/vulkan.hpp>

#include "math.hpp"

namespace flux {

    inline constexpr auto MAX_POINT_LIGHTS = 4;

    struct alignas(16) PushConstants {
        flux::float4x4 model;
        flux::float4 base_colour;
    };

    struct alignas(16) CameraData {
        flux::float4x4 view;
        flux::float4x4 projection;
    };

    struct alignas(16) GPUPointLight {
        flux::float4 position;
        flux::float4 colour;
    };

    struct LightData {
        alignas(16) flux::float4 ambient_light; // rgb = colour, a = intensity
        std::array<GPUPointLight, MAX_POINT_LIGHTS> point_lights;
        alignas(16) int active_point_lights;
    };

    struct TextureData {

        int width, height, num_channels;
        std::span<std::uint8_t> pixel_data;
    };

    struct Vertex {

        flux::float3 position;
        flux::float3 normal;
        flux::float2 tex_coord;

        static auto get_binding_description() -> vk::VertexInputBindingDescription {

            return {.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex};
        }

        static auto get_attribute_descriptions() -> std::array<vk::VertexInputAttributeDescription, 3> {

            return {
                    {
                        {
                            .location = 0,
                            .binding = 0,
                            .format = vk::Format::eR32G32B32Sfloat, //slang float3, glsl vec3
                            .offset = offsetof(Vertex, position)
                        },
                        {
                            .location = 1,
                            .binding = 0,
                            .format = vk::Format::eR32G32B32Sfloat, // slang float3, glsl vec3
                            .offset = offsetof(Vertex, normal)
                        },
                        {
                            .location = 2,
                            .binding = 0,
                            .format = vk::Format::eR32G32Sfloat, // slang float2, glsl vec3
                            .offset = offsetof(Vertex, tex_coord)
                        }
                    }
            };
        }
    };
}
