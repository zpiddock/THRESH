//
// Renderer — merge of the former GraphicsUtils + VulkanContext.
//

#include "renderer.hpp"

#include <cassert>
#include <format>

#include "substratum/log.hpp"

namespace flux {
    namespace {
        auto make_instance_ctx(const RenderConfig& c) -> VulkanInstanceContext {
            return {
                .application_name         = c.application_name,
                .enable_validation_layers = c.enable_validation_layers,
                .enable_sync_validation   = c.enable_sync_validation,
            };
        }
    } // namespace

    Renderer::Renderer(const thresh::Window& window, const RenderConfig& config)
        : m_config(config),
          m_window(&window),
          m_vk_instance(make_instance_ctx(config), window),
          m_vk_device(m_vk_instance.instance(), m_vk_instance.surface()),
          m_vk_swapchain(window, m_vk_instance, m_vk_device, config.present_mode),
          m_resource_heap(m_vk_device, DescriptorHeap::Kind::RESOURCE, config.resource_heap_capacity),
          m_sampler_heap(m_vk_device, DescriptorHeap::Kind::SAMPLER, config.sampler_heap_capacity),
          m_pipelines(m_vk_device),
          m_resources(m_vk_device, m_resource_heap, config.material_buffer_capacity) {

        m_default_sampler_slot = m_sampler_heap.allocate();
        m_sampler_heap.write_sampler(m_default_sampler_slot, vk::SamplerCreateInfo{
            .magFilter    = vk::Filter::eLinear, .minFilter = vk::Filter::eLinear,
            .mipmapMode   = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat,
        });

        create_depth_resources();
        create_offscreen_resources();
        m_pipelines.register_geometry_pipeline(m_offscreen_format, m_vk_device.find_depth_format());
        m_pipelines.register_composite_pipeline(m_vk_swapchain.swapchain_surface_format().format);
        create_frame_contexts();
        create_sync_objects();
    }

    Renderer::~Renderer() = default;

    // ---- backend setup (moved from VulkanContext) -------------------------------

    auto Renderer::create_depth_resources() -> void {
        const vk::Format format = m_vk_device.find_depth_format();
        m_depth_image = flux::Image(m_vk_device, {
            .extent = m_vk_swapchain.swapchain_extent(), .format = format,
            .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
            .aspect = vk::ImageAspectFlagBits::eDepth, .tiling = vk::ImageTiling::eOptimal,
            .memory = vk::MemoryPropertyFlagBits::eDeviceLocal, .mip_levels = 1, .debug_name = "Depth Image",
        });
    }

    auto Renderer::pick_offscreen_format() -> vk::Format {
        constexpr auto required = vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eColorAttachment;
        const auto props = m_vk_device.physical().getFormatProperties2(vk::Format::eR16G16B16A16Sfloat);
        if ((props.formatProperties.optimalTilingFeatures & required) == required) return vk::Format::eR16G16B16A16Sfloat;
        SUB_WARN("HDR eR16G16B16A16Sfloat unsupported as colour attachment, falling back to swapchain format.");
        return m_vk_swapchain.swapchain_surface_format().format;
    }

    auto Renderer::create_offscreen_resources() -> void {
        m_offscreen_images.clear();
        m_offscreen_format = pick_offscreen_format();
        const auto extent = m_vk_swapchain.swapchain_extent();
        m_offscreen_images.reserve(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto image = flux::Image(m_vk_device, {
                .extent = extent, .format = m_offscreen_format,
                .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                .aspect = vk::ImageAspectFlagBits::eColor, .tiling = vk::ImageTiling::eOptimal,
                .memory = vk::MemoryPropertyFlagBits::eDeviceLocal, .mip_levels = 1,
                .debug_name = std::format("Forward_Pass_Offscreen_Image_{}", i).c_str(),
            });
            if (m_offscreen_slots[i] == HEAP_INVALID_SLOT) m_offscreen_slots[i] = m_resource_heap.allocate();
            m_resource_heap.write_sampled_image(m_offscreen_slots[i], image.view_create_info(), vk::ImageLayout::eShaderReadOnlyOptimal);
            image.set_heap_index(DescriptorHeap::shader_index(m_offscreen_slots[i]));
            m_offscreen_images.emplace_back(std::move(image));
        }
    }

    auto Renderer::create_frame_contexts() -> void {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) m_frames[i] = FrameContext::create(m_vk_device);
    }

    auto Renderer::create_sync_objects() -> void {
        assert(m_render_complete_semaphores.empty());
        for (size_t i = 0; i < m_vk_swapchain.swapchain_images().size(); i++)
            m_render_complete_semaphores.emplace_back(m_vk_device.logical(), vk::SemaphoreCreateInfo{});
    }

    auto Renderer::advance_frame() -> void { m_frame_index = (m_frame_index + 1) % MAX_FRAMES_IN_FLIGHT; }

    // ---- frame loop -------------------------------------------------------------

    auto Renderer::draw_frame() -> void {
        const auto fence = *m_frames[m_frame_index].in_flight_fence;
        if (m_vk_device.logical().waitForFences(fence, vk::True, UINT64_MAX) != vk::Result::eSuccess)
            SUB_FATAL("Failed to wait for fence!");

        if (m_framebuffer_resized) {
            set_framebuffer_resized(false);
            recreate_swapchain();
            m_imgui_context.discard_frame();
            return;
        }

        auto present_semaphore = *m_frames[m_frame_index].present_complete;
        auto [result, image_index] = m_vk_swapchain.swapchain().acquireNextImage(UINT64_MAX, present_semaphore, nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreate_swapchain();
            m_imgui_context.discard_frame();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
            SUB_FATAL("Failed to acquire next image!");

        update_uniform_buffers(m_frame_index);
        m_vk_device.logical().resetFences(fence);

        auto& cmd = m_frames[m_frame_index].command_buffer;
        cmd.reset();
        record_command_buffers(cmd, image_index, m_draw_commands);

        auto render_semaphore = *m_render_complete_semaphores[image_index];
        vk::PipelineStageFlags wait_stage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo submit_info{
            .waitSemaphoreCount = 1, .pWaitSemaphores = &present_semaphore, .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1, .pCommandBuffers = &*cmd.raw(),
            .signalSemaphoreCount = 1, .pSignalSemaphores = &render_semaphore,
        };
        m_vk_device.graphics_queue().submit(submit_info, fence);

        const vk::PresentInfoKHR present_info{
            .waitSemaphoreCount = 1, .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1, .pSwapchains = &*m_vk_swapchain.swapchain(), .pImageIndices = &image_index,
        };
        try {
            result = m_vk_device.graphics_queue().presentKHR(present_info);
            if (result == vk::Result::eErrorOutOfDateKHR) { set_framebuffer_resized(false); recreate_swapchain(); return; }
        } catch (const vk::OutOfDateKHRError&) {
            recreate_swapchain();
            return;
        }
        advance_frame();
        m_draw_commands.clear();
    }

    auto Renderer::recreate_swapchain() -> void {
        SUB_DEBUG("Recreating swapchain (framebuffer resized)");
        m_vk_swapchain.recreate(*m_window, m_vk_instance, m_vk_device, m_config.present_mode);   // Stage 6 will thread config.present_mode here
        create_depth_resources();
        create_offscreen_resources();
    }

    auto Renderer::shutdown() -> void { SUB_INFO("Shutting down graphics"); m_vk_device.logical().waitIdle(); }

    auto Renderer::set_framebuffer_resized(bool resized) -> void { m_framebuffer_resized = resized; }
    auto Renderer::set_camera_data(const gpu::CameraData& camera_data) -> void { m_camera_data = camera_data; }
    auto Renderer::set_light_data(const gpu::LightData& light_data) -> void { m_light_data = light_data; }

    auto Renderer::update_uniform_buffers(const uint32_t frame_index) -> void {
        auto& frame = m_frames[frame_index];
        if (m_camera_data) frame.uniforms.write(*m_camera_data, FrameContext::CAMERA_OFFSET);
        if (m_light_data)  frame.uniforms.write(*m_light_data,  FrameContext::LIGHT_DATA_OFFSET);
    }

    auto Renderer::get_aspect_ratio() -> float {
        const auto e = m_vk_swapchain.swapchain_extent();
        return static_cast<float>(e.width) / static_cast<float>(e.height);
    }

    auto Renderer::submit_draw_command(const DrawCommand& draw_command) -> void { m_draw_commands.push_back(draw_command); }

    // ---- recording --------------------------------------------------------------

    auto Renderer::record_command_buffers(CommandBuffer& cmd_buffer, const uint32_t image_index, const std::vector<DrawCommand>& cmds) -> void {
        constexpr vk::CommandBufferBeginInfo begin_info{};
        cmd_buffer.begin(begin_info.flags);
        cmd_buffer.bind_heaps(m_resource_heap, m_sampler_heap);

        record_geometry_commands(cmd_buffer, cmds);

        if (m_debug_line_renderer && m_camera_data) {
            const auto view_projection = m_camera_data->projection * m_camera_data->view;
            m_debug_line_renderer->record_frame(cmd_buffer, m_frame_index, view_projection,
                m_offscreen_images[m_frame_index].view(), m_depth_image.view(), m_vk_swapchain.swapchain_extent());
        }

        cmd_buffer.transition(m_offscreen_images[m_frame_index], {
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead,
        });

        record_composite_commands(cmd_buffer, image_index);
        if (is_imgui_enabled()) record_imgui_commands(cmd_buffer, image_index);

        cmd_buffer.transition_raw(m_vk_swapchain.swapchain_images()[image_index], vk::ImageAspectFlagBits::eColor,
            { .layout = vk::ImageLayout::eColorAttachmentOptimal, .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput, .access = vk::AccessFlagBits2::eColorAttachmentWrite },
            { .layout = vk::ImageLayout::ePresentSrcKHR,          .stage = vk::PipelineStageFlagBits2::eBottomOfPipe,          .access = {} });

        cmd_buffer.end();
    }

    auto Renderer::record_geometry_commands(CommandBuffer& cmd_buffer, const std::vector<DrawCommand>& cmds) -> void {
        const auto  frame      = m_frame_index;
        const auto* frame_data = &m_frames[frame];
        auto*       pipeline   = m_pipelines.get("opaque_mesh");
        const auto  extent     = m_vk_swapchain.swapchain_extent();

        cmd_buffer.transition(m_offscreen_images[frame], {
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
            .stage  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .access = vk::AccessFlagBits2::eColorAttachmentWrite,
        });
        cmd_buffer.transition(m_depth_image, {
            .layout = vk::ImageLayout::eDepthAttachmentOptimal,
            .stage  = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        });

        const auto& clear = m_config.clear_colour;
        const vk::ClearValue clear_color = vk::ClearColorValue(clear.r, clear.g, clear.b, clear.a);
        constexpr vk::ClearValue depth_clear_value = vk::ClearDepthStencilValue(1.0f, 0);

        vk::RenderingAttachmentInfo colour_attach{
            .imageView = m_offscreen_images[frame].view(), .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear, .storeOp = vk::AttachmentStoreOp::eStore, .clearValue = clear_color,
        };
        vk::RenderingAttachmentInfo depth_attach{
            .imageView = m_depth_image.view(), .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear, .storeOp = vk::AttachmentStoreOp::eDontCare, .clearValue = depth_clear_value,
        };
        vk::RenderingInfo rendering_info{
            .renderArea = {.offset = {0, 0}, .extent = extent}, .layerCount = 1,
            .colorAttachmentCount = 1, .pColorAttachments = &colour_attach, .pDepthAttachment = &depth_attach,
        };

        cmd_buffer.raw().beginRendering(rendering_info);
        cmd_buffer.raw().bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline->graphics_pipeline());
        cmd_buffer.raw().setViewport(0, vk::Viewport{0, static_cast<float>(extent.height), static_cast<float>(extent.width), -static_cast<float>(extent.height), 0, 1});
        cmd_buffer.raw().setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extent});

        cmd_buffer.push_data(0, gpu::FramePushConstants{
            .camera          = frame_data->camera_address,
            .lights          = frame_data->light_data_address,
            .materials       = m_resources.material_buffer_address(),
            .default_sampler = gpu::DescriptorHandle::make(DescriptorHeap::shader_index(m_default_sampler_slot)),
        });
        for (const auto& cmd : cmds) {
            const auto* mesh     = m_resources.get_mesh_resource(cmd.mesh_handle);
            const auto* material = m_resources.get_material_resource(cmd.material_handle);

            cmd_buffer.push_data(gpu::DRAW_PUSH_OFFSET, gpu::DrawPushConstants{
                .model           = cmd.model,
                .colour_tint     = cmd.base_colour,
                .material_handle = material->gpu_index,
            });
            cmd_buffer.raw().bindVertexBuffers(0, {mesh->vertex.handle()}, {0});
            cmd_buffer.raw().bindIndexBuffer(mesh->index.handle(), 0, vk::IndexType::eUint32);
            cmd_buffer.raw().drawIndexed(mesh->index_count, 1, 0, 0, 0);
        }
        cmd_buffer.raw().endRendering();
    }

    auto Renderer::record_composite_commands(CommandBuffer& cmd_buffer, uint32_t image_index) -> void {
        const auto frame    = m_frame_index;
        const auto extent   = m_vk_swapchain.swapchain_extent();
        auto*      pipeline = m_pipelines.get("composite");

        cmd_buffer.transition_raw(m_vk_swapchain.swapchain_images()[image_index], vk::ImageAspectFlagBits::eColor,
            { .layout = vk::ImageLayout::eUndefined,             .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput, .access = {} },
            { .layout = vk::ImageLayout::eColorAttachmentOptimal, .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput, .access = vk::AccessFlagBits2::eColorAttachmentWrite });

        constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.f, 0.f, 0.f, 1.f);
        vk::RenderingAttachmentInfo colour_attach{
            .imageView = m_vk_swapchain.swapchain_image_views()[image_index], .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear, .storeOp = vk::AttachmentStoreOp::eStore, .clearValue = clear_color,
        };
        vk::RenderingInfo rendering_info{
            .renderArea = {.offset = {0, 0}, .extent = extent}, .layerCount = 1,
            .colorAttachmentCount = 1, .pColorAttachments = &colour_attach,
        };

        cmd_buffer.raw().beginRendering(rendering_info);
        cmd_buffer.raw().bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline->graphics_pipeline());
        cmd_buffer.raw().setViewport(0, vk::Viewport{0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1});
        cmd_buffer.raw().setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extent});
        cmd_buffer.push_data(0, gpu::CompositePushConstants{
            .colour_image = gpu::DescriptorHandle::make(m_offscreen_images[frame].heap_index()),
            .sampler      = gpu::DescriptorHandle::make(DescriptorHeap::shader_index(m_default_sampler_slot)),
        });
        cmd_buffer.raw().draw(3, 1, 0, 0);
        cmd_buffer.raw().endRendering();
    }

    auto Renderer::record_imgui_commands(CommandBuffer& cmd_buffer, const uint32_t image_index) -> void {
        const auto extent = m_vk_swapchain.swapchain_extent();
        vk::RenderingAttachmentInfo colour_attach{
            .imageView = m_vk_swapchain.swapchain_image_views()[image_index], .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad, .storeOp = vk::AttachmentStoreOp::eStore,
        };
        const vk::RenderingInfo rendering_info{
            .renderArea = {.offset = {0, 0}, .extent = extent}, .layerCount = 1,
            .colorAttachmentCount = 1, .pColorAttachments = &colour_attach,
        };
        cmd_buffer.raw().beginRendering(rendering_info);
        m_imgui_context.record_draw_data(cmd_buffer);
        cmd_buffer.raw().endRendering();
    }

    // ---- imgui / debug facade ---------------------------------------------------

    auto Renderer::imgui_init() -> void {
        // ImGuiInitInfo is DearImGuiContext's nested type — pass it as an unnamed braced-init.
        m_imgui_context.init(*m_window, {
            .instance           = m_vk_instance.instance(),
            .physical_device    = m_vk_device.physical(),
            .device             = m_vk_device.logical(),
            .queue_family_index = m_vk_device.queue_family_index(),
            .queue              = m_vk_device.graphics_queue(),
            .colour_format      = m_vk_swapchain.swapchain_surface_format().format,
            .image_count        = static_cast<uint32_t>(m_vk_swapchain.swapchain_images().size()),
            .min_image_count    = MAX_FRAMES_IN_FLIGHT,
        });
    }

    auto Renderer::imgui_shutdown() -> void { m_imgui_context.shutdown(); }
    auto Renderer::imgui_new_frame() -> bool { return m_imgui_context.new_frame(); }
    auto Renderer::imgui_process_event(const SDL_Event& event) -> void { m_imgui_context.process_event(event); }
    auto Renderer::imgui_enabled(const bool enabled) -> void { m_imgui_context.m_enabled = enabled; }
    auto Renderer::is_imgui_enabled() const -> bool { return m_imgui_context.m_enabled; }

    auto Renderer::enable_debug_line_renderer() -> void {
        if (m_debug_line_renderer) return;
        auto* pipeline = m_pipelines.register_pipeline("debug_lines",
            DebugLineRenderer::pipeline_context(m_offscreen_format, m_vk_device.find_depth_format()));
        m_debug_line_renderer = std::make_unique<DebugLineRenderer>(m_vk_device, *pipeline);   // ctor takes ThreshVkPipeline&
    }

    auto Renderer::debug_line_renderer() -> DebugLineRenderer* { return m_debug_line_renderer.get(); }
} // flux
