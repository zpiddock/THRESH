//
// Created by Admin on 27/06/2026.
//

#include "render_resource_registry.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "substratum/log.hpp"
#include "substratum/filesystem/vfs.hpp"
#include "vkbackend/image.hpp"
#include "vkbackend/vulkan_device.hpp"

namespace flux {
    RenderResourceRegistry::RenderResourceRegistry(ThreshVkDevice& device, DescriptorHeap& resource_heap,
        std::uint32_t material_capacity)
    : m_device(device),
    m_resource_heap(resource_heap),
    m_material_buffer(device, material_capacity) {
        register_dummy_texture();
    }

    auto RenderResourceRegistry::register_mesh(const MeshData& data) -> std::uint32_t {
        m_mesh_resources.emplace_back(create_mesh_resource(data));
        const auto handle = static_cast<std::uint32_t>(m_mesh_resources.size());
        SUB_TRACE("Registered mesh handle {} ({} vertices, {} indices)", handle, data.vertices.size(), data.indices.size());
        return handle;
    }

    auto RenderResourceRegistry::register_texture(const std::string& path) -> std::uint32_t {
        const auto handle = store_texture(create_texture_resource(path));
        SUB_TRACE("Registered texture handle {} from '{}'", handle, path);
        return handle;
    }

    auto RenderResourceRegistry::register_material(std::uint32_t texture_handle, helix::Colour base_colour,
        const std::string& material_type) -> std::uint32_t {
        const auto* albedo = get_texture_resource(texture_handle);
        const std::uint32_t albedo_slot = albedo ? albedo->image.heap_index() : m_dummy_texture_heap_index;

        const gpu::MaterialData data {
            .base_colour_factor = static_cast<helix::float4>(base_colour),
            .emissive_factor    = helix::float3{0},
            .metallic_factor    = 1.f,
            .roughness_factor   = 1.f,
            .normal_scale       = 1.f,
            .occlusion_strength = 1.f,
            .alpha_cutoff       = 0.5f,
            .base_colour_texture_handle        = albedo_slot,
            .normal_texture_handle             = m_dummy_texture_heap_index,
            .emissive_texture_handle           = m_dummy_texture_heap_index,
            .metallic_roughness_texture_handle = m_dummy_texture_heap_index,
            .occlusion_texture_handle          = m_dummy_texture_heap_index,
            .flags = 0,
            };
        const std::uint32_t gpu_index = m_material_buffer.register_material(data);

        m_material_resources.emplace_back(MaterialResource{ .material_type = material_type, .gpu_index = gpu_index });
        const auto handle = static_cast<std::uint32_t>(m_material_resources.size());
        SUB_TRACE("Registered material handle {} -> gpu index {} (type='{}', albedo_slot={})", handle, gpu_index, material_type, albedo_slot);
        return handle;
    }

    auto RenderResourceRegistry::get_mesh_resource(std::uint32_t mesh_id) const -> const MeshResource* {
        if (mesh_id == 0 || mesh_id > m_mesh_resources.size()) return nullptr;
        return &m_mesh_resources[mesh_id - 1];
    }

    auto RenderResourceRegistry::get_texture_resource(std::uint32_t texture_id) const -> const TextureResource* {
        if (texture_id == 0 || texture_id > m_texture_resources.size()) return nullptr;
        return &m_texture_resources[texture_id - 1];
    }

    auto RenderResourceRegistry::get_material_resource(std::uint32_t material_id) const -> const MaterialResource* {
        if (material_id == 0 || material_id > m_material_resources.size()) return nullptr;
        return &m_material_resources[material_id - 1];
    }

    auto RenderResourceRegistry::create_mesh_resource(const MeshData& mesh_data) -> MeshResource {
        MeshResource result;
        for (const auto& vertex : mesh_data.vertices) {
            result.local_aabb.expand(vertex.position);
        }
        result.vertex = m_device.upload_device_local(std::as_bytes(std::span{mesh_data.vertices}), vk::BufferUsageFlagBits::eVertexBuffer, "Vertex Buffer");
        result.index  = m_device.upload_device_local(std::as_bytes(std::span{mesh_data.indices}),  vk::BufferUsageFlagBits::eIndexBuffer,  "Index Buffer");
        result.index_count = mesh_data.indices.size();
        return result;
    }

    auto RenderResourceRegistry::create_texture_resource(const std::string& path) -> TextureResource {
        int width, height, nrChannels;
        const auto bytes = substratum::VFS::read_file(path);
        stbi_uc* data = stbi_load_from_memory(bytes.data(), bytes.size(), &width, &height, &nrChannels, STBI_rgb_alpha);
        const size_t size = static_cast<size_t>(width) * height * STBI_rgb_alpha;

        const TextureData texture {
            .width = width, .height = height, .num_channels = nrChannels,
            .pixel_data = std::span(data, size),
        };
        TextureResource result = create_texture_resource(texture);
        stbi_image_free(data);
        return result;
    }

    auto RenderResourceRegistry::create_texture_resource(const TextureData& texture_data) -> TextureResource {
        auto image = m_device.upload_image(
            std::as_bytes(texture_data.pixel_data),
            vk::Extent2D{ static_cast<uint32_t>(texture_data.width), static_cast<uint32_t>(texture_data.height) },
            vk::Format::eR8G8B8A8Srgb,
            nullptr);
        return TextureResource{ .image = std::move(image) };
    }

    auto RenderResourceRegistry::store_texture(TextureResource&& texture) -> std::uint32_t {
        const HeapSlot slot = m_resource_heap.allocate();
        m_resource_heap.write_sampled_image(slot, texture.image.view_create_info(), vk::ImageLayout::eShaderReadOnlyOptimal);
        texture.image.set_heap_index(DescriptorHeap::shader_index(slot));
        m_texture_resources.emplace_back(std::move(texture));
        return static_cast<std::uint32_t>(m_texture_resources.size());
    }

    auto RenderResourceRegistry::register_dummy_texture() -> void {
        static constexpr std::array<std::uint8_t, 4> white{255, 255, 255, 255};
        const TextureData texture_data {
            .width = 1, .height = 1, .num_channels = 4,
            .pixel_data = std::span(const_cast<std::uint8_t*>(white.data()), white.size()),
        };
        const std::uint32_t handle = store_texture(create_texture_resource(texture_data));
        m_dummy_texture_heap_index = get_texture_resource(handle)->image.heap_index();
    }
} // flux