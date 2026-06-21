//
// Created by Admin on 24/05/2026.
//

#pragma once
#include "vkbackend/buffer.hpp"
#include "vkbackend/vulkan_context.hpp"

namespace helix {

    struct DebugLineVertex {
        helix::float3 position;
        helix::float3 colour;
    };

    struct DebugLinePushConstants {
        helix::float4x4 view_proj;
    };

    class DebugLineRenderer {

        public:
            explicit DebugLineRenderer(VulkanContext& ctx);

            auto submit_line(helix::float3 p0, helix::float3 p1, helix::float3 colour = helix::float3{1.f}) -> void;

            auto submit_aabb(const AABB& aabb, helix::float3 colour = helix::float3{1.f}) -> void;

            auto clear() -> void;

            auto record_frame(
                helix::CommandBuffer& cmd,
                const helix::float4x4& view_proj,
                vk::ImageView colour_view,
                vk::ImageView depth_view,
                vk::Extent2D extents
                ) -> void;

        private:
            static constexpr std::size_t MAX_VERTICES = 16384;

            VulkanContext&               m_context;
            ThreshVkPipeline*            m_pipeline;
            std::vector<DebugLineVertex> m_pending{};
            std::array<Buffer, 2>        m_vertex_buffers{};
    };
} // flux
