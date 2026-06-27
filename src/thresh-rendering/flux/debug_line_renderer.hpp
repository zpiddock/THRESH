//
// Created by Admin on 24/05/2026.
//

#pragma once
#include <array>
#include <cstddef>
#include <vector>

#include "helix/math.hpp"
#include "vkbackend/buffer.hpp"
#include "vkbackend/frame_context.hpp"   // flux::MAX_FRAMES_IN_FLIGHT
#include "vkbackend/vulkan_pipeline.hpp"

namespace flux {

    struct DebugLineVertex {
        helix::float3 position;
        helix::float3 colour;
    };

    struct DebugLinePushConstants {
        helix::float4x4 view_proj;
    };

    class DebugLineRenderer {

        public:
            explicit DebugLineRenderer(ThreshVkDevice& device, ThreshVkPipeline& pipeline);

            auto submit_line(helix::float3 p0, helix::float3 p1, helix::float3 colour = helix::float3{1.f}) -> void;

            auto submit_aabb(const helix::AABB& aabb, helix::float3 colour = helix::float3{1.f}) -> void;

            auto clear() -> void;

            auto record_frame(
                CommandBuffer& cmd,
                uint32_t frame_index,
                const helix::float4x4& view_proj,
                vk::ImageView colour_view,
                vk::ImageView depth_view,
                vk::Extent2D extents
                ) -> void;

            static auto pipeline_context(vk::Format colour, vk::Format depth) -> PipelineContext;

        private:

            static constexpr std::size_t MAX_VERTICES = 16384;

            ThreshVkPipeline&            m_pipeline;
            std::vector<DebugLineVertex> m_pending{};
            std::array<Buffer, 2>        m_vertex_buffers{};
    };
} // flux
