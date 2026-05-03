//
// flux.cpp — Graphics utilities for the Vulkan rendering backend.
//

#include "graphics_utils.hpp"

#include "horizon/window.hpp"
#include "SDL3/SDL_events.h"
#include "substratum/log.hpp"

namespace flux {
    auto GraphicsUtils::init_vulkan(const VulkanInstanceContext& ctx, const thresh::Window& window) -> void {
        m_window = &window;
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

        const auto fence = *m_context->m_inflight_fences[m_context->m_frame_index];
        auto fence_result = m_context->m_device.waitForFences(fence, vk::True, UINT64_MAX);
        if (fence_result != vk::Result::eSuccess) {
            SUB_FATAL("Failed to wait for fence!");
        }

        if (m_framebuffer_resized) {
            set_framebuffer_resized(false);
            recreate_swapchain();
            return;
        }

        auto present_semaphore = *m_context->m_present_complete_semaphores[m_context->m_frame_index];
        auto [result, image_index] =
            m_context->m_swapchain.acquireNextImage(
                UINT64_MAX, present_semaphore, nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreate_swapchain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            SUB_FATAL("Failed to acquire next image!");
        }

        m_context->m_device.resetFences(fence);

        m_context->m_command_buffers[m_context->m_frame_index].reset();
        m_context->record_command_buffer(image_index);

        auto render_semaphore = *m_context->m_render_complete_semaphores[image_index];
        vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo submit_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &present_semaphore,
            .pWaitDstStageMask = &wait_destination_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*m_context->m_command_buffers[m_context->m_frame_index],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_semaphore
        };

        m_context->m_graphics_queue.submit(submit_info, fence);

        const vk::PresentInfoKHR present_info = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*m_context->m_swapchain,
            .pImageIndices = &image_index,
        };

        try {
            result = m_context->m_graphics_queue.presentKHR(present_info);
            if (result == vk::Result::eErrorOutOfDateKHR) {
                set_framebuffer_resized(false);
                recreate_swapchain();
                return;
            }
        } catch (const vk::OutOfDateKHRError& e) {
            recreate_swapchain();
            return;
        }
        m_context->m_frame_index = (m_context->m_frame_index + 1) % VulkanContext::MAX_FRAMES_IN_FLIGHT;
    }

    auto GraphicsUtils::recreate_swapchain() -> void {

        m_context->m_device.waitIdle();

        m_context->cleanup_swapchain();

        m_context->create_swapchain(*m_window);
        m_context->create_image_views();
    }

    auto GraphicsUtils::shutdown() -> void {

        m_context->m_device.waitIdle();
    }

    auto GraphicsUtils::set_framebuffer_resized(bool resized) -> void {
        m_framebuffer_resized = resized;
    }
} // namespace flux
