//
// Created by Admin on 20/06/2026.
//

#include "material_buffer.hpp"

#include "substratum/log.hpp"

namespace flux {
    MaterialBuffer::MaterialBuffer(ThreshVkDevice& device, const std::uint32_t capacity) : m_capacity(capacity) {

        m_storage = Buffer(device, {
            .size = vk::DeviceSize{capacity} * sizeof(flux::gpu::MaterialData),
            .usage = vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .memory = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            .persistent_map = true,
            .debug_name = "Material Buffer"
        });
    }

    auto MaterialBuffer::register_material(const flux::gpu::MaterialData& data) -> std::uint32_t {

        if (m_count >= m_capacity) {
            SUB_FATAL("Material buffer exhausted (capacity {})", m_capacity);
        }

        const uint32_t index = m_count++;
        m_storage.write(data, index * sizeof(flux::gpu::MaterialData));
        return index;
    }
} // flux