#pragma once

#include <cstdint>

namespace thresh {
    /**
     * Snapshot of frame state produced by the update thread and consumed by the render thread.
     * Triple-buffered for lock-free communication between threads.
     */
    struct FrameSnapshot {
        std::uint64_t frame_number = 0;
        float delta_time = 0.0f;
        float total_time = 0.0f;
        // TODO: transforms, draw commands, camera data
    };
} // namespace thresh
