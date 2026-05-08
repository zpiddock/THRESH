//
// Created by Admin on 08/05/2026.
//

#include "vulkan_swapchain.hpp"

#include "horizon/window.hpp"

namespace flux {
    ThreshVkSwapchain::ThreshVkSwapchain(const thresh::Window& window, ThreshVkInstance& instance, ThreshVkDevice& device) {

        create_swapchain(window, instance, device);
        create_image_views(device);
    }

    auto ThreshVkSwapchain::create_swapchain(const thresh::Window& window, ThreshVkInstance& instance, ThreshVkDevice& device) -> void {

        const auto surface = instance.surface();

        vk::SurfaceCapabilitiesKHR surface_capabilities = device.physical().getSurfaceCapabilitiesKHR(surface);
        m_swapchain_extent                              = choose_swap_extents(surface_capabilities, window);
        uint32_t min_swap_image_count                   = choose_min_swap_image_count(surface_capabilities);

        std::vector<vk::SurfaceFormatKHR> formats = device.physical().getSurfaceFormatsKHR(surface);
        m_swapchain_surface_format                = choose_swap_surface_format(formats);

        vk::PresentModeKHR present_mode =
                choose_swapchain_present_mode(device.physical().getSurfacePresentModesKHR(surface));
        vk::SwapchainCreateInfoKHR swapchain_info{
            .surface          = surface,
            .minImageCount    = min_swap_image_count,
            .imageFormat      = m_swapchain_surface_format.format,
            .imageColorSpace  = m_swapchain_surface_format.colorSpace,
            .imageExtent      = m_swapchain_extent,
            .imageArrayLayers = 1, // Only ever higher if doing Stereoscopic
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform     = surface_capabilities.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = present_mode,
            .clipped          = vk::True,
            .oldSwapchain     = nullptr
        };

        m_swapchain        = vk::raii::SwapchainKHR(device.logical(), swapchain_info);
        m_swapchain_images = m_swapchain.getImages();
    }

    auto ThreshVkSwapchain::create_image_views(ThreshVkDevice& device) -> void {

        m_swapchain_image_views.reserve(m_swapchain_images.size());

        for (const auto& image : m_swapchain_images) {
            m_swapchain_image_views.emplace_back(device.create_image_view(image, m_swapchain_surface_format.format, vk::ImageAspectFlagBits::eColor));
        }
    }

    auto ThreshVkSwapchain::choose_swap_surface_format(
        const std::vector<vk::SurfaceFormatKHR>& formats) -> vk::SurfaceFormatKHR {

        const auto format_interator = std::ranges::find_if(formats, [](const auto& format) {
            return format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace ==
                    vk::ColorSpaceKHR::eSrgbNonlinear;
        });

        return format_interator != formats.end() ? *format_interator : formats[0];
    }

    auto ThreshVkSwapchain::choose_swap_extents(const vk::SurfaceCapabilitiesKHR& surface_capabilities,
        const thresh::Window& window) -> vk::Extent2D {

        if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return surface_capabilities.currentExtent;
        }

        int width, height;
        window.get_frame_buffer_size(width, height);
        return {
            std::clamp<uint32_t>(width, surface_capabilities.minImageExtent.width,
                                 surface_capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, surface_capabilities.minImageExtent.height,
                                 surface_capabilities.maxImageExtent.height)
        };
    }

    auto ThreshVkSwapchain::choose_min_swap_image_count(
        const vk::SurfaceCapabilitiesKHR& surface_capabilities) -> uint32_t {

        auto min_image_count = std::max(3u, surface_capabilities.minImageCount);
        if ((0 > surface_capabilities.maxImageCount) && (min_image_count > surface_capabilities.maxImageCount)) {
            min_image_count = surface_capabilities.maxImageCount;
        }
        return min_image_count;
    }

    auto ThreshVkSwapchain::choose_swapchain_present_mode(
        const std::vector<vk::PresentModeKHR>& present_modes) -> vk::PresentModeKHR {

        assert(std::ranges::any_of(present_modes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo
                   ; }));
        return std::ranges::any_of(present_modes,
                                   [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; })
                   ? vk::PresentModeKHR::eMailbox
                   : vk::PresentModeKHR::eFifo;
    }

    auto ThreshVkSwapchain::recreate(const thresh::Window& window, ThreshVkInstance& instance,
        ThreshVkDevice& device) -> void {

        device.logical().waitIdle();

        cleanup_swapchain();

        create_swapchain(window, instance, device);
        create_image_views(device);
    }

    auto ThreshVkSwapchain::cleanup_swapchain() -> void {

        m_swapchain_image_views.clear();
        m_swapchain = nullptr;
    }
} // flux