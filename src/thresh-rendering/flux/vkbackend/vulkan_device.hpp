//
// Created by Admin on 06/05/2026.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace flux {
    class ThreshVkDevice {

        public:
            ThreshVkDevice(const vk::raii::Instance& instance, const vk::SurfaceKHR& surface);

            auto transition_image_layout(const vk::raii::Image& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::ImageAspectFlags aspect_flags) -> void;

            auto find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) -> uint32_t;

            auto create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) -> std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>;

            auto begin_single_time_commands() -> vk::raii::CommandBuffer;

            auto end_single_time_commands(const vk::raii::CommandBuffer& command_buffer) -> void;

            auto copy_buffer(const vk::raii::Buffer& src_buffer, vk::raii::Buffer& dst_buffer, vk::DeviceSize size) -> void;

            auto copy_buffer_to_image(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height) -> void;

            auto create_image(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling_mode, vk::ImageUsageFlags usage_flags, vk::MemoryPropertyFlags memory_props) -> std::pair<vk::raii::Image, vk::raii::DeviceMemory>;

            auto create_image_view(const vk::Image& image, vk::Format format, vk::ImageAspectFlags flags) -> vk::raii::ImageView;

            auto find_supported_format(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) -> vk::Format;

            auto find_depth_format() -> vk::Format;

            auto has_stencil_component(vk::Format format) -> bool;

            auto physical() -> const vk::raii::PhysicalDevice& {
                return m_physical_device;
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

            vk::raii::PhysicalDevice m_physical_device     = nullptr;
            vk::raii::Device         m_device             = nullptr;
            uint32_t                 m_queue_family_index = ~0u;
            vk::raii::Queue          m_graphics_queue     = nullptr;
            vk::raii::CommandPool    m_command_pool       = nullptr;

            std::vector<const char*> m_required_device_extensions = {
                vk::KHRSwapchainExtensionName
            };
    };
} // flux
