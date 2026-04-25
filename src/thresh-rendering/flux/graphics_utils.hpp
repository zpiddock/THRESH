
#pragma once
#include <memory>

#include "vkbackend/vulkan_context.hpp"


namespace flux {

    class GraphicsUtils {
        public:

            auto init_vulkan(const VulkanInstanceContext& ctx) -> void;

            auto init_vulkan() -> void;

            auto get_vulkan_context() -> VulkanContext*;

        private:
            std::unique_ptr<VulkanContext> m_context;

    };

} // namespace flux
