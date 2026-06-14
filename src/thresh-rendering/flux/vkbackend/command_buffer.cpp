//
// Created by Admin on 14/06/2026.
//

#include "command_buffer.hpp"

#include "buffer.hpp"
#include "image.hpp"

namespace flux {

    auto CommandBuffer::emit_barrier(vk::Image image, vk::ImageAspectFlags aspect, const ImageState& src,
        const ImageState& dst) {

        const vk::ImageMemoryBarrier2 barrier{
            .srcStageMask        = src.stage,  .srcAccessMask = src.access,
            .dstStageMask        = dst.stage,  .dstAccessMask = dst.access,
            .oldLayout           = src.layout, .newLayout     = dst.layout,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image               = image,
            .subresourceRange    = { .aspectMask     = aspect,
                                     .baseMipLevel   = 0, .levelCount = vk::RemainingMipLevels,
                                     .baseArrayLayer = 0, .layerCount = vk::RemainingArrayLayers },
        };
        m_command_buffer.pipelineBarrier2(vk::DependencyInfo{ .imageMemoryBarrierCount = 1,
                                                   .pImageMemoryBarriers = &barrier });
    }

    auto CommandBuffer::begin(vk::CommandBufferUsageFlags usage) -> void {
        m_command_buffer.begin(vk::CommandBufferBeginInfo{ .flags = usage });
    }

    auto CommandBuffer::end() -> void {
        m_command_buffer.end();
    }

    auto CommandBuffer::reset() -> void {
        m_command_buffer.reset();
    }

    auto CommandBuffer::transition(Image& image, const ImageState& dst) -> void {

        emit_barrier(image.handle(), image.aspect(), image.state(), dst);
        image.set_state(dst);
    }

    auto CommandBuffer::transition_raw(vk::Image image, vk::ImageAspectFlags aspect,
                                       const ImageState& src, const ImageState& dst) -> void {
        emit_barrier(image, aspect, src, dst);
    }

    auto CommandBuffer::copy_buffer(const Buffer& src, const Buffer& dst, const vk::DeviceSize size) const -> void {
        m_command_buffer.copyBuffer(src.handle(), dst.handle(), vk::BufferCopy{.size = size});
    }

    auto CommandBuffer::copy_buffer_to_image(const Buffer& src, const Image& dst) const -> void {

        // Image must already be in eTransferDstOptimal (caller transitions first).
        const vk::BufferImageCopy region{
            .imageSubresource = { .aspectMask = dst.aspect(), .mipLevel = 0,
                                  .baseArrayLayer = 0, .layerCount = 1 },
            .imageExtent      = { dst.extent().width, dst.extent().height, 1 },
        };
        m_command_buffer.copyBufferToImage(src.handle(), dst.handle(),
                                vk::ImageLayout::eTransferDstOptimal, region);
    }
} // flux