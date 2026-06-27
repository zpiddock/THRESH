//
// Renderer — owns the Vulkan backend, the frame loop, and the rendering
// subsystems. Merge of the former GraphicsUtils + VulkanContext.
//

#pragma once
#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "render_config.hpp"
#include "render_resource_registry.hpp"
#include "dear_im_gui_context.hpp"
#include "debug_line_renderer.hpp"
#include "render_resources.hpp"
#include "gpu_data.hpp"
#include "horizon/window.hpp"
#include "vkbackend/descriptor_heap.hpp"
#include "vkbackend/frame_context.hpp"
#include "vkbackend/image.hpp"
#include "vkbackend/pipeline_registry.hpp"
#include "vkbackend/vulkan_device.hpp"
#include "vkbackend/vulkan_instance.hpp"
#include "vkbackend/vulkan_swapchain.hpp"

namespace flux {

    class Renderer {
        public:
            Renderer(const thresh::Window& window, const RenderConfig& config);
            ~Renderer();

            // Owns device + heaps by value and lends them (by reference) to the
            // registries it also owns — must never be copied or moved.
            Renderer(const Renderer&)                    = delete;
            Renderer(Renderer&&)                         = delete;
            auto operator=(const Renderer&) -> Renderer& = delete;
            auto operator=(Renderer&&)      -> Renderer& = delete;

            // --- frame loop ---
            auto draw_frame() -> void;
            auto recreate_swapchain() -> void;
            auto set_framebuffer_resized(bool resized) -> void;
            auto set_camera_data(const gpu::CameraData& camera_data) -> void;
            auto set_light_data(const gpu::LightData& light_data) -> void;
            auto get_aspect_ratio() -> float;
            auto submit_draw_command(const DrawCommand& draw_command) -> void;

            // --- subsystems / facade ---
            auto resources() -> RenderResourceRegistry& { return m_resources; }
            auto imgui_init() -> void;
            auto imgui_shutdown() -> void;
            auto imgui_new_frame() -> bool;
            auto imgui_process_event(const SDL_Event& event) -> void;
            auto imgui_enabled(bool enabled) -> void;
            [[nodiscard]] auto is_imgui_enabled() const -> bool;
            auto enable_debug_line_renderer() -> void;
            auto debug_line_renderer() -> DebugLineRenderer*;
            auto shutdown() -> void;

        private:
            // backend setup (was VulkanContext)
            auto create_depth_resources() -> void;
            auto create_offscreen_resources() -> void;
            auto pick_offscreen_format() -> vk::Format;
            auto create_frame_contexts() -> void;
            auto create_sync_objects() -> void;
            auto advance_frame() -> void;

            // frame recording (was GraphicsUtils private)
            auto record_command_buffers(CommandBuffer& cmd_buffer, uint32_t image_index, const std::vector<DrawCommand>& cmds) -> void;
            auto record_geometry_commands(CommandBuffer& cmd_buffer, const std::vector<DrawCommand>& cmds) -> void;
            auto record_composite_commands(CommandBuffer& cmd_buffer, uint32_t image_index) -> void;
            auto record_imgui_commands(CommandBuffer& cmd_buffer, uint32_t image_index) -> void;
            auto update_uniform_buffers(uint32_t frame_index) -> void;

            RenderConfig          m_config;
            const thresh::Window* m_window = nullptr;   // non-owning

            // --- backend (declaration order == construction order) ---
            ThreshVkInstance  m_vk_instance;
            ThreshVkDevice    m_vk_device;
            ThreshVkSwapchain m_vk_swapchain;
            DescriptorHeap    m_resource_heap;
            DescriptorHeap    m_sampler_heap;
            PipelineRegistry       m_pipelines;   // borrows m_vk_device
            RenderResourceRegistry m_resources;   // borrows m_vk_device + m_resource_heap

            HeapSlot          m_default_sampler_slot = HEAP_INVALID_SLOT;
            std::array<HeapSlot, MAX_FRAMES_IN_FLIGHT> m_offscreen_slots{HEAP_INVALID_SLOT, HEAP_INVALID_SLOT};

            flux::Image              m_depth_image;
            std::vector<flux::Image> m_offscreen_images;
            vk::Format               m_offscreen_format = vk::Format::eUndefined;

            std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> m_frames;
            std::vector<vk::raii::Semaphore>               m_render_complete_semaphores;
            uint32_t                                       m_frame_index = 0;

            std::optional<gpu::CameraData> m_camera_data;
            std::optional<gpu::LightData>  m_light_data;
            bool                           m_framebuffer_resized = false;
            DearImGuiContext               m_imgui_context;
            std::unique_ptr<DebugLineRenderer> m_debug_line_renderer;
            std::vector<DrawCommand>           m_draw_commands;
    };
} // flux
