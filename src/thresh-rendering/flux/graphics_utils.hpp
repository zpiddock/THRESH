
#pragma once
#include <memory>

#include "dear_im_gui_context.hpp"
#include "debug_line_renderer.hpp"
#include "graphics_types.hpp"
#include "render_resources.hpp"
#include "horizon/window.hpp"
#include "mesh/mesh_primitive.hpp"
#include "vkbackend/vulkan_context.hpp"


namespace flux {

    class GraphicsUtils {
        public:

            auto vulkan_init(const VulkanInstanceContext& ctx, const thresh::Window& window) -> void;

            auto vulkan_init(const thresh::Window& window) -> void;

            auto get_vulkan_context() -> VulkanContext*;

            auto draw_frame() -> void;

            auto shutdown() -> void;

            auto recreate_swapchain() -> void;

            auto set_framebuffer_resized(bool resized) -> void;

            auto update_uniform_buffers(uint32_t frame_index) -> void;

            auto set_camera_data(const gpu::CameraData& camera_data) -> void;

            auto set_light_data(const gpu::LightData& light_data) -> void;

            auto get_aspect_ratio() -> float;

            auto create_mesh_resource(const MeshData& mesh_data) -> MeshResource;

            auto create_texture_resource(const std::string& path) -> TextureResource;

            auto create_texture_resource(const TextureData& texture_data) -> TextureResource;

            auto register_mesh(const MeshData& data) -> std::uint32_t;

            auto register_texture(const std::string& path) -> std::uint32_t;

            auto register_material(const std::uint32_t& texture_handle,
                flux::float4 base_colour = {1.f, 1.f, 1.f, 1.f},
                const std::string& material_type = "opaque")
        -> std::uint32_t;

            auto submit_draw_command(const DrawCommand& draw_command) -> void;

            auto get_mesh_resource(std::uint32_t mesh_id) const -> const MeshResource*;

            auto get_texture_resource(std::uint32_t texture_id) const -> const TextureResource*;

            auto get_material_resource(std::uint32_t material_id) const -> const MaterialResource*;

            auto get_draw_commands() -> std::vector<DrawCommand>&;

            auto imgui_init() -> void;

            auto imgui_shutdown() -> void;

            auto imgui_new_frame() -> bool;

            auto imgui_process_event(const SDL_Event& event) -> void;

            auto imgui_enabled(bool enabled) -> void;

            [[nodiscard]] auto is_imgui_enabled() const -> bool;

            auto enable_debug_line_renderer() -> void;

            auto debug_line_renderer() -> DebugLineRenderer*;

        private:
            auto record_command_buffers(flux::CommandBuffer& cmd_buffer, uint32_t image_index, const std::vector<DrawCommand>& cmds) -> void;

            auto record_geometry_commands(flux::CommandBuffer& cmd_buffer, const std::vector<DrawCommand>& cmds) -> void;

            auto record_composite_commands(flux::CommandBuffer& cmd_buffer, uint32_t image_index) -> void;

            auto record_imgui_commands(flux::CommandBuffer& cmd_buffer, uint32_t image_index) -> void;

            std::unique_ptr<VulkanContext> m_context;

            std::optional<gpu::CameraData> m_camera_data;
            std::optional<gpu::LightData>  m_light_data;

            bool                   m_framebuffer_resized = false;
            // Non Owning
            const thresh::Window* m_window = nullptr;

            DearImGuiContext m_imgui_context;

            // Resource Handles
            std::vector<MeshResource> m_mesh_resources;
            std::vector<TextureResource> m_texture_resources;
            std::vector<MaterialResource> m_material_resources;

            std::vector<DrawCommand> m_draw_commands;

            std::unique_ptr<DebugLineRenderer> m_debug_line_renderer;
    };

} // namespace flux
