//
// Created by Admin on 09/05/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "helix/math.hpp"
#include "vkbackend/buffer.hpp"

namespace flux {
    struct MeshResource {

        Buffer vertex;
        Buffer index;
        uint32_t index_count{0};
        helix::AABB local_aabb{};
    };

    struct TextureResource {
        flux::Image image{};
    };

    struct MaterialResource {

        std::string material_type;
        std::uint32_t gpu_index = 0;
    };

    struct DrawCommand {

        helix::float4x4 model;
        helix::float4 base_colour;
        std::uint32_t mesh_handle;
        std::uint32_t material_handle;
    };
} // flux