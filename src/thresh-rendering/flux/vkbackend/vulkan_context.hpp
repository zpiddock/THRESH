//
// Created by Admin on 23/04/2026.
//

#pragma once
#include <string>

#include "vulkan/vulkan_raii.hpp"
#include "vk_structs.hpp"
#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_swapchain.hpp"
#include "horizon/window.hpp"

namespace flux {

    class VulkanContext {
        public:
            VulkanContext(const VulkanInstanceContext& ctx, const thresh::Window& window);

            ~VulkanContext();

        private:


            auto create_depth_resources() -> void;

            auto create_texture_image() -> void; // Move to assets loading system

            auto create_texture_image_view() -> void;

            auto create_texture_sampler() -> void;

            auto create_vertex_buffer() -> void;

            auto create_index_buffer() -> void;

            auto create_uniform_buffers() -> void;

            auto create_descriptor_pool() -> void;

            auto create_descriptor_sets() -> void;

            auto create_command_buffers() -> void;

            auto create_sync_objects() -> void;

            // Rendering functions
            auto record_command_buffer(uint32_t image_index) -> void;

            auto transition_image_layout(vk::Image         image,
                                         vk::ImageLayout         old_layout,
                                         vk::ImageLayout         new_layout,
                                         vk::AccessFlags2        src_access_mask,
                                         vk::AccessFlags2        dst_access_mask,
                                         vk::PipelineStageFlags2 src_stage_mask,
                                         vk::PipelineStageFlags2 dst_stage_mask, vk::ImageAspectFlags aspect_flags) -> void;



            ThreshVkInstance  m_vk_instance;
            ThreshVkDevice    m_vk_device;
            ThreshVkSwapchain m_vk_swapchain;
            ThreshVkPipeline  m_vk_pipeline;

            std::vector<vk::raii::CommandBuffer>          m_command_buffers;


            std::vector<vk::raii::Semaphore>              m_present_complete_semaphores;
            std::vector<vk::raii::Semaphore>              m_render_complete_semaphores;
            std::vector<vk::raii::Fence>                  m_inflight_fences;
            uint32_t                                      m_frame_index = 0;

            vk::raii::Buffer                     m_vertex_buffer           = nullptr;
            vk::raii::DeviceMemory               m_vertex_buffer_memory    = nullptr;
            vk::raii::Buffer                     m_index_buffer            = nullptr;
            vk::raii::DeviceMemory               m_index_buffer_memory     = nullptr;
            std::vector<vk::raii::Buffer>        m_uniform_buffers;
            std::vector<vk::raii::DeviceMemory>  m_uniform_buffer_memory;
            std::vector<void*>                   m_uniform_buffers_mapped;

            vk::raii::Image m_image = nullptr;
            vk::raii::DeviceMemory m_image_memory = nullptr;
            vk::raii::ImageView m_image_view = nullptr;

            vk::raii::Sampler m_texture_sampler = nullptr;

            vk::raii::Image m_depth_image = nullptr;
            vk::raii::DeviceMemory m_depth_image_memory = nullptr;
            vk::raii::ImageView m_depth_image_view = nullptr;

            vk::raii::DescriptorPool             m_descriptor_pool         = nullptr;
            std::vector<vk::raii::DescriptorSet> m_descriptor_sets;

            // const bits
            constexpr static int MAX_FRAMES_IN_FLIGHT = 2;

            friend class GraphicsUtils;
    };
} // flux
