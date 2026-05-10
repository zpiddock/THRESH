
#pragma once
#include <memory>

#include "graphics_types.hpp"
#include "render_resources.hpp"
#include "horizon/window.hpp"
#include "mesh/mesh_primitive.hpp"
#include "vkbackend/vulkan_context.hpp"


namespace flux {

    class GraphicsUtils {
        public:

            auto init_vulkan(const VulkanInstanceContext& ctx, const thresh::Window& window) -> void;

            auto init_vulkan(const thresh::Window& window) -> void;

            auto get_vulkan_context() -> VulkanContext*;

            auto draw_frame() -> void;

            auto shutdown() -> void;

            auto recreate_swapchain() -> void;

            auto set_framebuffer_resized(bool resized) -> void;

            auto update_uniform_buffers(uint32_t frame_index) -> void;

            auto set_camera_data(const CameraData& camera_data) -> void;

            auto get_aspect_ratio() -> float;

            auto create_mesh_resource(const MeshData& mesh_data) -> MeshResource;

            auto create_texture_resource(const std::string& path) -> TextureResource;

            auto create_texture_resource(const TextureData& texture_data) -> TextureResource;

            auto create_material_descriptor_sets(const TextureResource& texture) -> std::vector<vk::raii::DescriptorSet>;

            auto register_mesh(const MeshData& data) -> std::uint32_t;

            auto register_texture(const std::string& path) -> std::uint32_t;

            auto register_material(const std::uint32_t& texture_handle, flux::float4 base_colour = {1.f, 1.f, 1.f, 1.f}) -> std::uint32_t;

            auto submit_draw_command(const DrawCommand& draw_command) -> void;

            auto get_mesh_resource(std::uint32_t mesh_id) const -> const MeshResource*;

            auto get_texture_resource(std::uint32_t texture_id) const -> const TextureResource*;

            auto get_material_resource(std::uint32_t material_id) const -> const MaterialResource*;

            auto get_draw_commands() -> std::vector<DrawCommand>&;

        private:
            auto record_command_buffers(uint32_t image_index, const std::vector<DrawCommand>& cmds) -> void;

            std::unique_ptr<VulkanContext> m_context;

            std::optional<CameraData> m_camera_data;

            bool                   m_framebuffer_resized = false;
            // Non Owning
            const thresh::Window* m_window = nullptr;

            // Resource Handles
            std::vector<MeshResource> m_mesh_resources;
            std::vector<TextureResource> m_texture_resources;
            std::vector<MaterialResource> m_material_resources;

            std::vector<DrawCommand> m_draw_commands;
    };

} // namespace flux
