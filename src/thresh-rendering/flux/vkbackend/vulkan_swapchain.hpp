//
// Created by Admin on 08/05/2026.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vector>

#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"
#include "horizon/window.hpp"

namespace flux {
    class ThreshVkSwapchain {

        public:
            ThreshVkSwapchain(const thresh::Window& window, ThreshVkInstance& instance, ThreshVkDevice& device, vk::PresentModeKHR preferred);

            auto recreate(const thresh::Window& window, ThreshVkInstance& instance, ThreshVkDevice& device, vk::PresentModeKHR preferred) -> void;

            auto swapchain() -> const vk::raii::SwapchainKHR& {
                return m_swapchain;
            }

            auto swapchain_images() -> const std::vector<vk::Image>& {
                return m_swapchain_images;
            }

            auto swapchain_image_views() -> const std::vector<vk::raii::ImageView>& {
                return m_swapchain_image_views;
            }

            auto swapchain_extent() -> const vk::Extent2D& {
                return m_swapchain_extent;
            }

            auto swapchain_surface_format() -> const vk::SurfaceFormatKHR& {
                return m_swapchain_surface_format;
            }

            ~ThreshVkSwapchain() {
               cleanup_swapchain();
            }

        private:

            auto create_swapchain(const thresh::Window& window, ThreshVkInstance& instance, ThreshVkDevice& device, vk::PresentModeKHR preferred) -> void;

            auto create_image_views(ThreshVkDevice& device) -> void;

            auto choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& formats) -> vk::SurfaceFormatKHR;

            auto choose_swap_extents(const vk::SurfaceCapabilitiesKHR& surface_capabilities,
                                     const thresh::Window&             window) -> vk::Extent2D;

            auto choose_min_swap_image_count(const vk::SurfaceCapabilitiesKHR& surface_capabilities) -> uint32_t;

            auto choose_swapchain_present_mode(
                const std::vector<vk::PresentModeKHR>& present_modes, vk::PresentModeKHR preferred) -> vk::PresentModeKHR;

            auto cleanup_swapchain() -> void;

            vk::raii::SwapchainKHR m_swapchain = nullptr;
            std::vector<vk::Image> m_swapchain_images;
            vk::SurfaceFormatKHR   m_swapchain_surface_format;
            vk::Extent2D           m_swapchain_extent;

            std::vector<vk::raii::ImageView>     m_swapchain_image_views = {};
    };
} // flux
