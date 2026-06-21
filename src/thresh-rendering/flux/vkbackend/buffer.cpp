//
// Created by Admin on 13/06/2026.
//

#include "buffer.hpp"

#include "vulkan_device.hpp"

namespace flux {
    Buffer::Buffer(ThreshVkDevice& device, const Desc& desc) : m_size(desc.size) {

        m_buffer = vk::raii::Buffer(device.logical(), vk::BufferCreateInfo{
            .size = desc.size,
            .usage = desc.usage,
            .sharingMode = vk::SharingMode::eExclusive
        });

        const auto requirements = m_buffer.getMemoryRequirements();
        const auto wants_address =
            static_cast<bool>(desc.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) ? WantsDeviceAddress::YES : WantsDeviceAddress::NO;

        m_memory = device.allocate_memory(requirements, desc.memory, wants_address);
        m_buffer.bindMemory(*m_memory, 0);

        if (desc.persistent_map) {
            m_mapped_memory = m_memory.mapMemory(0, desc.size);
        }
        if (wants_address == WantsDeviceAddress::YES) {
            m_device_address = device.logical().getBufferAddress({.buffer = *m_buffer});
        }
        if (desc.debug_name) {
            device.set_debug_name(
                vk::ObjectType::eBuffer,
                reinterpret_cast<uint64_t>(static_cast<VkBuffer>(*m_buffer)),
                desc.debug_name
                );
        }
    }
} // flux