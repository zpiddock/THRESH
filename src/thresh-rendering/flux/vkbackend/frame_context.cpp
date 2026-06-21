//
// Created by Admin on 14/06/2026.
//

#include "frame_context.hpp"

#include "vulkan_device.hpp"

namespace helix {
    auto FrameContext::create(ThreshVkDevice& device) -> FrameContext {

        FrameContext frame;

        frame.uniforms = Buffer(device, {
            .size = LIGHT_DATA_OFFSET + sizeof(helix::gpu::LightData),
            .usage = vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .memory = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            .persistent_map = true,
            .debug_name = "Frame Uniforms"
        });
        frame.camera_address = frame.uniforms.device_address() + CAMERA_OFFSET;
        frame.light_data_address = frame.uniforms.device_address() + LIGHT_DATA_OFFSET;

        // Persistent primary command buffer
        const vk::CommandBufferAllocateInfo alloc_info{
            .commandPool = device.command_pool(),
            .level       = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        frame.command_buffer = helix::CommandBuffer{std::move(device.logical().allocateCommandBuffers(alloc_info).front())};

        frame.present_complete = vk::raii::Semaphore(device.logical(), vk::SemaphoreCreateInfo{});
        frame.in_flight_fence  = vk::raii::Fence(device.logical(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});

        return frame;
    }
} // flux