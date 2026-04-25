//
// flux.cpp — Graphics utilities for the Vulkan rendering backend.
//

#include "graphics_utils.hpp"

namespace flux {
    auto GraphicsUtils::init_vulkan(const VulkanInstanceContext& ctx) -> void {
        m_context = std::make_unique<VulkanContext>(ctx);
    }

    auto GraphicsUtils::init_vulkan() -> void {

        const VulkanInstanceContext ctx = {
            .application_name = "Thresh Application",
            .engine_name = "THRΞSH",
            .engine_version = "0.0.1",
            .application_version = "0.0.1"
        };
        init_vulkan(ctx);
    }

    auto GraphicsUtils::get_vulkan_context() -> VulkanContext* {

        return m_context.get();
    }
} // namespace flux
