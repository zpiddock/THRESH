//
// Created by Admin on 23/04/2026.
//

#pragma once
#include <string>

#include "vulkan/vulkan_raii.hpp"
#include "frame_context.hpp"
#include "image.hpp"
#include "vk_structs.hpp"
#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_swapchain.hpp"
#include "flux/render_resources.hpp"
#include "horizon/window.hpp"

namespace flux {

    class VulkanContext {
        public:
            VulkanContext(const VulkanInstanceContext& ctx, const thresh::Window& window);

            ~VulkanContext();

            auto create_depth_resources() -> void;

            auto create_uniform_buffers() -> void;

            auto create_descriptor_pool() -> void;

            auto create_composite_descriptor_sets() -> void;

            auto create_command_buffers() -> void;

            auto create_sync_objects() -> void;

            auto pick_offsreen_format() -> vk::Format;

            auto create_offscreen_resources() -> void;

            auto register_pipeline(const std::string& name, const PipelineContext& context) -> ThreshVkPipeline*;

            auto get_pipeline(const std::string& name) -> ThreshVkPipeline*;

            auto register_geometry_pipeline() -> void;

            auto register_composite_pipeline() -> void;

            auto default_sampler_index() const -> uint32_t {
                return DescriptorHeap::shader_index(m_default_sampler_slot);
            }

            // const bits
            constexpr static int MAX_FRAMES_IN_FLIGHT = 2;

            ThreshVkInstance  m_vk_instance;
            ThreshVkDevice    m_vk_device;
            ThreshVkSwapchain m_vk_swapchain;
            
            vk::raii::DescriptorPool             m_descriptor_pool         = nullptr;

            std::unordered_map<std::string, std::unique_ptr<ThreshVkPipeline>> m_pipelines;

            std::vector<vk::raii::DescriptorSet> m_composite_pass_descriptor_sets;
            // ThreshVkPipeline  m_vk_pipeline;

            std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> m_frames;

            std::vector<vk::raii::Semaphore>              m_render_complete_semaphores;

            uint32_t                                      m_frame_index = 0;

            // Depth Images
            flux::Image m_depth_image;

            // Offscreen Images
            std::vector<flux::Image> m_offscreen_images;
            vk::raii::Sampler m_offscreen_sampler = nullptr;
            vk::Format m_offscreen_format = vk::Format::eUndefined;

            friend class GraphicsUtils;
            friend class DearImGuiContext;
    };
} // flux
