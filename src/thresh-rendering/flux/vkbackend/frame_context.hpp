//
// Created by Admin on 14/06/2026.
//

#pragma once
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "command_buffer.hpp"
#include "flux/gpu_data.hpp"

namespace helix {
    class ThreshVkDevice;

    struct FrameContext {

        static constexpr vk::DeviceSize CAMERA_OFFSET = 0;
        static constexpr vk::DeviceSize LIGHT_DATA_OFFSET = (sizeof(helix::gpu::CameraData) + 15) & ~vk::DeviceSize{15};

        helix::Buffer uniforms;
        vk::DeviceAddress camera_address;
        vk::DeviceAddress light_data_address;

        CommandBuffer command_buffer;
        vk::raii::Semaphore present_complete = nullptr;
        vk::raii::Fence in_flight_fence = nullptr;

        static auto create(ThreshVkDevice& device) -> FrameContext;
    };
} // flux
