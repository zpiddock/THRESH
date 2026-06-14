//
// Created by Admin on 23/04/2026.
//

#include "vulkan_context.hpp"

#include <memory>
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

        m_depth_image = flux::Image(m_vk_device, {
            .extent     = m_vk_swapchain.swapchain_extent(),
            .format     = format,
            .usage      = vk::ImageUsageFlagBits::eDepthStencilAttachment,
            .aspect     = vk::ImageAspectFlagBits::eDepth,
            .tiling     = vk::ImageTiling::eOptimal,
            .memory     = vk::MemoryPropertyFlagBits::eDeviceLocal,
            .mip_levels = 1,
            .debug_name = "Depth Image"
        });
    }

    auto VulkanContext::create_command_buffers() -> void {

    }

    auto VulkanContext::create_uniform_buffers() -> void {

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            m_frames[i] = FrameContext::create(m_vk_device);
        }
    }

    auto VulkanContext::create_descriptor_pool() -> void {

        std::array pool_size {

            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, 64 * MAX_FRAMES_IN_FLIGHT * 2 },
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
                .imageView = m_offscreen_images[i].view(),
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

        assert(m_render_complete_semaphores.empty());

        for (size_t i = 0; i < m_vk_swapchain.swapchain_images().size(); i++) {
            m_render_complete_semaphores.emplace_back(m_vk_device.logical(), vk::SemaphoreCreateInfo{});
        }
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

        m_offscreen_images.clear();

        m_offscreen_format = pick_offsreen_format();
        const auto extent = m_vk_swapchain.swapchain_extent();

        m_offscreen_images.reserve(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            auto image = flux::Image(m_vk_device, {
                .extent = extent,
                .format = m_offscreen_format,
                .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                .aspect = vk::ImageAspectFlagBits::eColor,
                .tiling = vk::ImageTiling::eOptimal,
                .memory = vk::MemoryPropertyFlagBits::eDeviceLocal,
                .mip_levels = 1,
                .debug_name = std::format("Forward_Pass_Offscreen_Image_{}", i).c_str()
            });

            m_offscreen_images.emplace_back(std::move(image));
        }

        // Check if offscreen sampler has been created or not
        if (!*m_offscreen_sampler) {
            // Nearest for a 1:1 copy. Switch back to eLinear if offscreen extent ever
            // diverges from swapchain extent (resolution scaling, fixed-res offscreen, etc).
            constexpr vk::SamplerCreateInfo sampler_info {
                .magFilter = vk::Filter::eNearest,
                .minFilter = vk::Filter::eNearest,
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

        SUB_DEBUG("Registering pipeline '{}'", name);
        auto pipeline = std::make_unique<ThreshVkPipeline>(context, m_vk_device);

        auto* raw = pipeline.get();
        auto [it, inserted] = m_pipelines.emplace(name, std::move(pipeline));
        if (!inserted) {
            SUB_WARN("Pipeline with name {} already exists, ignoring.", name);
            return it->second.get();
        }
        return raw;
    }

    auto VulkanContext::get_pipeline(const std::string& name) -> ThreshVkPipeline* {

        const auto it = m_pipelines.find(name);
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
                        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                        .descriptorCount = 1,
                        .stageFlags      = vk::ShaderStageFlagBits::eFragment
                    }
                },
            .push_constants = {
                    {
                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                        0,
                        sizeof(gpu::PushConstants)
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
