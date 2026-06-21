//
// Created by Admin on 13/06/2026.
//

#include "image.hpp"

namespace flux {
    Image::Image(ThreshVkDevice& device, const Desc& desc) : m_desc(desc) {

        m_image = vk::raii::Image(device.logical(), vk::ImageCreateInfo{
            .imageType   = vk::ImageType::e2D,
            .format      = desc.format,
            .extent      = { desc.extent.width, desc.extent.height, 1 },
            .mipLevels   = desc.mip_levels,
            .arrayLayers = 1,
            .samples     = vk::SampleCountFlagBits::e1,
            .tiling      = desc.tiling,
            .usage       = desc.usage,
            .sharingMode = vk::SharingMode::eExclusive,
        });

        const auto reqs = m_image.getMemoryRequirements();
        m_memory = device.allocate_memory(reqs, desc.memory, WantsDeviceAddress::NO);
        m_image.bindMemory(*m_memory, 0);

        m_view = vk::raii::ImageView(device.logical(), view_create_info());

        // m_state stays default (eUndefined / eTopOfPipe / {}); the first
        // CommandBuffer::transition supplies the real initial layout.

        if (desc.debug_name) {
            device.set_debug_name(
                vk::ObjectType::eImage,
                reinterpret_cast<uint64_t>(static_cast<VkImage>(*m_image)),
                desc.debug_name
                );
        }
    }
} // flux