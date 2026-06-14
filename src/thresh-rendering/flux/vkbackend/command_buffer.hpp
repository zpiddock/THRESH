//
// Created by Admin on 14/06/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

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

        private:
            auto emit_barrier(vk::Image image, vk::ImageAspectFlags aspect, const ImageState& src, const ImageState& dst);

            vk::raii::CommandBuffer m_command_buffer = nullptr;
    };
} // flux
