
#pragma once
#include <memory>

#include "horizon/window.hpp"
#include "vkbackend/vulkan_context.hpp"


namespace flux {

    class GraphicsUtils {
        public:

            auto init_vulkan(const VulkanInstanceContext& ctx, const thresh::Window& window) -> void;

            auto init_vulkan(const thresh::Window& window) -> void;

            auto get_vulkan_context() -> VulkanContext*;

            auto draw_frame() -> void;

            auto shutdown() -> void;

        private:
            std::unique_ptr<VulkanContext> m_context;

    };

} // namespace flux
