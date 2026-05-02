//
// flux.cpp — Graphics utilities for the Vulkan rendering backend.
//

#include "graphics_utils.hpp"

#include "horizon/window.hpp"
#include "substratum/log.hpp"

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

    auto GraphicsUtils::draw_frame() -> void {

        auto fence_result = m_context->m_device.waitForFences(*m_context->m_draw_fence, vk::True, UINT64_MAX);
        if (fence_result != vk::Result::eSuccess) {
            SUB_FATAL("Failed to wait for fence!");
        }
        m_context->m_device.resetFences(*m_context->m_draw_fence);

        auto [result, image_index] =
            m_context->m_swapchain.acquireNextImage(
                UINT64_MAX, *m_context->m_present_complete_semaphore, nullptr);
        if (result != vk::Result::eSuccess) {
            SUB_FATAL("Failed to acquire next image!");
        }

        m_context->record_command_buffer(image_index);

        m_context->m_device.waitIdle();

        vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo submit_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*m_context->m_present_complete_semaphore,
            .pWaitDstStageMask = &wait_destination_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*m_context->m_command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*m_context->m_render_complete_semaphore
        };

        m_context->m_graphics_queue.submit(submit_info, *m_context->m_draw_fence);

        const vk::PresentInfoKHR present_info = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*m_context->m_render_complete_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*m_context->m_swapchain,
            .pImageIndices = &image_index,
        };

        result = m_context->m_graphics_queue.presentKHR(present_info);
    }

    auto GraphicsUtils::shutdown() -> void {

        m_context->m_device.waitIdle();
    }
} // namespace flux
