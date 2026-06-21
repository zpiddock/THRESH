//
// Created by Admin on 13/06/2026.
//

#include "buffer.hpp"

#include "vulkan_device.hpp"

namespace helix {
    Buffer::Buffer(ThreshVkDevice& device, const Desc& desc) : m_size(desc.size) {

        m_buffer = vk::raii::Buffer(device.logical(), vk::BufferCreateInfo{
            .size = desc.size,
            .usage = desc.usage,
            .sharingMode = vk::SharingMode::eExclusive
        });

        const auto requirements = m_buffer.getMemoryRequirements();
        const auto wants_address = static_cast<bool>(desc.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress);

        const vk::MemoryAllocateFlagsInfo address_flags {
            .flags = vk::MemoryAllocateFlagBits::eDeviceAddress
        };

        m_memory = vk::raii::DeviceMemory(device.logical(), vk::MemoryAllocateInfo{
            .pNext = wants_address ? &address_flags : nullptr,
            .allocationSize = requirements.size,
            .memoryTypeIndex = device.find_memory_type(requirements.memoryTypeBits, desc.memory)
        });
        m_buffer.bindMemory(*m_memory, 0);

        if (desc.persistent_map) {
            m_mapped_memory = m_memory.mapMemory(0, desc.size);
        }
        if (wants_address) {
            m_device_address = device.logical().getBufferAddress({.buffer = *m_buffer});
        }
        if (desc.debug_name) {
            device.logical().setDebugUtilsObjectNameEXT({
                .objectType = vk::ObjectType::eBuffer,
                .objectHandle = reinterpret_cast<uint64_t>(static_cast<VkBuffer>(*m_buffer)),
                .pObjectName = desc.debug_name
            });
        }
    }
} // flux