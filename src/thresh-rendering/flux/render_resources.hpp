//
// Created by Admin on 09/05/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "flux/math.hpp"
#include "vkbackend/buffer.hpp"

namespace flux {
    struct MeshResource {

        flux::Buffer vertex;
        flux::Buffer index;
        uint32_t index_count{0};
        flux::AABB local_aabb{};
    };

    struct TextureResource {
        flux::Image image{};
        vk::raii::Sampler sampler{nullptr};
    };

    struct MaterialResource {

        std::string material_type;
        std::uint32_t albedo_texture_handle = 0;
        flux::float4 albedo_tint = flux::float4(1.0f);
        std::uint32_t normal_texture_handle = 0;
        std::uint32_t metallic_texture_handle = 0;
        std::uint32_t roughness_texture_handle = 0;
        std::uint32_t emission_texture_handle = 0;

        std::vector<vk::raii::DescriptorSet> descriptor_sets;
    };

    struct DrawCommand {

        flux::float4x4 model;
        flux::float4 base_colour;
        std::uint32_t mesh_handle;
        std::uint32_t material_handle;
    };
} // flux