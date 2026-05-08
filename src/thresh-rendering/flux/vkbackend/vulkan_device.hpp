//
// Created by Admin on 06/05/2026.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace flux {
    class ThreshVkDevice {

        public:
            ThreshVkDevice(const vk::raii::Instance& instance, const vk::SurfaceKHR& surface);

            auto physical() -> const vk::raii::PhysicalDevice& {
                return m_physicalDevice;
            }

            auto logical() -> const vk::raii::Device& {
                return m_device;
            }

            auto command_pool() -> const vk::raii::CommandPool& {
                return m_command_pool;
            }

            auto graphics_queue() -> const vk::raii::Queue& {
                return m_graphics_queue;
            }

        private:
            auto pick_suitable_device(const vk::raii::Instance& instance) -> void;

            auto create_logical_device(const vk::SurfaceKHR& surface) -> void;

            auto is_device_suitable(const vk::PhysicalDevice& device) -> bool;

            auto create_command_pool() -> void;

            vk::raii::PhysicalDevice m_physicalDevice     = nullptr;
            vk::raii::Device         m_device             = nullptr;
            uint32_t                 m_queue_family_index = ~0u;
            vk::raii::Queue          m_graphics_queue     = nullptr;
            vk::raii::CommandPool    m_command_pool       = nullptr;

            std::vector<const char*> m_required_device_extensions = {
                vk::KHRSwapchainExtensionName
            };
    };
} // flux
