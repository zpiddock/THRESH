//
// Created by Admin on 24/05/2026.
//

#include "debug_line_renderer.hpp"

namespace flux {
    DebugLineRenderer::DebugLineRenderer(VulkanContext& ctx) : m_context(ctx){

        // --- Vertex layout for {float3 pos, float3 colour} ---
        const std::vector<vk::VertexInputBindingDescription> bindings{
            { .binding = 0, .stride = sizeof(DebugLineVertex), .inputRate = vk::VertexInputRate::eVertex },
        };
        const std::vector<vk::VertexInputAttributeDescription> attribs{
            { .location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat,
              .offset = offsetof(DebugLineVertex, position) },
            { .location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat,
              .offset = offsetof(DebugLineVertex, colour) },
        };

        PipelineContext pipeline_context{

            .shader_path = "debug_lines.spv",
            .bindings = {},
            .push_constants = {{vk::ShaderStageFlagBits::eVertex, 0, sizeof(DebugLinePushConstants)}},
            .use_vertex_input = true,
            .depth_test = true,
            .depth_write = false,
            .cull_mode = vk::CullModeFlagBits::eNone,
            .colour_format = ctx.m_offscreen_format,
            .depth_format = ctx.m_vk_device.find_depth_format(),
            .topology = vk::PrimitiveTopology::eLineList,
            .vertex_bindings = bindings,
            .vertex_attributes = attribs
        };
        m_pipeline = ctx.register_pipeline("debug_lines", pipeline_context);

        constexpr auto buffer_size = sizeof(DebugLineVertex) * MAX_VERTICES;
        auto& device = ctx.m_vk_device;
        for (int i = 0; i < VulkanContext::MAX_FRAMES_IN_FLIGHT; ++i) {
            std::tie(m_vertex_buffers[i], m_vertex_buffer_memory[i]) = device.create_buffer(buffer_size, vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            m_mapped_memory[i] = m_vertex_buffer_memory[i].mapMemory(0, buffer_size);
        }
        m_pending.reserve(MAX_VERTICES);
    }

    auto DebugLineRenderer::submit_line(const flux::float3 p0, const flux::float3 p1, const flux::float3 colour) -> void {

        if (m_pending.size() + 2 > MAX_VERTICES) {
            return; // silently drop any overflow lines
        }
        m_pending.push_back({p0, colour});
        m_pending.push_back({p1, colour});
    }

    auto DebugLineRenderer::submit_aabb(const AABB& aabb, flux::float3 colour) -> void {

        const flux::float3 verts[8] = {
            {aabb.min.x, aabb.min.y, aabb.min.z}, {aabb.max.x, aabb.min.y, aabb.min.z},
            {aabb.max.x, aabb.max.y, aabb.min.z}, {aabb.min.x, aabb.max.y, aabb.min.z},
            {aabb.min.x, aabb.min.y, aabb.max.z}, {aabb.max.x, aabb.min.y, aabb.max.z},
            {aabb.max.x, aabb.max.y, aabb.max.z}, {aabb.min.x, aabb.max.y, aabb.max.z},
        };
        constexpr int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, // botton
            {4, 5}, {5, 6}, {6, 7}, {7, 4}, // top
            {0, 4}, {1, 5}, {2, 6}, {3, 7}, // verticals
        };
        for (const auto& [a, e] : edges) {
            submit_line(verts[a], verts[e], colour);
        }
    }

    auto DebugLineRenderer::clear() -> void {
        m_pending.clear();
    }

    auto DebugLineRenderer::record_frame(vk::raii::CommandBuffer& cmd, const flux::float4x4& view_proj,
        vk::ImageView colour_view, vk::ImageView depth_view, vk::Extent2D extents) -> void {

        if (m_pending.empty()) {
            return;
        }

        const auto frame = m_context.m_frame_index;
        std::memcpy(m_mapped_memory[frame], m_pending.data(), m_pending.size() * sizeof(DebugLineVertex));

        vk::RenderingAttachmentInfo colour_info{

            .imageView = colour_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore
        };
        vk::RenderingAttachmentInfo depth_info{

            .imageView = depth_view,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eDontCare
        };
        vk::RenderingInfo rendering_info{
            .renderArea = {.offset = {0, 0}, .extent = extents},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colour_info,
            .pDepthAttachment = &depth_info
        };
        cmd.beginRendering(rendering_info);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline->graphics_pipeline());
        cmd.setViewport(0,
            vk::Viewport{
            0,
            static_cast<float>(extents.height),    // y starts at bottom
            static_cast<float>(extents.width),
            -static_cast<float>(extents.height),   // negative height = Y-flip
            0, 1
        });
        cmd.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extents});

        DebugLinePushConstants push_constants{view_proj};
        cmd.pushConstants<DebugLinePushConstants>(*m_pipeline->pipeline_layout(), vk::ShaderStageFlagBits::eVertex, 0, push_constants);
        cmd.bindVertexBuffers(0, {m_vertex_buffers[frame]}, {0});
        cmd.draw(static_cast<uint32_t>(m_pending.size()), 1, 0, 0);
        cmd.endRendering();
    }
} // flux