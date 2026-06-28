//
// Created by Admin on 27/06/2026.
//

#pragma once
#include <cstdint>

#include "material_buffer.hpp"
#include "render_resources.hpp"
#include "helix/colour.hpp"
#include "mesh/mesh_primitive.hpp"

namespace flux {
    class ThreshVkDevice;
    class DescriptorHeap;

    class RenderResourceRegistry {

        public:
            RenderResourceRegistry(ThreshVkDevice& device, DescriptorHeap& resource_heap, std::uint32_t material_capacity);

            auto register_mesh(const MeshData& data) -> std::uint32_t;
            auto register_mesh_data(std::span<const std::byte> vertices,
                std::span<const std::byte> indices,
                std::uint32_t index_count,
                const helix::AABB& local_aabb) -> std::uint32_t;

            auto register_texture(const std::string& path) -> std::uint32_t;

            auto register_material(std::uint32_t texture_handle, helix::Colour base_colour = helix::Colour(1.f) , const std::string& material_type = "opaque") -> std::uint32_t;
            auto register_material(const gpu::MaterialData& data, const std::string& material_type = "opaque") -> std::uint32_t;

            [[nodiscard]] auto get_mesh_resource(std::uint32_t mesh_id) const -> const MeshResource*;
            [[nodiscard]] auto get_texture_resource(std::uint32_t texture_id) const -> const TextureResource*;
            [[nodiscard]] auto get_material_resource(std::uint32_t material_id) const -> const MaterialResource*;

            [[nodiscard]] auto material_buffer_address() const -> vk::DeviceAddress { return m_material_buffer.device_address(); }
            [[nodiscard]] auto dummy_texture_handle() const -> std::uint32_t { return m_dummy_texture_heap_index; }

        private:
            auto create_mesh_resource(const MeshData& mesh_data) -> MeshResource;
            auto create_texture_resource(const std::string& path) -> TextureResource;
            auto create_texture_resource(const TextureData& texture_data) -> TextureResource;
            auto store_texture(TextureResource&& texture) -> std::uint32_t;
            auto register_dummy_texture() -> void;

            ThreshVkDevice& m_device;
            DescriptorHeap& m_resource_heap;
            MaterialBuffer  m_material_buffer;

            std::vector<MeshResource>     m_mesh_resources;
            std::vector<TextureResource>  m_texture_resources;
            std::vector<MaterialResource> m_material_resources;
            std::uint32_t                 m_dummy_texture_heap_index = 0;
    };
} // flux
