//
// Created by Admin on 03/05/2026.
//
#pragma once

#include <vulkan/vulkan.hpp>

#include "gpu_data.hpp"
#include "helix/math.hpp"

namespace flux {

    struct TextureData {

        int width, height, num_channels;
        std::span<std::uint8_t> pixel_data;
    };

    struct Vertex {

        helix::float3 position;
        helix::float3 normal;
        helix::float2 tex_coord;

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
