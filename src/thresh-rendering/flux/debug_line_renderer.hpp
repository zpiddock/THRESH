//
// Created by Admin on 24/05/2026.
//

#pragma once
#include "vkbackend/buffer.hpp"
#include "vkbackend/vulkan_context.hpp"

namespace flux {

    struct DebugLineVertex {
        flux::float3 position;
        flux::float3 colour;
    };

    struct DebugLinePushConstants {
        flux::float4x4 view_proj;
    };

    class DebugLineRenderer {

        public:
            explicit DebugLineRenderer(VulkanContext& ctx);

            auto submit_line(flux::float3 p0, flux::float3 p1, flux::float3 colour = flux::float3{1.f}) -> void;

            auto submit_aabb(const AABB& aabb, flux::float3 colour = flux::float3{1.f}) -> void;

            auto clear() -> void;

            auto record_frame(
                vk::raii::CommandBuffer& cmd,
                const flux::float4x4& view_proj,
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
