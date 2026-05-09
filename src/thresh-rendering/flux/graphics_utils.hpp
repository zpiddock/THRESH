
#pragma once
#include <memory>

#include "graphics_types.hpp"
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

            auto set_camera_data(const CameraData& camera_data) -> void;

            auto get_aspect_ratio() -> float;

        private:
            std::unique_ptr<VulkanContext> m_context;

            std::optional<CameraData> m_camera_data;

            bool                   m_framebuffer_resized = false;
            // Non Owning
            const thresh::Window* m_window = nullptr;
    };

} // namespace flux
