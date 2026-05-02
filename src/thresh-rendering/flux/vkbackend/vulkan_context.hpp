//
// Created by Admin on 23/04/2026.
//

#pragma once
#include <string>

#include "horizon/window.hpp"
#include "vulkan/vulkan_raii.hpp"

namespace flux {
    struct VulkanInstanceContext {
        std::string              application_name;
        std::string              engine_name               = "THRΞSH";
        std::string              engine_version            = "0.0.1";
        std::string              application_version       = "0.0.1";
        bool                     enable_validation_layers  = true;
        std::vector<const char*> enabled_validation_layers = {
            "VK_LAYER_KHRONOS_validation"
        };
        std::vector<const char*> required_instance_extensions = {};
    };

    class VulkanContext {
        public:
            VulkanContext(const VulkanInstanceContext& ctx, const thresh::Window& window);

            ~VulkanContext();

        private:
            // Vulkan Init Functions
            auto create_instance(const VulkanInstanceContext& ctx) -> void;

            auto setup_debug_messenger(const VulkanInstanceContext& ctx) -> void;

            auto create_surface(const thresh::Window& window) -> void;

            auto pick_suitable_device() -> void;

            auto create_logical_device() -> void;

            auto create_swapchain(const thresh::Window& window) -> void;

            auto create_image_views() -> void;

            auto create_graphics_pipelines() -> void;

            auto create_command_pool() -> void;

            auto create_command_buffer() -> void;

            // Device Private functions
            auto get_required_extensions(const VulkanInstanceContext& ctx) -> std::vector<const char*>;

            auto is_device_suitable(const vk::PhysicalDevice& device) -> bool;

            // Swapchain Private Functions
            auto choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& formats) -> vk::SurfaceFormatKHR;

            auto choose_swap_extents(const vk::SurfaceCapabilitiesKHR& surface_capabilities,
                                     const thresh::Window&             window) -> vk::Extent2D;

            auto choose_min_swap_image_count(const vk::SurfaceCapabilitiesKHR& surface_capabilities) -> uint32_t;

            auto choose_swapchain_present_mode(
                const std::vector<vk::PresentModeKHR>& present_modes) -> vk::PresentModeKHR;

            // Shader Functions - TODO: Create Shader Wrapper Class
            auto load_shader(const std::string& shader_path) -> vk::raii::ShaderModule;

            // Rendering functions
            auto record_command_buffer(uint32_t image_index);

            auto transition_image_layout(uint32_t                imageIndex,
                                         vk::ImageLayout         old_layout,
                                         vk::ImageLayout         new_layout,
                                         vk::AccessFlags2        src_access_mask,
                                         vk::AccessFlags2        dst_access_mask,
                                         vk::PipelineStageFlags2 src_stage_mask,
                                         vk::PipelineStageFlags2 dst_stage_mask) -> void;

            std::vector<const char*> m_required_device_extensions = {
                vk::KHRSwapchainExtensionName
            };

            vk::raii::Context                m_context;
            vk::raii::Instance               m_instance                 = nullptr;
            vk::raii::DebugUtilsMessengerEXT m_debugMessenger           = nullptr;
            vk::raii::PhysicalDevice         m_physicalDevice           = nullptr;
            vk::raii::Device                 m_device                   = nullptr;
            uint32_t                         m_queue_family_index       = ~0u;
            vk::raii::Queue                  m_graphics_queue           = nullptr;
            vk::raii::SurfaceKHR             m_surface                  = nullptr;
            vk::raii::PipelineLayout         m_pipeline_layout          = nullptr;
            vk::raii::Pipeline               m_graphics_pipeline        = nullptr;
            vk::raii::CommandPool            m_command_pool             = nullptr;
            vk::raii::CommandBuffer          m_command_buffer           = nullptr;

            vk::raii::Semaphore              present_complete_semaphore = nullptr;
            vk::raii::Semaphore              render_complete_semaphore  = nullptr;
            vk::raii::Fence                  draw_fence                 = nullptr;

            // Swapchain Stuff
            vk::raii::SwapchainKHR m_swapchain = nullptr;
            std::vector<vk::Image> m_swapchain_images;
            vk::SurfaceFormatKHR   m_swapchain_surface_format;
            vk::Extent2D           m_swapchain_extent;

            // Image View stuff
            std::vector<vk::raii::ImageView> m_swapchain_image_views;

            // const strings
            const std::string vertex_main   = "vertexMain";
            const std::string fragment_main = "fragmentMain";
    };
} // flux
