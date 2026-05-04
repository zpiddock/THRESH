
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

            auto recreate_swapchain() -> void;

            auto set_framebuffer_resized(bool resized) -> void;

            auto update_uniform_buffers(uint32_t frame_index) -> void;

        private:
            std::unique_ptr<VulkanContext> m_context;

            bool                   m_framebuffer_resized = false;
            // Non Owning
            const thresh::Window* m_window = nullptr;
    };

} // namespace flux
