//
// Created by Admin on 28/06/2026.
//

#include "model_loader.hpp"

#include <algorithm>
#include <format>
#include <ktx.h>

#include "substratum/filesystem/vfs.hpp"
#include "thresh/asset/material_flags.hpp"
#include "thresh/asset/model_io.hpp"
#include "thresh/scene/ecs_types.hpp"

namespace thresh {
    ModelLoader::ModelLoader(flux::RenderResourceRegistry& registry) : m_registry(registry) {
    }

    auto ModelLoader::load_model_asset(const std::string& vfs_path) -> std::expected<asset::ModelAsset, asset::DecodeError> {

        const auto bytes = substratum::VFS::read_file(vfs_path);

        if (bytes.empty()) {

            std::println("Model {} does not exist!", vfs_path);
            return std::unexpected(asset::DecodeError::BAD_FILE);
        }
        return asset::decode_thresh_model(bytes);
    }

    auto ModelLoader::get_or_load(const std::string& vfs_path) -> const LoadedModel* {

        if (auto it = m_models.find(vfs_path); it != m_models.end()) return &it->second;

        auto decoded = load_model_asset(vfs_path);
        if (!decoded) {
            std::println("ModelLoader: failed to decode '{}'", vfs_path);
            return nullptr;
        }
        const auto& asset = *decoded;

        LoadedModel model;
        model.name = asset.name.empty() ? "model" : asset.name;
        model.mesh_handle = m_registry.register_mesh_data(
            std::as_bytes(std::span{asset.vertices}),
            std::as_bytes(std::span{asset.indices}),
            asset.index_count, asset.aabb);

        std::vector<std::uint32_t> mat_handles;
        mat_handles.reserve(asset.materials.size());
        for (const auto& entry : asset.materials) {
            mat_handles.push_back(register_material(entry, model.name));
        }

        model.submeshes.reserve(asset.submeshes.size());
        for (const auto& sm : asset.submeshes) {
            const std::uint32_t mat_handle = sm.material_index < mat_handles.size()
                                           ? mat_handles[sm.material_index] : default_material();
            model.submeshes.push_back({ mat_handle, sm.index_offset, sm.index_count, sm.local, sm.aabb });
        }

        auto [it, inserted] = m_models.emplace(vfs_path, std::move(model));
        return &it->second;
    }

    auto ModelLoader::spawn(flecs::world& world, const std::string& vfs_path, flecs::entity parent) -> flecs::entity {

        const auto* model = get_or_load(vfs_path);
        if (!model) return flecs::entity{};

        // flecs sibling names must be unique — suffix repeat spawns under the same parent.
        std::string name = model->name;
        for (int n = 1; parent.lookup(name.c_str()); ++n) {
            name = std::format("{}_{}", model->name, n);
        }

        auto entity = world.entity().child_of(parent);
        entity.set_name(name.c_str());
        entity.set<Transform>({});
        entity.set<MeshRenderer>({ model->mesh_handle, model->submeshes }); // small vector copy per instance
        return entity;
    }

    auto ModelLoader::register_material(const asset::MaterialEntry& entry, const std::string& model_name) -> std::uint32_t {

        const flux::gpu::MaterialData md{
            .base_colour_factor = entry.base_colour_factor,   // helix on both sides — no unpacking
            .emissive_factor    = entry.emissive_factor,
            .metallic_factor = entry.metallic_factor, .roughness_factor = entry.roughness_factor,
            .normal_scale = entry.normal_scale, .occlusion_strength = entry.occlusion_strength,
            .alpha_cutoff = entry.alpha_cutoff,
            .base_colour_texture_handle        = resolve_texture(entry.base_colour_texture, model_name),
            .normal_texture_handle             = resolve_texture(entry.normal_texture, model_name),
            .emissive_texture_handle           = resolve_texture(entry.emissive_texture, model_name),
            .metallic_roughness_texture_handle = resolve_texture(entry.metallic_roughness_texture, model_name),
            .occlusion_texture_handle          = resolve_texture(entry.occlusion_texture, model_name),
            .flags = entry.flags,
        };
        return m_registry.register_material(md, entry.shader_type); // shader string -> material_type
    }

    auto ModelLoader::resolve_texture(const asset::TextureRef& ref, const std::string& model_name) -> std::uint32_t {

        // Empty slot → the engine default for that usage (flat-normal for normals, white otherwise).
        const bool is_default = ref.hash == 0;
        const std::uint64_t hash = is_default
            ? (ref.usage == asset::TextureUsage::Normal ? asset::FLAT_NORMAL_HASH : asset::WHITE_HASH)
            : ref.hash;
        if (hash == 0) return m_registry.dummy_texture_handle();            // defaults not cooked/pinned

        if (auto it = m_texture_by_hash.find(hash); it != m_texture_by_hash.end())
            return it->second;                                             // one heap slot per unique texture/default

        // Defaults live in the flat root; asset textures in the per-asset subdir.
        const auto vfs_path = is_default
            ? std::format("textures/{:016x}.ktx2", hash)
            : std::format("textures/{}/{:016x}.ktx2", model_name, hash);
        const auto bytes = substratum::VFS::read_file(vfs_path);
        if (bytes.empty()) {
            std::println("resolve_texture: missing sidecar {} (using dummy)", vfs_path);
            return m_registry.dummy_texture_handle();
        }

        ktxTexture2* tex = nullptr;
        if (ktxTexture2_CreateFromMemory(bytes.data(), bytes.size(),
                KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex) != KTX_SUCCESS) {
            std::println("resolve_texture: KTX2 parse failed for {:016x}", hash);
            return m_registry.dummy_texture_handle();
        }

        if (ktxTexture2_NeedsTranscoding(tex)) {                           // UASTC/ETC1S → BC7
            if (ktxTexture2_TranscodeBasis(tex, KTX_TTF_BC7_RGBA, 0) != KTX_SUCCESS) {
                ktxTexture_Destroy(ktxTexture(tex));
                return m_registry.dummy_texture_handle();
            }
        }

        const auto* data  = ktxTexture_GetData(ktxTexture(tex));
        const auto  total = ktxTexture_GetDataSize(ktxTexture(tex));
        std::vector<flux::MipRegion> regions(tex->numLevels);
        for (std::uint32_t l = 0; l < tex->numLevels; ++l) {
            ktx_size_t offset = 0;
            ktxTexture_GetImageOffset(ktxTexture(tex), l, 0, 0, &offset);
            regions[l] = {
                .buffer_offset = offset,
                .extent        = { std::max(1u, tex->baseWidth >> l), std::max(1u, tex->baseHeight >> l) },
                .mip_level     = l,
            };
        }

        const std::uint32_t handle = m_registry.register_texture_mips(
            { reinterpret_cast<const std::byte*>(data), total }, regions,
            { tex->baseWidth, tex->baseHeight }, static_cast<vk::Format>(tex->vkFormat), tex->numLevels);
        ktxTexture_Destroy(ktxTexture(tex));

        const std::uint32_t slot = m_registry.get_texture_resource(handle)->image.heap_index();
        m_texture_by_hash.emplace(hash, slot);
        return slot;
    }

    auto ModelLoader::default_material() -> std::uint32_t {
        if (m_default_material == 0) m_default_material = m_registry.register_material(0u, helix::Colour(1.f));
        return m_default_material;
    }
} // thresh
