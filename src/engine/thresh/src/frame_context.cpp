#include "thresh/renderer/frame_context.hpp"

namespace thresh {
    auto FrameData::create(VkDevice device, std::uint32_t queue_family_index) -> FrameData {
        FrameData frame{};

        frame.command_buffer = std::make_unique<flux::CommandBuffer>(flux::CommandBuffer::Config{
            .device = device,
            .queue_family_index = queue_family_index,
            .buffer_count = 1
        });

        frame.image_available_semaphore = std::make_unique<flux::Semaphore>(flux::Semaphore::Config{
            .device = device
        });

        frame.render_finished_semaphore = std::make_unique<flux::Semaphore>(flux::Semaphore::Config{
            .device = device
        });

        frame.in_flight_fence = std::make_unique<flux::Fence>(flux::Fence::Config{
            .device = device,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        });

        frame.deletion_queue = std::make_unique<flux::DeletionQueue>(device, MAX_FRAMES_IN_FLIGHT);

        return frame;
    }
} // namespace thresh
