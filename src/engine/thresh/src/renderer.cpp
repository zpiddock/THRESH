#include "thresh/renderer.hpp"
#include "thresh/engine_config.hpp"
#include "substratum/log.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>

namespace thresh {
    Renderer::Renderer(horizon::Window &window, const EngineConfig &config) {
        SUB_INFO("Initializing Renderer");

        // --- Instance ---
        std::uint32_t glfw_ext_count = 0;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

        flux::Instance::Config instance_config{};
        instance_config.application_name = config.application_name;
        instance_config.engine_name = "THRESH Engine";
        instance_config.enable_validation = config.enable_validation;

        for (std::uint32_t i = 0; i < glfw_ext_count; ++i) {
            instance_config.extensions.push_back(glfw_extensions[i]);
        }
        if (config.enable_validation) {
            instance_config.extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            instance_config.validation_layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        m_instance = std::make_unique<flux::Instance>(instance_config);

        // --- Surface ---
        m_surface = std::make_unique<flux::Surface>(m_instance->get_handle(), window);

        // --- Device ---
        flux::Device::Config device_config{};
        device_config.instance = m_instance->get_handle();
        device_config.surface = m_surface->get_handle();
        device_config.device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        m_device = std::make_unique<flux::Device>(device_config);

        // Give the device its own command pool for single-time commands
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = m_device->get_graphics_queue_family();

        VkCommandPool command_pool = VK_NULL_HANDLE;
        if (::vkCreateCommandPool(m_device->get_logical_device(), &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
            SUB_FATAL("Failed to create device command pool");
            throw std::runtime_error("Failed to create device command pool");
        }
        m_device->set_command_pool(command_pool);

        // --- Swapchain ---
        auto [fb_width, fb_height] = window.get_framebuffer_size();

        flux::Swapchain::Config swapchain_config{};
        swapchain_config.physical_device = m_device->get_physical_device();
        swapchain_config.device = m_device->get_logical_device();
        swapchain_config.surface = m_surface->get_handle();
        swapchain_config.width = fb_width;
        swapchain_config.height = fb_height;
        swapchain_config.preferred_present_mode = config.preferred_present_mode;

        m_swapchain = std::make_unique<flux::Swapchain>(swapchain_config);

        // --- Per-frame resources ---
        for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_frames[i] = FrameData::create(
                m_device->get_logical_device(),
                m_device->get_graphics_queue_family()
            );
        }

        SUB_INFO("Renderer initialized ({}x{}, {} frames in flight)",
                 m_swapchain->get_extent().width, m_swapchain->get_extent().height,
                 MAX_FRAMES_IN_FLIGHT);
    }

    Renderer::~Renderer() {
        if (m_device) {
            m_device->wait_idle();
        }

        // FrameData destructors run here (unique_ptrs inside each FrameData)
        for (auto &frame : m_frames) {
            if (frame.deletion_queue) {
                frame.deletion_queue->flush_all();
            }
        }

        // Destroy the command pool we created for single-time commands
        if (m_device && m_device->get_command_pool() != VK_NULL_HANDLE) {
            ::vkDestroyCommandPool(m_device->get_logical_device(), m_device->get_command_pool(), nullptr);
            m_device->set_command_pool(VK_NULL_HANDLE);
        }

        // unique_ptrs destroy in reverse member declaration order:
        // m_swapchain -> m_device -> m_surface -> m_instance
    }

    auto Renderer::begin_frame() -> std::optional<VkCommandBuffer> {
        // Handle any pending resize
        {
            std::lock_guard lock(m_resize_mutex);
            if (m_resize_pending) {
                handle_resize();
                m_resize_pending = false;
            }
        }

        auto &frame = m_frames[m_current_frame];

        // Wait for this frame's previous work to finish
        frame.in_flight_fence->wait();

        // Flush deferred deletions that are now safe
        frame.deletion_queue->flush(m_frame_number);

        // Acquire next swapchain image
        auto result = ::vkAcquireNextImageKHR(
            m_device->get_logical_device(),
            m_swapchain->get_handle(),
            UINT64_MAX,
            frame.image_available_semaphore->get_handle(),
            VK_NULL_HANDLE,
            &m_current_image_index
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            SUB_WARN("Swapchain out of date on acquire - recreating");
            handle_resize();
            return std::nullopt;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            SUB_ERROR("Failed to acquire swapchain image: {}", static_cast<int>(result));
            return std::nullopt;
        }

        // Reset fence only after we know we'll submit work
        frame.in_flight_fence->reset();

        // Reset and begin command buffer
        frame.command_buffer->reset();
        auto cmd = frame.command_buffer->get_command_buffer(0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (::vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS) {
            SUB_ERROR("Failed to begin command buffer");
            return std::nullopt;
        }

        return cmd;
    }

    auto Renderer::end_frame() -> void {
        auto &frame = m_frames[m_current_frame];
        auto cmd = frame.command_buffer->get_command_buffer(0);

        if (::vkEndCommandBuffer(cmd) != VK_SUCCESS) {
            SUB_ERROR("Failed to end command buffer");
            return;
        }

        // Submit using vkQueueSubmit2
        VkSemaphoreSubmitInfo wait_semaphore{};
        wait_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait_semaphore.semaphore = frame.image_available_semaphore->get_handle();
        wait_semaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signal_semaphore{};
        signal_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal_semaphore.semaphore = frame.render_finished_semaphore->get_handle();
        signal_semaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

        VkCommandBufferSubmitInfo cmd_info{};
        cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmd_info.commandBuffer = cmd;

        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit_info.waitSemaphoreInfoCount = 1;
        submit_info.pWaitSemaphoreInfos = &wait_semaphore;
        submit_info.commandBufferInfoCount = 1;
        submit_info.pCommandBufferInfos = &cmd_info;
        submit_info.signalSemaphoreInfoCount = 1;
        submit_info.pSignalSemaphoreInfos = &signal_semaphore;

        if (::vkQueueSubmit2(m_device->get_graphics_queue(), 1, &submit_info,
                             frame.in_flight_fence->get_handle()) != VK_SUCCESS) {
            SUB_ERROR("Failed to submit command buffer");
            return;
        }

        // Present
        VkSwapchainKHR swapchains[] = {m_swapchain->get_handle()};
        VkSemaphore wait_sems[] = {frame.render_finished_semaphore->get_handle()};

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = wait_sems;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &m_current_image_index;

        auto result = ::vkQueuePresentKHR(m_device->get_present_queue(), &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            SUB_WARN("Swapchain out of date/suboptimal on present - will recreate next frame");
            std::lock_guard lock(m_resize_mutex);
            m_resize_pending = true;
        } else if (result != VK_SUCCESS) {
            SUB_ERROR("Failed to present: {}", static_cast<int>(result));
        }

        // Advance frame
        m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
        ++m_frame_number;
    }

    auto Renderer::request_resize(std::uint32_t width, std::uint32_t height) -> void {
        std::lock_guard lock(m_resize_mutex);
        m_resize_pending = true;
        m_resize_width = width;
        m_resize_height = height;
    }

    auto Renderer::wait_idle() -> void {
        if (m_device) {
            m_device->wait_idle();
        }
    }

    auto Renderer::get_current_image() const -> VkImage {
        return m_swapchain->get_images()[m_current_image_index];
    }

    auto Renderer::get_current_image_view() const -> VkImageView {
        return m_swapchain->get_image_views()[m_current_image_index];
    }

    auto Renderer::handle_resize() -> void {
        m_device->wait_idle();

        auto width = m_resize_width;
        auto height = m_resize_height;

        // If resize was triggered by swapchain (not explicit), use 0 to let swapchain query
        if (width == 0 || height == 0) {
            auto extent = m_swapchain->get_extent();
            width = extent.width;
            height = extent.height;
        }

        if (width == 0 || height == 0) {
            SUB_WARN("Skipping resize: zero-size framebuffer");
            return;
        }

        SUB_INFO("Recreating swapchain: {}x{}", width, height);
        m_swapchain->resize(width, height);
    }
} // namespace thresh
