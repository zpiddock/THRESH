#pragma once

#include "flux/command_buffer.hpp"
#include "flux/sync.hpp"
#include "flux/deletion_queue.hpp"

#include <array>
#include <cstdint>
#include <memory>

#ifdef _WIN32
#ifdef THRESH_EXPORTS
#define THRESH_API __declspec(dllexport)
#else
#define THRESH_API __declspec(dllimport)
#endif
#else
#define THRESH_API
#endif

namespace thresh {
    constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    /**
     * Per-frame GPU synchronization and command recording resources.
     * Each frame-in-flight gets its own set to avoid CPU-GPU conflicts.
     */
    struct THRESH_API FrameData {
        std::unique_ptr<flux::CommandBuffer> command_buffer;
        std::unique_ptr<flux::Semaphore> image_available_semaphore;
        std::unique_ptr<flux::Semaphore> render_finished_semaphore;
        std::unique_ptr<flux::Fence> in_flight_fence;
        std::unique_ptr<flux::DeletionQueue> deletion_queue;

        /**
         * Factory method to create a fully initialized FrameData.
         * @param device Vulkan logical device
         * @param queue_family_index Graphics queue family index
         * @return Initialized FrameData with all resources created
         */
        static auto create(VkDevice device, std::uint32_t queue_family_index) -> FrameData;
    };
} // namespace thresh
