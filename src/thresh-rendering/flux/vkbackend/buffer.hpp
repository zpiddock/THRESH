//
// Created by Admin on 13/06/2026.
//

#pragma once
#include "vulkan_device.hpp"

namespace flux {
    class Buffer {

        public:

            struct Desc {
                vk::DeviceSize size            = 0;
                vk::BufferUsageFlags usage     = {};
                vk::MemoryPropertyFlags memory = {};
                bool persistent_map            = false;
                const char* debug_name         = nullptr;
            };

            Buffer() = default;
            Buffer(ThreshVkDevice& device, const Desc& desc);

            Buffer(const Buffer&)                     = delete;
            auto operator=(const Buffer&) -> Buffer&  = delete;
            Buffer(const Buffer&&)                    = delete;
            auto operator=(const Buffer&&) -> Buffer& = delete;

            [[nodiscard]] auto handle() const -> vk::Buffer { return *m_buffer; }
            [[nodiscard]] auto size() const -> vk::DeviceSize { return m_size; }

            // 0 unless created with eShaderDeviceAddress usage.
            [[nodiscard]] auto device_address() const -> vk::DeviceAddress { return m_device_address; }

            // Empty span unless persistent_map was requested in Desc.
            [[nodiscard]] auto mapped() const -> std::span<std::byte> {
                return { static_cast<std::byte*>(m_mapped_memory), m_mapped_memory ? m_size : 0 };
            }

            // Trivially-copyable write into the persistent mapping. Host-coherent memory
            // assumed (no explicit flush); add a flush overload if a non-coherent heap appears.
            template <typename T>
            auto write(const T& value, const vk::DeviceSize offset = 0) -> void {
                static_assert(std::is_trivially_copyable_v<T>);
                assert(m_mapped_memory && "Buffer::write on an unmapped buffer");
                assert(offset + sizeof(T) <= m_size);
                std::memcpy(static_cast<std::byte*>(m_mapped_memory) + offset, &value, sizeof(T));
            }

        private:
            vk::raii::DeviceMemory m_memory    = nullptr;
            vk::raii::Buffer m_buffer          = nullptr;
            void* m_mapped_memory              = nullptr;
            vk::DeviceSize m_size              = 0;
            vk::DeviceAddress m_device_address = 0;
    };
} // flux
