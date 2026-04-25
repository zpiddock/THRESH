//
// flux.cpp — Graphics utilities for the Vulkan rendering backend.
//

#include "graphics_utils.hpp"

#include "horizon/window.hpp"

namespace flux {
    auto GraphicsUtils::init_vulkan(const VulkanInstanceContext& ctx, const thresh::Window& window) -> void {
        m_context = std::make_unique<VulkanContext>(ctx, window);
    }

    auto GraphicsUtils::init_vulkan(const thresh::Window& window) -> void {

        const VulkanInstanceContext ctx = {
            .application_name = "Thresh Application",
            .engine_name = "THRΞSH",
            .engine_version = "0.0.1",
            .application_version = "0.0.1"
        };
        init_vulkan(ctx, window);
    }

    auto GraphicsUtils::get_vulkan_context() -> VulkanContext* {

        return m_context.get();
    }
} // namespace flux
