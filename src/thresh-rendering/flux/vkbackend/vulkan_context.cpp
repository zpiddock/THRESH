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
