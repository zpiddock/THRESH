//
// Created by Admin on 03/05/2026.
//
#pragma once

#include <vulkan/vulkan.hpp>

#include "glm/glm.hpp"

namespace flux {

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
    };

    struct Vertex {

        glm::vec2 position;
        glm::vec3 colour;

        static auto get_binding_description() -> vk::VertexInputBindingDescription {

            return {.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex};
        }

        static auto get_attribute_descriptions() -> std::array<vk::VertexInputAttributeDescription, 2> {

            return {
                    {
                        {
                            .location = 0,
                            .binding = 0,
                            .format = vk::Format::eR32G32Sfloat, //slang float2, glsl vec3
                            .offset = offsetof(Vertex, position)
                        },
                        {
                            .location = 1,
                            .binding = 0,
                            .format = vk::Format::eR32G32B32Sfloat, // slang float3, glsl vec3
                            .offset = offsetof(Vertex, colour)
                        }
                    }
            };
        }
    };
}