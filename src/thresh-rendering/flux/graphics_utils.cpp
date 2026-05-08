//
// flux.cpp — Graphics utilities for the Vulkan rendering backend.
//

#include "graphics_utils.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics_types.hpp"
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
        auto fence_result = m_context->m_vk_device.logical().waitForFences(fence, vk::True, UINT64_MAX);
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
            m_context->m_vk_swapchain.swapchain().acquireNextImage(
                UINT64_MAX, present_semaphore, nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreate_swapchain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            SUB_FATAL("Failed to acquire next image!");
        }

        update_uniform_buffers(m_context->m_frame_index);

        m_context->m_vk_device.logical().resetFences(fence);

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

        m_context->m_vk_device.graphics_queue().submit(submit_info, fence);

        const vk::PresentInfoKHR present_info = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*m_context->m_vk_swapchain.swapchain(),
            .pImageIndices = &image_index,
        };

        try {
            result = m_context->m_vk_device.graphics_queue().presentKHR(present_info);
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

        m_context->m_vk_swapchain.recreate(*m_window, m_context->m_vk_instance, m_context->m_vk_device);
        m_context->create_depth_resources();
    }

    auto GraphicsUtils::shutdown() -> void {

        m_context->m_vk_device.logical().waitIdle();
    }

    auto GraphicsUtils::set_framebuffer_resized(bool resized) -> void {
        m_framebuffer_resized = resized;
    }

    auto GraphicsUtils::update_uniform_buffers(uint32_t frame_index) -> void {

        // Testing Purposes only, all updates to be done in update loop, not draw loop
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();


        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(
            glm::radians(45.f),
            static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().width) /
            static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().height),
            0.1f, 10.f
            );
        ubo.projection[1][1] *= -1; // Invert Y due to glm being designed for OpenGL

        memcpy(m_context->m_uniform_buffers_mapped[frame_index], &ubo, sizeof(ubo));
    }
} // namespace flux
