//
// Created by Admin on 23/04/2026.
//

#include "vulkan_context.hpp"

#include <iostream>
#include <set>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "flux/graphics_types.hpp"
#include "flux/math.hpp"
#include "SDL3/SDL_vulkan.h"
#include "substratum/log.hpp"
#include "substratum/filesystem/vfs.hpp"

namespace flux {

    VulkanContext::VulkanContext(const VulkanInstanceContext& ctx, const thresh::Window& window) :
    m_vk_instance(ctx, window),
    m_vk_device(m_vk_instance.instance(), m_vk_instance.surface()),
    m_vk_swapchain(window, m_vk_instance, m_vk_device) {

        create_depth_resources();
        create_offscreen_resources();
        register_geometry_pipeline();
        register_composite_pipeline();
        create_uniform_buffers();
        create_descriptor_pool();
        create_composite_descriptor_sets();
        create_command_buffers();
        create_sync_objects();
    }

    VulkanContext::~VulkanContext() {
    }

    auto VulkanContext::create_depth_resources() -> void {

        vk::Format format = m_vk_device.find_depth_format();
        auto [depth_image, depth_image_memory] = m_vk_device.create_image(m_vk_swapchain.swapchain_extent().width, m_vk_swapchain.swapchain_extent().height, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);

        m_depth_image = std::move(depth_image);
        m_depth_image_memory = std::move(depth_image_memory);
        m_depth_image_view = m_vk_device.create_image_view(m_depth_image, format, vk::ImageAspectFlagBits::eDepth);
    }

    auto VulkanContext::create_command_buffers() -> void {
        vk::CommandBufferAllocateInfo alloc_info{
            .commandPool        = m_vk_device.command_pool(),
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };

        m_command_buffers = vk::raii::CommandBuffers(m_vk_device.logical(), alloc_info);
    }

    auto VulkanContext::create_texture_image() -> void {

        int width, height, nrChannels;

        auto texture_data = substratum::VFS::read_file("textures/brick.png");

        stbi_uc* data = stbi_load_from_memory(texture_data.data(), texture_data.size(), &width, &height, &nrChannels, STBI_rgb_alpha);

        // vk::DeviceSize image_size = width * height * STBI_rgb_alpha;
        //
        // if (!data || width <= 0 || height <= 0) {
        //     SUB_FATAL("Failed to load texture image!");
        // }
        //
        // SUB_TRACE("Texture Data: Size:{}, Width:{}, Height:{}, Channels:{}", texture_data.size(), width, height, nrChannels);
        //
        // auto [buffer, buffer_memory] = m_vk_device.create_buffer(
        //     image_size,
        //     vk::BufferUsageFlagBits::eTransferSrc,
        //     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        //     );
        //
        // void* data_staging = buffer_memory.mapMemory(0, image_size);
        // memcpy(data_staging, data, image_size);
        // buffer_memory.unmapMemory();
        //
        // // Free STB memory
        // stbi_image_free(data);
        //
        // auto [texture, texture_memory] = m_vk_device.create_image(width, height, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);
        //
        // m_vk_device.transition_image_layout(texture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);
        // m_vk_device.copy_buffer_to_image(buffer, texture, width, height);
        // m_vk_device.transition_image_layout(texture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor);
        //
        // m_image = std::move(texture);
        // m_image_memory = std::move(texture_memory);
    }

    auto VulkanContext::create_texture_image_view() -> void {

    }

    auto VulkanContext::create_texture_sampler() -> void {

    }

    auto VulkanContext::create_vertex_buffer() -> void {

        // vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
        //
        // auto [staging_buffer, staging_buffer_memory] =
        //     m_vk_device.create_buffer(buffer_size,
        //         vk::BufferUsageFlagBits::eTransferSrc,
        //         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        //         );
        //
        // void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
        // memcpy(data_staging, vertices.data(), buffer_size);
        // staging_buffer_memory.unmapMemory();
        //
        // std::tie(m_vertex_buffer, m_vertex_buffer_memory) =
        //      m_vk_device.create_buffer(buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        //
        // m_vk_device.copy_buffer(staging_buffer, m_vertex_buffer, buffer_size);
    }

    auto VulkanContext::create_index_buffer() -> void {

        // vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();
        // auto [staging_buffer, staging_buffer_memory] =
        //     m_vk_device.create_buffer(buffer_size,
        //         vk::BufferUsageFlagBits::eTransferSrc,
        //         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        //         );
        // void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
        // memcpy(data_staging, indices.data(), buffer_size);
        // staging_buffer_memory.unmapMemory();
        // std::tie(m_index_buffer, m_index_buffer_memory) =
        //      m_vk_device.create_buffer(buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        // m_vk_device.copy_buffer(staging_buffer, m_index_buffer, buffer_size);
    }

    auto VulkanContext::create_uniform_buffers() -> void {

        m_camera_buffers.clear();
        m_camera_buffer_memory.clear();
        m_camera_buffers_mapped.clear();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            vk::DeviceSize camera_buffer_size = sizeof(CameraData);
            auto [camera_buffer, camera_memory] =
                m_vk_device.create_buffer(camera_buffer_size,
                    vk::BufferUsageFlagBits::eUniformBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                    );
            m_camera_buffers.emplace_back(std::move(camera_buffer));
            m_camera_buffer_memory.emplace_back(std::move(camera_memory));
            m_camera_buffers_mapped.emplace_back(m_camera_buffer_memory[i].mapMemory(0, camera_buffer_size));
        }
    }

    auto VulkanContext::create_descriptor_pool() -> void {

        std::array pool_size {

            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT },
            vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, 64 * MAX_FRAMES_IN_FLIGHT },
        };

        vk::DescriptorPoolCreateInfo pool_create_info {
            .flags          = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets        = 64 * MAX_FRAMES_IN_FLIGHT,
            .poolSizeCount  = pool_size.size(),
            .pPoolSizes     = pool_size.data(),
        };

        m_descriptor_pool = vk::raii::DescriptorPool(m_vk_device.logical(), pool_create_info);
    }

    auto VulkanContext::create_composite_descriptor_sets() -> void {

        auto* composite_pipeline = get_pipeline("composite");

        std::vector layouts(MAX_FRAMES_IN_FLIGHT, *composite_pipeline->descriptor_set_layout());

        vk::DescriptorSetAllocateInfo alloc_info {
            .descriptorPool = m_descriptor_pool,
            .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
            .pSetLayouts = layouts.data()
        };
        m_composite_pass_descriptor_sets.clear();
        auto sets = m_vk_device.logical().allocateDescriptorSets(alloc_info);
        m_composite_pass_descriptor_sets = std::move(sets);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            vk::DescriptorImageInfo image_info {
                .sampler = m_offscreen_sampler,
                .imageView = m_offscreen_image_views[i],
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };

            vk::WriteDescriptorSet writes {
                .dstSet = m_composite_pass_descriptor_sets[i],
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &image_info
            };

            m_vk_device.logical().updateDescriptorSets(writes, {});
        }
    }

    auto VulkanContext::create_sync_objects() -> void {

        assert(m_present_complete_semaphores.empty() && m_render_complete_semaphores.empty() && m_inflight_fences.empty());

        for (size_t i = 0; i < m_vk_swapchain.swapchain_images().size(); i++) {
            m_render_complete_semaphores.emplace_back(m_vk_device.logical(), vk::SemaphoreCreateInfo{});
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_present_complete_semaphores.emplace_back(m_vk_device.logical(), vk::SemaphoreCreateInfo{});
            m_inflight_fences.emplace_back(m_vk_device.logical(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        }
    }

    auto VulkanContext::record_command_buffer(const uint32_t image_index, const std::vector<DrawCommand>& cmds) -> void {
        // constexpr vk::CommandBufferBeginInfo begin_info{};
        //
        // auto& command_buffer = m_command_buffers[m_frame_index];
        //
        // command_buffer.begin(begin_info);
        //
        // transition_image_layout(
        //     m_vk_swapchain.swapchain_images()[image_index],
        //     vk::ImageLayout::eUndefined,
        //     vk::ImageLayout::eColorAttachmentOptimal,
        //     {},
        //     vk::AccessFlagBits2::eColorAttachmentWrite,
        //     vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        //     vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        //     vk::ImageAspectFlagBits::eColor
        //     );
        //
        // transition_image_layout(
        //     m_depth_image,
        //     vk::ImageLayout::eUndefined,
        //     vk::ImageLayout::eDepthAttachmentOptimal,
        //     vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        // vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        // vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        // vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        //     vk::ImageAspectFlagBits::eDepth
        // );
        //
        // constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.f, 1.f, 0.f, 1.f);
        // constexpr vk::ClearValue depth_clear_value = vk::ClearDepthStencilValue(1.0f, 0);
        // vk::RenderingAttachmentInfo attachment_info {
        //     .imageView = m_vk_swapchain.swapchain_image_views()[image_index],
        //     .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        //     .loadOp = vk::AttachmentLoadOp::eClear,
        //     .storeOp = vk::AttachmentStoreOp::eStore,
        //     .clearValue = clear_color
        // };
        //
        // vk::RenderingAttachmentInfo depth_attachment_info {
        //     .imageView = m_depth_image_view,
        //     .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        //     .loadOp = vk::AttachmentLoadOp::eClear,
        //     .storeOp = vk::AttachmentStoreOp::eDontCare,
        //     .clearValue = depth_clear_value
        // };
        //
        // vk::RenderingInfo rendering_info {
        //     .renderArea = {.offset = {0, 0}, .extent = m_vk_swapchain.swapchain_extent()},
        //     .layerCount = 1,
        //     .colorAttachmentCount = 1,
        //     .pColorAttachments = &attachment_info,
        //     .pDepthAttachment = &depth_attachment_info
        // };
        //
        // command_buffer.beginRendering(rendering_info);
        //
        // command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_vk_pipeline.graphics_pipeline());
        // command_buffer.setViewport(0,
        //     vk::Viewport{0, 0, static_cast<float>(m_vk_swapchain.swapchain_extent().width)
        //         , static_cast<float>(m_vk_swapchain.swapchain_extent().height), 0, 1});
        // command_buffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, m_vk_swapchain.swapchain_extent()});
        //
        // std::uint32_t prev_material = 0;
        //
        // for (const auto& cmd : cmds) {
        //
        //     const auto* mesh = utils->get_mesh_resource(cmd.mesh_handle);
        //     const auto* material = utils->get_material_resource(cmd.material_handle);
        //
        //     if (cmd.material_handle != prev_material) {
        //         command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_vk_pipeline.pipeline_layout(), 0, *material->descriptor_sets[m_frame_index], nullptr);
        //         prev_material = cmd.material_handle;
        //     }
        //
        //     const PushConstants push_constants { cmd.model, cmd.base_colour };
        //     command_buffer.pushConstants<PushConstants>(*m_vk_pipeline.pipeline_layout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, push_constants);
        //
        //     command_buffer.bindVertexBuffers(0, {*mesh->vertex_buffer}, {0});
        //     command_buffer.bindIndexBuffer(*mesh->index_buffer, 0, vk::IndexType::eUint32);
        //     command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_vk_pipeline.pipeline_layout(), 0, *material->descriptor_sets[m_frame_index], nullptr);
        //     command_buffer.drawIndexed(mesh->index_count, 1, 0, 0, 0);
        // }
        //
        // command_buffer.endRendering();
        //
        // transition_image_layout(
        //     m_vk_swapchain.swapchain_images()[image_index],
        //     vk::ImageLayout::eColorAttachmentOptimal,
        //     vk::ImageLayout::ePresentSrcKHR,
        //     vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
        //     {},                                                     // dstAccessMask
        //     vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
        //     vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
        //     vk::ImageAspectFlagBits::eColor
        // );
        // command_buffer.end();
    }

    auto VulkanContext::transition_image_layout(vk::Image         image, vk::ImageLayout  old_layout,
                                                vk::ImageLayout         new_layout, vk::AccessFlags2 src_access_mask,
                                                vk::AccessFlags2        dst_access_mask,
                                                vk::PipelineStageFlags2 src_stage_mask,
                                                vk::PipelineStageFlags2 dst_stage_mask, vk::ImageAspectFlags aspect_flags) -> void {
        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask        = src_stage_mask,
            .srcAccessMask       = src_access_mask,
            .dstStageMask        = dst_stage_mask,
            .dstAccessMask       = dst_access_mask,
            .oldLayout           = old_layout,
            .newLayout           = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image,
            .subresourceRange    = {
                .aspectMask     = aspect_flags,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };
        vk::DependencyInfo dependency_info = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };
        m_command_buffers[m_frame_index].pipelineBarrier2(dependency_info);
    }

    auto VulkanContext::pick_offsreen_format() -> vk::Format {

        constexpr auto required = vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eColorAttachment;

        const auto properties = m_vk_device.physical().getFormatProperties(vk::Format::eR16G16B16A16Sfloat);
        if((properties.optimalTilingFeatures & required) == required) {
            return vk::Format::eR16G16B16A16Sfloat;
        }
        SUB_WARN("HDR eR16G16B16A16Sfloat unsupported as colour attachment, falling back to swapchain format.");
        return m_vk_swapchain.swapchain_surface_format().format;
    }

    auto VulkanContext::create_offscreen_resources() -> void {

        m_offscreen_image_views.clear();
        m_offscreen_image_memory.clear();
        m_offscreen_images.clear();

        m_offscreen_format = pick_offsreen_format();
        const auto extent = m_vk_swapchain.swapchain_extent();

        m_offscreen_images.reserve(MAX_FRAMES_IN_FLIGHT);
        m_offscreen_image_views.reserve(MAX_FRAMES_IN_FLIGHT);
        m_offscreen_image_memory.reserve(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto [image, memory] = m_vk_device.create_image(
                extent.width,
                extent.height,
                m_offscreen_format,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal
                );

            auto view = m_vk_device.create_image_view(image, m_offscreen_format, vk::ImageAspectFlagBits::eColor);

            m_offscreen_images.emplace_back(std::move(image));
            m_offscreen_image_memory.emplace_back(std::move(memory));
            m_offscreen_image_views.emplace_back(std::move(view));
        }

        // Check if offscreen sampler has been created or not
        if (!*m_offscreen_sampler) {
            constexpr vk::SamplerCreateInfo sampler_info {
                .magFilter = vk::Filter::eLinear,
                .minFilter = vk::Filter::eLinear,
                .mipmapMode = vk::SamplerMipmapMode::eNearest,
                .addressModeU = vk::SamplerAddressMode::eClampToEdge,
                .addressModeV = vk::SamplerAddressMode::eClampToEdge,
                .addressModeW = vk::SamplerAddressMode::eClampToEdge,
                .anisotropyEnable = vk::False,
                .compareEnable = vk::False,
                .minLod = 0.f, .maxLod = 0.f
            };

            m_offscreen_sampler = vk::raii::Sampler(m_vk_device.logical(), sampler_info);
        }
    }

    auto VulkanContext::register_pipeline(const std::string& name,
        const PipelineContext& context) -> ThreshVkPipeline* {

        auto pipeline = std::unique_ptr<ThreshVkPipeline>(new ThreshVkPipeline(context, m_vk_device));

        auto* raw = pipeline.get();
        auto [it, inserted] = m_pipelines.emplace(name, std::move(pipeline));
        if (!inserted) {
            SUB_WARN("Pipeline with name {} already exists, ignoring.", name);
            return it->second.get();
        }
        return raw;
    }

    auto VulkanContext::get_pipeline(const std::string& name) -> ThreshVkPipeline* {

        auto it = m_pipelines.find(name);
        if (it == m_pipelines.end()) {
            SUB_FATAL("Pipeline with name {} not found.", name);
        }
        return it->second.get();
    }

    auto VulkanContext::register_geometry_pipeline() -> void {

        const PipelineContext context {
            .shader_path = "opaque_mesh.spv",
            .bindings = {
                    {
                        .binding = 0,
                        .descriptorType  = vk::DescriptorType::eUniformBuffer,
                        .descriptorCount = 1,
                        .stageFlags      = vk::ShaderStageFlagBits::eVertex
                    },
                    {
                        .binding = 1,
                        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                        .descriptorCount = 1,
                        .stageFlags      = vk::ShaderStageFlagBits::eFragment
                    }
                },
            .push_constants = {
                    {
                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                        0,
                        sizeof(PushConstants)
                    }
                },
            .use_vertex_input = true,
            .depth_test = true,
            .colour_format = m_offscreen_format,
            .depth_format = m_vk_device.find_depth_format()
        };

        register_pipeline("opaque_mesh", context);
    }

    auto VulkanContext::register_composite_pipeline() -> void {

        const PipelineContext context {
            .shader_path = "composite.spv",
            .bindings = {
                    { .binding = 0,
                      .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                      .descriptorCount = 1,
                      .stageFlags      = vk::ShaderStageFlagBits::eFragment },
                },
            .use_vertex_input = false,
            .depth_test = false,
            .cull_mode = vk::CullModeFlagBits::eNone,    // fullscreen tri — winding doesn't matter
            .colour_format = m_vk_swapchain.swapchain_surface_format().format,
            .depth_format = vk::Format::eUndefined
        };

        register_pipeline("composite", context);
    }
} // flux
