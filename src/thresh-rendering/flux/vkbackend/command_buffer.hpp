//
// Created by Admin on 14/06/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "descriptor_heap.hpp"

namespace flux {
    class Buffer;
    class Image;
    struct ImageState;

    class CommandBuffer {

        public:

            CommandBuffer() = default;
            explicit CommandBuffer(vk::raii::CommandBuffer&& command_buffer) : m_command_buffer(std::move(command_buffer)){}

            CommandBuffer(const CommandBuffer&)                        = delete;
            auto operator=(const CommandBuffer&) -> CommandBuffer&     = delete;
            CommandBuffer(CommandBuffer&&) noexcept                    = default;
            auto operator=(CommandBuffer&&) noexcept -> CommandBuffer& = default;

            auto begin(vk::CommandBufferUsageFlags usage = {}) -> void;

            auto end() -> void;
            auto reset() -> void;

            // Tracked transition: computes the src half from image.state(), emits the
            // barrier, writes dst back. NOTE: Image::state() tracks *recording* order, not
            // execution order - fine while one thread records one primary buffer per frame
            // (true for flux today). Multi-threaded recording would move state into a
            // per-recording context
            auto transition(flux::Image& image, const ImageState& dst) -> void;

            auto transition_raw(vk::Image image, vk::ImageAspectFlags aspect, const ImageState& src, const ImageState& dst) -> void;

            auto copy_buffer(const Buffer& src, const Buffer& dst, vk::DeviceSize size) const -> void;

            auto copy_buffer_to_image(const Buffer& src, const Image& dst) const -> void;

            [[nodiscard]] auto raw() const -> const vk::raii::CommandBuffer& { return m_command_buffer; }

            auto bind_heaps(const DescriptorHeap& resource_heap, const DescriptorHeap& sampler_heap) -> void;

            template<typename T>
            auto push_data(const uint32_t offset, const T& value) -> void {
                static_assert(std::is_trivially_copyable_v<T>);
                static_assert(sizeof(T) % 4 == 0, "push data size must be a multiple of 4 (VU)");
                assert(offset % 4 == 0);
                m_command_buffer.pushDataEXT(vk::PushDataInfoEXT{
                    .offset = offset, // no layout, no stage flags
                    .data   = { .address = &value, .size = sizeof(T) },
                });
            }

        private:
            auto emit_barrier(vk::Image image, vk::ImageAspectFlags aspect, const ImageState& src, const ImageState& dst);

            vk::raii::CommandBuffer m_command_buffer = nullptr;
    };
} // flux
