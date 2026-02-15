#pragma once

#include "render_data.hpp"

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

        /// Render data extracted from the scene by the update thread.
        FrameRenderData render_data;
    };
} // namespace thresh
