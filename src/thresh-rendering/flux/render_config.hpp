#pragma once
#include <string>
#include <vulkan/vulkan.hpp>

#include "helix/colour.hpp"

namespace flux {
    struct RenderConfig {
        std::string application_name;
        bool enable_validation_layers = true;
        bool enable_sync_validation = true;
        vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
        std::uint32_t resource_heap_capacity = 4096;
        std::uint32_t sampler_heap_capacity = 64;
        std::uint32_t material_buffer_capacity = 1024;
        helix::Colour clear_colour = helix::from_hex("#1A1A1A");
    };
}
