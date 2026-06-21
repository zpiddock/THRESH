//
// Created by Admin on 13/06/2026.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "vulkan_device.hpp"

namespace helix {

    struct ImageState {
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eTopOfPipe;
        vk::AccessFlags2 access = {};
    };

    class Image {

        public:

            struct Desc {
                vk::Extent2D extent = {};
                vk::Format format = {};
                vk::ImageUsageFlags usage = {};
                vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
                vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
                vk::MemoryPropertyFlags memory = vk::MemoryPropertyFlagBits::eDeviceLocal;
                uint32_t mip_levels = 1;
                const char* debug_name = nullptr;
            };

            Image() = default;
            Image(ThreshVkDevice& device, const Desc& desc);

            Image(const Image&)                             = delete;
            auto operator=(const Image&) -> Image&          = delete;
            Image(Image&&) noexcept                         = default;
            auto operator=(Image&&) noexcept -> Image&      = default;

            [[nodiscard]] auto handle() const -> vk::Image    { return *m_image; }
            [[nodiscard]] auto format() const -> vk::Format   { return m_desc.format; }
            [[nodiscard]] auto extent() const -> vk::Extent2D { return m_desc.extent; }
            [[nodiscard]] auto aspect() const -> vk::ImageAspectFlags { return m_desc.aspect; }

            [[nodiscard]] auto view() const -> vk::ImageView  { return *m_view; }


            [[nodiscard]] auto view_create_info() const -> vk::ImageViewCreateInfo {
                return {
                    .image            = *m_image,
                    .viewType         = vk::ImageViewType::e2D,
                    .format           = m_desc.format,
                    .subresourceRange = {
                        .aspectMask = m_desc.aspect,    .baseMipLevel   = 0,
                        .levelCount = m_desc.mip_levels, .baseArrayLayer = 0, .layerCount = 1
                    }
                };
            }

            [[nodiscard]] auto state() const -> const ImageState&       { return m_state; }
            auto set_state(const ImageState& s) -> void                 { m_state = s; }

            // ~0u = not in heap.
            [[nodiscard]] auto heap_index() const -> uint32_t           { return m_heap_index; }
            auto set_heap_index(const uint32_t idx) -> void             { m_heap_index = idx; }

        private:
            vk::raii::DeviceMemory m_memory = nullptr;
            vk::raii::Image m_image = nullptr;
            vk::raii::ImageView m_view = nullptr;
            Desc m_desc = {};
            ImageState m_state = {};
            uint32_t m_heap_index = ~0u;
    };
} // flux
