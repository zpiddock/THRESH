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
    auto GraphicsUtils::init_vulkan(const VulkanInstanceContext& ctx, const thresh::Window& window) -> void {
        m_window = &window;
        m_context = std::make_unique<VulkanContext>(ctx, window);
    }

    auto GraphicsUtils::init_vulkan(const thresh::Window& window) -> void {

        const VulkanInstanceContext ctx = {
            .application_name = "Thresh Application",
            .engine_name = "THRΞSH",
            .engine_version = "0.0.1",
            .application_version = "0.0.1"
        };
        init_vulkan(ctx, window);
    }

    auto GraphicsUtils::get_vulkan_context() -> VulkanContext* {

        return m_context.get();
    }

    auto GraphicsUtils::draw_frame() -> void {

        const auto fence = *m_context->m_inflight_fences[m_context->m_frame_index];
        auto fence_result = m_context->m_vk_device.logical().waitForFences(fence, vk::True, UINT64_MAX);
        if (fence_result != vk::Result::eSuccess) {
            SUB_FATAL("Failed to wait for fence!");
        }

        if (m_framebuffer_resized) {
            set_framebuffer_resized(false);
            recreate_swapchain();
            return;
        }

        auto present_semaphore = *m_context->m_present_complete_semaphores[m_context->m_frame_index];
        auto [result, image_index] =
            m_context->m_vk_swapchain.swapchain().acquireNextImage(
                UINT64_MAX, present_semaphore, nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreate_swapchain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            SUB_FATAL("Failed to acquire next image!");
        }

        update_uniform_buffers(m_context->m_frame_index);

        m_context->m_vk_device.logical().resetFences(fence);

        m_context->m_command_buffers[m_context->m_frame_index].reset();
        record_command_buffers(image_index, m_draw_commands);

        auto render_semaphore = *m_context->m_render_complete_semaphores[image_index];
        vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo submit_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &present_semaphore,
            .pWaitDstStageMask = &wait_destination_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*m_context->m_command_buffers[m_context->m_frame_index],
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

        m_context->m_vk_swapchain.recreate(*m_window, m_context->m_vk_instance, m_context->m_vk_device);
        m_context->create_depth_resources();
    }

    auto GraphicsUtils::shutdown() -> void {

        m_context->m_vk_device.logical().waitIdle();
    }

    auto GraphicsUtils::set_framebuffer_resized(bool resized) -> void {
        m_framebuffer_resized = resized;
    }

    auto GraphicsUtils::set_camera_data(const CameraData& camera_data) -> void {
        m_camera_data = camera_data;
    }

    auto GraphicsUtils::update_uniform_buffers(uint32_t frame_index) -> void {

        // Testing Purposes only, all updates to be done in update loop, not draw loop
        // static auto startTime = std::chrono::high_resolution_clock::now();
        //
        // auto currentTime = std::chrono::high_resolution_clock::now();
        // float time = std::chrono::duration<float>(currentTime - startTime).count();

        if (m_camera_data != std::nullopt) {

            m_camera_data.value().projection[1][1] *= -1; // Invert Y due to glm being designed for OpenGL
            memcpy(m_context->m_camera_buffers_mapped[frame_index], &m_camera_data.value(), sizeof(CameraData));
        }

    }

    auto GraphicsUtils::get_aspect_ratio() -> float {

        return static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().width) / static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().height);
    }

    auto GraphicsUtils::create_mesh_resource(const MeshData& mesh_data) -> MeshResource {

        MeshResource result;
        auto& device = m_context->m_vk_device;

        vk::DeviceSize buffer_size = sizeof(Vertex) * mesh_data.vertices.size();

        auto [staging_buffer, staging_buffer_memory] =
            device.create_buffer(buffer_size,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                );

        void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
        memcpy(data_staging, mesh_data.vertices.data(), buffer_size);
        staging_buffer_memory.unmapMemory();

        std::tie(result.vertex_buffer, result.vertex_buffer_memory) =
             device.create_buffer(buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

        device.copy_buffer(staging_buffer, result.vertex_buffer, buffer_size);

        buffer_size = sizeof(std::uint32_t) * mesh_data.indices.size();
        auto [staging_buffer_indices, staging_buffer_memory_indices] =
            device.create_buffer(buffer_size,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                );
        data_staging = staging_buffer_memory_indices.mapMemory(0, buffer_size);
        memcpy(data_staging, mesh_data.indices.data(), buffer_size);
        staging_buffer_memory_indices.unmapMemory();
        std::tie(result.index_buffer, result.index_buffer_memory) =
             device.create_buffer(buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        device.copy_buffer(staging_buffer_indices, result.index_buffer, buffer_size);

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

        vk::DeviceSize image_size = texture_data.width * texture_data.height * STBI_rgb_alpha;

        auto [buffer, buffer_memory] = device.create_buffer(
            image_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );

        void* data_staging = buffer_memory.mapMemory(0, image_size);
        memcpy(data_staging, texture_data.pixel_data.data(), image_size);
        buffer_memory.unmapMemory();

        // Don't free STB memory, thats up to the caller

        auto [texture, texture_memory] = device.create_image(texture_data.width, texture_data.height, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

        device.transition_image_layout(texture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);
        device.copy_buffer_to_image(buffer, texture, texture_data.width, texture_data.height);
        device.transition_image_layout(texture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor);

        vk::PhysicalDeviceProperties props = device.physical().getProperties();
        vk::SamplerCreateInfo sampler_info{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eMirroredRepeat,
            .addressModeV = vk::SamplerAddressMode::eMirroredRepeat,
            .addressModeW = vk::SamplerAddressMode::eMirroredRepeat,
            .anisotropyEnable = vk::True,
            .maxAnisotropy = props.limits.maxSamplerAnisotropy,
            .compareEnable = vk::False,
            .compareOp = vk::CompareOp::eAlways,
        };

        vk::raii::ImageView iv = device.create_image_view(texture, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
        vk::raii::Sampler sampler = device.logical().createSampler(sampler_info);

        return TextureResource{
            .image        = std::move(texture),
            .image_memory = std::move(texture_memory),
            .image_view   = std::move(iv),
            .sampler      = std::move(sampler)
        };
    }

    auto GraphicsUtils::create_material_descriptor_sets(const TextureResource& texture) -> std::vector<vk::raii::DescriptorSet> {

        auto& device = m_context->m_vk_device;

        std::vector layouts(VulkanContext::MAX_FRAMES_IN_FLIGHT, *m_context->m_vk_pipeline.descriptor_set_layout());

        vk::DescriptorSetAllocateInfo alloc_info{
            .descriptorPool = m_context->m_descriptor_pool,
            .descriptorSetCount = VulkanContext::MAX_FRAMES_IN_FLIGHT,
            .pSetLayouts = layouts.data()
        };

        auto sets = device.logical().allocateDescriptorSets(alloc_info);

        for (uint32_t i = 0; i < VulkanContext::MAX_FRAMES_IN_FLIGHT; i++) {

            const vk::DescriptorBufferInfo camera_info{
                .buffer = m_context->m_camera_buffers[i],
                .offset = 0,
                .range = sizeof(CameraData)
            };
            const vk::DescriptorImageInfo texture_info{
                .sampler = texture.sampler,
                .imageView = texture.image_view,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };
            std::array descriptor_writes {
                vk::WriteDescriptorSet {
                    .dstSet = sets[i],
                    .dstBinding = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo = &camera_info
                },
                vk::WriteDescriptorSet {
                    .dstSet = sets[i],
                    .dstBinding = 1,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo = &texture_info
                }
            };
            device.logical().updateDescriptorSets(descriptor_writes, {});
        }
        return sets;
    }

    auto GraphicsUtils::register_mesh(const MeshData& data) -> std::uint32_t {

        MeshResource mesh_resource = create_mesh_resource(data);
        m_mesh_resources.emplace_back(std::move(mesh_resource));
        return m_mesh_resources.size();
    }

    auto GraphicsUtils::register_texture(const std::string& path) -> std::uint32_t {

        TextureResource texture_resource = create_texture_resource(path);
        m_texture_resources.emplace_back(std::move(texture_resource));
        return m_texture_resources.size();
    }

    auto GraphicsUtils::register_material(const std::uint32_t& texture_handle,
        const flux::float4 base_colour) -> std::uint32_t {

        MaterialResource material;
        material.albedo_texture_handle = texture_handle;
        material.albedo_color = base_colour;
        material.descriptor_sets = create_material_descriptor_sets(*get_texture_resource(texture_handle));
        m_material_resources.emplace_back(std::move(material));
        return m_material_resources.size();
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

    auto GraphicsUtils::record_command_buffers(uint32_t image_index, const std::vector<DrawCommand>& cmds) -> void {

        constexpr vk::CommandBufferBeginInfo begin_info{};

        auto& command_buffer = m_context->m_command_buffers[m_context->m_frame_index];

        command_buffer.begin(begin_info);

        m_context->transition_image_layout(
            m_context->m_vk_swapchain.swapchain_images()[image_index],
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::ImageAspectFlagBits::eColor
            );

        m_context->transition_image_layout(
            m_context->m_depth_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::ImageAspectFlagBits::eDepth
        );

        constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.1f, 0.1f, 0.1f, 1.f);
        constexpr vk::ClearValue depth_clear_value = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderingAttachmentInfo attachment_info {
            .imageView = m_context->m_vk_swapchain.swapchain_image_views()[image_index],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clear_color
        };

        vk::RenderingAttachmentInfo depth_attachment_info {
            .imageView = m_context->m_depth_image_view,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .clearValue = depth_clear_value
        };

        vk::RenderingInfo rendering_info {
            .renderArea = {.offset = {0, 0}, .extent = m_context->m_vk_swapchain.swapchain_extent()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachment_info,
            .pDepthAttachment = &depth_attachment_info
        };

        command_buffer.beginRendering(rendering_info);

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_context->m_vk_pipeline.graphics_pipeline());
        command_buffer.setViewport(0,
            vk::Viewport{0, static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().height), static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().width)
                , -static_cast<float>(m_context->m_vk_swapchain.swapchain_extent().height), 0, 1});
        command_buffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, m_context->m_vk_swapchain.swapchain_extent()});

        std::uint32_t prev_material = 0;

        for (const auto& cmd : cmds) {

            const auto* mesh = get_mesh_resource(cmd.mesh_handle);
            const auto* material = get_material_resource(cmd.material_handle);

            if (cmd.material_handle != prev_material) {
                command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_context->m_vk_pipeline.pipeline_layout(), 0, *material->descriptor_sets[m_context->m_frame_index], nullptr);
                prev_material = cmd.material_handle;
            }

            const PushConstants push_constants { cmd.model, cmd.base_colour };
            command_buffer.pushConstants<PushConstants>(*m_context->m_vk_pipeline.pipeline_layout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, push_constants);

            command_buffer.bindVertexBuffers(0, {*mesh->vertex_buffer}, {0});
            command_buffer.bindIndexBuffer(*mesh->index_buffer, 0, vk::IndexType::eUint32);
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_context->m_vk_pipeline.pipeline_layout(), 0, *material->descriptor_sets[m_context->m_frame_index], nullptr);
            command_buffer.drawIndexed(mesh->index_count, 1, 0, 0, 0);
        }

        command_buffer.endRendering();

        m_context->transition_image_layout(
            m_context->m_vk_swapchain.swapchain_images()[image_index],
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
            {},                                                     // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
            vk::ImageAspectFlagBits::eColor
        );
        command_buffer.end();
    }
} // namespace flux
