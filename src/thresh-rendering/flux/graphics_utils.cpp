//
// flux.cpp — Graphics utilities for the Vulkan rendering backend.
//

#include "graphics_utils.hpp"

#define GLM_FORCE_RADIANS
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics_types.hpp"
#include "horizon/window.hpp"
#include "SDL3/SDL_events.h"
#include "substratum/log.hpp"
#include "substratum/filesystem/vfs.hpp"

namespace flux {
    auto GraphicsUtils::vulkan_init(const VulkanInstanceContext& ctx, const thresh::Window& window) -> void {
        m_window = &window;
        m_context = std::make_unique<VulkanContext>(ctx, window);
    }

    auto GraphicsUtils::vulkan_init(const thresh::Window& window) -> void {

        const VulkanInstanceContext ctx = {
            .application_name = "Thresh Application",
            .engine_name = "THRΞSH",
            .engine_version = "0.0.1",
            .application_version = "0.0.1",
            .enable_validation_layers = true,
            .enable_sync_validation = true,
        };
        vulkan_init(ctx, window);
    }

    auto GraphicsUtils::get_vulkan_context() -> VulkanContext* {

        return m_context.get();
    }

    auto GraphicsUtils::draw_frame() -> void {

        const auto fence = *m_context->m_frames[m_context->m_frame_index].in_flight_fence;
        auto fence_result = m_context->m_vk_device.logical().waitForFences(fence, vk::True, UINT64_MAX);
        if (fence_result != vk::Result::eSuccess) {
            SUB_FATAL("Failed to wait for fence!");
        }

        if (m_framebuffer_resized) {
            set_framebuffer_resized(false);
            recreate_swapchain();
            m_imgui_context.discard_frame();
            return;
        }

        auto present_semaphore = *m_context->m_frames[m_context->m_frame_index].present_complete;
        auto [result, image_index] =
            m_context->m_vk_swapchain.swapchain().acquireNextImage(
                UINT64_MAX, present_semaphore, nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreate_swapchain();
            m_imgui_context.discard_frame();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            SUB_FATAL("Failed to acquire next image!");
        }

        update_uniform_buffers(m_context->m_frame_index);

        m_context->m_vk_device.logical().resetFences(fence);

        auto& cmd = m_context->m_frames[m_context->m_frame_index].command_buffer;
        cmd.reset();
        record_command_buffers(cmd, image_index, m_draw_commands);

        auto render_semaphore = *m_context->m_render_complete_semaphores[image_index];
        vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo submit_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &present_semaphore,
            .pWaitDstStageMask = &wait_destination_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*cmd.raw(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_semaphore
        };

        m_context->m_vk_device.graphics_queue().submit(submit_info, fence);

        const vk::PresentInfoKHR present_info = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*m_context->m_vk_swapchain.swapchain(),
            .pImageIndices = &image_index,
        };

        try {
            result = m_context->m_vk_device.graphics_queue().presentKHR(present_info);
            if (result == vk::Result::eErrorOutOfDateKHR) {
                set_framebuffer_resized(false);
                recreate_swapchain();
                return;
            }
        } catch (const vk::OutOfDateKHRError& e) {
            recreate_swapchain();
            return;
        }
        m_context->m_frame_index = (m_context->m_frame_index + 1) % VulkanContext::MAX_FRAMES_IN_FLIGHT;
        m_draw_commands.clear();
    }

    auto GraphicsUtils::recreate_swapchain() -> void {

        SUB_DEBUG("Recreating swapchain (framebuffer resized)");
        m_context->m_vk_swapchain.recreate(*m_window, m_context->m_vk_instance, m_context->m_vk_device);
        m_context->create_depth_resources();
        m_context->create_offscreen_resources();
    }

    auto GraphicsUtils::shutdown() -> void {

        SUB_INFO("Shutting down graphics");
        m_context->m_vk_device.logical().waitIdle();
    }

    auto GraphicsUtils::set_framebuffer_resized(bool resized) -> void {
        m_framebuffer_resized = resized;
    }

    auto GraphicsUtils::set_camera_data(const gpu::CameraData& camera_data) -> void {
        m_camera_data = camera_data;
    }

    auto GraphicsUtils::set_light_data(const gpu::LightData& light_data) -> void {
        m_light_data = light_data;
    }

    auto GraphicsUtils::update_uniform_buffers(const uint32_t frame_index) -> void {

        auto& frame = m_context->m_frames[frame_index];

        if (m_camera_data != std::nullopt) {

            frame.uniforms.write(*m_camera_data, FrameContext::CAMERA_OFFSET);
        }
        if (m_light_data != std::nullopt) {
            frame.uniforms.write(*m_light_data, FrameContext::LIGHT_DATA_OFFSET);
        }
    }

    auto GraphicsUtils::get_aspect_ratio() -> float {

        return static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().width) / static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().height);
    }

    auto GraphicsUtils::create_mesh_resource(const MeshData& mesh_data) -> MeshResource {

        auto& device = m_context->m_vk_device;

        MeshResource result;
        for (const auto& vertex : mesh_data.vertices) {
            result.local_aabb.expand(vertex.position);
        }

        result.vertex = device.upload_device_local(std::as_bytes(std::span{mesh_data.vertices}), vk::BufferUsageFlagBits::eVertexBuffer, "Vertex Buffer");
        result.index = device.upload_device_local(std::as_bytes(std::span{mesh_data.indices}), vk::BufferUsageFlagBits::eIndexBuffer, "Index Buffer");

        result.index_count = mesh_data.indices.size();

        return result;
    }

    auto GraphicsUtils::create_texture_resource(const std::string& path) -> TextureResource {

        int width, height, nrChannels;

        const auto texture_data = substratum::VFS::read_file(path);

        stbi_uc* data = stbi_load_from_memory(texture_data.data(), texture_data.size(), &width, &height, &nrChannels, STBI_rgb_alpha);

        size_t size = width * height * STBI_rgb_alpha;

        TextureData texture = {

            .width = width,
            .height = height,
            .num_channels = nrChannels,
            .pixel_data = std::span(data, size)
        };

        TextureResource result = create_texture_resource(texture);
        stbi_image_free(data);
        return result;
    }

    auto GraphicsUtils::create_texture_resource(const TextureData& texture_data) -> TextureResource {

        auto& device = m_context->m_vk_device;

        auto image = device.upload_image(std::as_bytes(texture_data.pixel_data),
        vk::Extent2D{ static_cast<uint32_t>(texture_data.width),
                      static_cast<uint32_t>(texture_data.height) },
        vk::Format::eR8G8B8A8Srgb,
        nullptr);

        return TextureResource{
            .image        = std::move(image),
        };
    }

    auto GraphicsUtils::register_mesh(const MeshData& data) -> std::uint32_t {

        MeshResource mesh_resource = create_mesh_resource(data);
        m_mesh_resources.emplace_back(std::move(mesh_resource));
        const auto handle = static_cast<std::uint32_t>(m_mesh_resources.size());
        SUB_TRACE("Registered mesh handle {} ({} vertices, {} indices)",
                  handle, data.vertices.size(), data.indices.size());
        return handle;
    }

    auto GraphicsUtils::register_texture(const std::string& path) -> std::uint32_t {

        TextureResource texture_resource = create_texture_resource(path);

        const HeapSlot slot = m_context->m_resource_heap.allocate();
        m_context->m_resource_heap.write_sampled_image(slot, texture_resource.image.view_create_info(), vk::ImageLayout::eShaderReadOnlyOptimal);
        texture_resource.image.set_heap_index(DescriptorHeap::shader_index(slot));

        m_texture_resources.emplace_back(std::move(texture_resource));
        const auto handle = static_cast<std::uint32_t>(m_texture_resources.size());
        SUB_TRACE("Registered texture handle {} from '{}'", handle, path);
        return handle;
    }

    auto GraphicsUtils::register_material(const std::uint32_t& texture_handle,
    const flux::float4 base_colour,
            const std::string& material_type) -> std::uint32_t {

        MaterialResource material;
        material.material_type = material_type;
        material.albedo_texture_handle = texture_handle;
        material.albedo_tint = base_colour;
        m_material_resources.emplace_back(std::move(material));
        const auto handle = static_cast<std::uint32_t>(m_material_resources.size());
        SUB_TRACE("Registered material handle {} (type='{}', albedo_tex={})",
                  handle, material_type, texture_handle);
        return handle;
    }

    auto GraphicsUtils::submit_draw_command(const DrawCommand& draw_command) -> void {

        m_draw_commands.push_back(draw_command);
    }

    auto GraphicsUtils::get_mesh_resource(const std::uint32_t mesh_id) const -> const MeshResource* {

        if (mesh_id == 0 || mesh_id > m_mesh_resources.size()) return nullptr;
        return &m_mesh_resources[mesh_id - 1];
    }

    auto GraphicsUtils::get_texture_resource(const std::uint32_t texture_id) const -> const TextureResource* {
        if (texture_id == 0 || texture_id > m_texture_resources.size()) return nullptr;
        return &m_texture_resources[texture_id - 1];
    }

    auto GraphicsUtils::get_material_resource(const std::uint32_t material_id) const -> const MaterialResource* {
        if (material_id == 0 || material_id > m_material_resources.size()) return nullptr;
        return &m_material_resources[material_id - 1];
    }

    auto GraphicsUtils::get_draw_commands() -> std::vector<DrawCommand>& {

        return m_draw_commands;
    }

    auto GraphicsUtils::record_command_buffers(flux::CommandBuffer& cmd_buffer, const uint32_t image_index, const std::vector<DrawCommand>& cmds) -> void {

        constexpr vk::CommandBufferBeginInfo begin_info{};
        cmd_buffer.begin(begin_info.flags);

        cmd_buffer.bind_heaps(m_context->m_resource_heap, m_context->m_sampler_heap);

        record_geometry_commands(cmd_buffer, cmds);

        if (m_debug_line_renderer && m_camera_data) {

            const auto view_projection = m_camera_data->projection * m_camera_data->view;
            m_debug_line_renderer->record_frame(cmd_buffer, view_projection, m_context->m_offscreen_images[m_context->m_frame_index].view(), m_context->m_depth_image.view(), m_context->m_vk_swapchain.swapchain_extent());
        }

        cmd_buffer.transition(m_context->m_offscreen_images[m_context->m_frame_index],
            {
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderRead
            });

        record_composite_commands(cmd_buffer, image_index);

        if (is_imgui_enabled()) {
            record_imgui_commands(cmd_buffer, image_index);
        }

        cmd_buffer.transition_raw(
            m_context->m_vk_swapchain.swapchain_images()[image_index],
            vk::ImageAspectFlagBits::eColor,
            {
                .layout = vk::ImageLayout::eColorAttachmentOptimal,
                .stage  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .access = vk::AccessFlagBits2::eColorAttachmentWrite
            },
            {
                .layout = vk::ImageLayout::ePresentSrcKHR,
                .stage  = vk::PipelineStageFlagBits2::eBottomOfPipe,
                .access = {}
            });

        cmd_buffer.end();
    }

    auto GraphicsUtils::record_geometry_commands(flux::CommandBuffer& cmd_buffer,
        const std::vector<DrawCommand>& cmds) -> void {

        const auto frame = m_context->m_frame_index;
        const auto frame_data = &m_context->m_frames[frame];
        auto* pipeline = m_context->get_pipeline("opaque_mesh");
        const auto extent = m_context->m_vk_swapchain.swapchain_extent();

        // Transition offscreen image to colour attachment
        cmd_buffer.transition(m_context->m_offscreen_images[frame], {
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .access = vk::AccessFlagBits2::eColorAttachmentWrite
        });

        cmd_buffer.transition(m_context->m_depth_image, {
            .layout = vk::ImageLayout::eDepthAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite
        });

        constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.1f, 0.1f, 0.1f, 1.f);
        constexpr vk::ClearValue depth_clear_value = vk::ClearDepthStencilValue(1.0f, 0);

        vk::RenderingAttachmentInfo colour_attach {
            .imageView = m_context->m_offscreen_images[frame].view(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clear_color
        };
        vk::RenderingAttachmentInfo depth_attach {
            .imageView = m_context->m_depth_image.view(),
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .clearValue = depth_clear_value
        };
        vk::RenderingInfo rendering_info {
            .renderArea = {.offset = {0, 0}, .extent = extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colour_attach,
            .pDepthAttachment = &depth_attach
        };

        cmd_buffer.raw().beginRendering(rendering_info);
        cmd_buffer.raw().bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline->graphics_pipeline());
        cmd_buffer.raw().setViewport(0, vk::Viewport{0, static_cast<float>(extent.height), static_cast<float>(extent.width), -static_cast<float>(extent.height), 0, 1});
        cmd_buffer.raw().setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extent});

        std::uint32_t prev_material = 0;
        for (const auto& cmd : cmds) {
            // SUB_TRACE("{}:{}", cmd.mesh_handle, cmd.material_handle);
            const auto* mesh = get_mesh_resource(cmd.mesh_handle);
            const auto* material = get_material_resource(cmd.material_handle);
            const auto* albedo = get_texture_resource(material->albedo_texture_handle);

            const gpu::PushConstants push_constants {
                cmd.model,
                cmd.base_colour,
                frame_data->camera_address,
                frame_data->light_data_address,
                flux::gpu::DescriptorHandle::make(albedo->image.heap_index()),
                flux::gpu::DescriptorHandle::make(m_context->default_sampler_index())
            };
            cmd_buffer.push_data(0, push_constants);
            cmd_buffer.raw().bindVertexBuffers(0, {mesh->vertex.handle()}, {0});
            cmd_buffer.raw().bindIndexBuffer(mesh->index.handle(), 0, vk::IndexType::eUint32);
            cmd_buffer.raw().drawIndexed(mesh->index_count, 1, 0, 0, 0);
        }
        cmd_buffer.raw().endRendering();
    }

    auto GraphicsUtils::record_composite_commands(flux::CommandBuffer& cmd_buffer, uint32_t image_index) -> void {

        const auto frame = m_context->m_frame_index;
        const auto extent = m_context->m_vk_swapchain.swapchain_extent();
        const auto& pipeline = m_context->get_pipeline("composite");

        cmd_buffer.transition_raw(
            m_context->m_vk_swapchain.swapchain_images()[image_index],
            vk::ImageAspectFlagBits::eColor,
            {
                .layout = vk::ImageLayout::eUndefined,
                .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .access = {}
            },
            {
                .layout = vk::ImageLayout::eColorAttachmentOptimal,
                .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .access = vk::AccessFlagBits2::eColorAttachmentWrite
            });

        constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.f, 0.f, 0.f, 1.f);
        vk::RenderingAttachmentInfo colour_attach {
            .imageView = m_context->m_vk_swapchain.swapchain_image_views()[image_index],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clear_color
        };
        vk::RenderingInfo rendering_info {
            .renderArea = {.offset = {0, 0}, .extent = extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colour_attach,
        };

        cmd_buffer.raw().beginRendering(rendering_info);
        cmd_buffer.raw().bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline->graphics_pipeline());
        cmd_buffer.raw().setViewport(0, vk::Viewport{0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1});
        cmd_buffer.raw().setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extent});
        cmd_buffer.push_data(
            0,
            flux::gpu::CompositePushConstants{
                .colour_image = flux::gpu::DescriptorHandle::make(m_context->m_offscreen_images[frame].heap_index()),
                .sampler      = flux::gpu::DescriptorHandle::make(m_context->default_sampler_index())
            });
        cmd_buffer.raw().draw(3, 1, 0, 0);
        cmd_buffer.raw().endRendering();
    }

    auto GraphicsUtils::record_imgui_commands(flux::CommandBuffer& cmd_buffer, const uint32_t image_index) -> void {

        const auto extent = m_context->m_vk_swapchain.swapchain_extent();
        vk::RenderingAttachmentInfo colour_attach {
            .imageView = m_context->m_vk_swapchain.swapchain_image_views()[image_index],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
        };
        const vk::RenderingInfo rendering_info {
            .renderArea = {.offset = {0, 0}, .extent = extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colour_attach,
        };
        cmd_buffer.raw().beginRendering(rendering_info);
        m_imgui_context.record_draw_data(cmd_buffer);
        cmd_buffer.raw().endRendering();
    }

    auto GraphicsUtils::imgui_init() -> void {
        m_imgui_context.init(*m_window, *m_context);
    }

    auto GraphicsUtils::imgui_shutdown() -> void {
        m_imgui_context.shutdown();
    }

    auto GraphicsUtils::imgui_new_frame() -> bool {
        return m_imgui_context.new_frame();
    }

    auto GraphicsUtils::imgui_process_event(const SDL_Event& event) -> void {
        m_imgui_context.process_event(event);
    }

    auto GraphicsUtils::imgui_enabled(const bool enabled) -> void {
        m_imgui_context.m_enabled = enabled;
    }

    auto GraphicsUtils::is_imgui_enabled() const -> bool {
        return m_imgui_context.m_enabled;
    }

    auto GraphicsUtils::enable_debug_line_renderer() -> void {
        if (m_debug_line_renderer) {
            return; // Already enabled
        }
        m_debug_line_renderer = std::make_unique<DebugLineRenderer>(*m_context);
    }

    auto GraphicsUtils::debug_line_renderer() -> DebugLineRenderer* {
        if (!m_debug_line_renderer) {
            return nullptr;
        }
        return m_debug_line_renderer.get();
    }
} // namespace flux
