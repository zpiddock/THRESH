//
// Created by Admin on 20/06/2026.
//

#pragma once
#include "gpu_data.hpp"
#include "vkbackend/buffer.hpp"

namespace flux {
    class ThreshVkDevice;

    class MaterialBuffer {

        public:
            MaterialBuffer() = default;
            MaterialBuffer(ThreshVkDevice& device, std::uint32_t capacity);

            auto register_material(const flux::gpu::MaterialData& data) -> std::uint32_t;

            [[nodiscard]] auto device_address() const -> vk::DeviceAddress { return m_storage.device_address(); }
            [[nodiscard]] auto count() const -> std::uint32_t { return m_count; }

        private:
            Buffer m_storage;
            std::uint32_t m_capacity = 0;
            std::uint32_t m_count    = 0;
    };
} // flux
