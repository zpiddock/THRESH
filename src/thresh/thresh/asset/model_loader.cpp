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

    auto ModelLoader::prefab_for(flecs::world& world, const std::string& vfs_path) -> flecs::entity {

        if (auto it = m_prefab_by_path.find(vfs_path); it != m_prefab_by_path.end()) return it->second;
        auto model = load_model_asset(vfs_path);                          // propagate error in real code
        auto root  = build_prefab(world, *model);
        m_prefab_by_path.emplace(vfs_path, root);
        return root;
    }
    auto ModelLoader::spawn(flecs::world& world, const std::string& path, flecs::entity parent) -> flecs::entity {
        auto inst = world.entity().is_a(prefab_for(world, path));     // deep-copies the prefab child hierarchy
        inst.child_of(parent);
        return inst;
    }

    auto ModelLoader::build_prefab(flecs::world& world, const asset::ModelAsset& asset) -> flecs::entity {
        std::vector<std::uint32_t> mesh_handles;
        mesh_handles.reserve(asset.meshes.size());
        for (const auto& m : asset.meshes) mesh_handles.push_back(upload_mesh(m));

        std::vector<std::uint32_t> mat_handles;
        mat_handles.reserve(asset.materials.size());
        for (const auto& mat : asset.materials) {
            mat_handles.push_back(register_material(mat));
        }

        auto root = world.prefab();                                   // Prefab-tagged, unscoped (not under SceneRoot)
        std::vector<flecs::entity> ents(asset.nodes.size());
        for (std::size_t i = 0; i < asset.nodes.size(); ++i) {
            const auto& n = asset.nodes[i];
            auto p = world.prefab();                                  // unnamed → no sibling name collisions
            p.set<Transform>({ {n.translation[0], n.translation[1], n.translation[2]},
                               {n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]},  // glm::quat(w,x,y,z) ← xyzw
                               {n.scale[0], n.scale[1], n.scale[2]} });
            p.child_of(n.parent_index < 0 ? root : ents[n.parent_index]);         // ChildOf among prefab entities
            if (n.mesh >= 0)
                for (const auto& sm : asset.meshes[n.mesh].submeshes) {
                    const std::uint32_t mat_handle = sm.material_index < mat_handles.size() ? mat_handles[sm.material_index] : default_material();
                    world.prefab().child_of(p)                        // one prefab child per submesh
                         .set<Transform>({})                          // identity; node carries the transform
                         .set<Mesh>({ mesh_handles[n.mesh], mat_handle, sm.index_offset, sm.index_count });
                }
            ents[i] = p;
        }
        return root;
    }

    auto ModelLoader::upload_mesh(const asset::MeshEntry& asset) -> std::uint32_t {
        const helix::AABB aabb {
            {asset.aabb_min[0], asset.aabb_min[1], asset.aabb_min[2]},
            {asset.aabb_max[0], asset.aabb_max[1], asset.aabb_max[2]}
        };
        return m_registry.register_mesh_data(std::as_bytes(std::span{asset.vertices}),
                                         std::as_bytes(std::span{asset.indices}), asset.index_count, aabb);
    }

    auto ModelLoader::register_material(const asset::MaterialEntry& entry) -> std::uint32_t {

        const flux::gpu::MaterialData md{
            .base_colour_factor = { entry.base_colour_factor[0], entry.base_colour_factor[1],
                                    entry.base_colour_factor[2], entry.base_colour_factor[3] },
            .emissive_factor    = { entry.emissive_factor[0], entry.emissive_factor[1], entry.emissive_factor[2] },
            .metallic_factor = entry.metallic_factor, .roughness_factor = entry.roughness_factor,
            .normal_scale = entry.normal_scale, .occlusion_strength = entry.occlusion_strength,
            .alpha_cutoff = entry.alpha_cutoff,
            .base_colour_texture_handle        = resolve_texture(entry.base_colour_texture),
            .normal_texture_handle             = resolve_texture(entry.normal_texture),
            .emissive_texture_handle           = resolve_texture(entry.emissive_texture),
            .metallic_roughness_texture_handle = resolve_texture(entry.metallic_roughness_texture),
            .occlusion_texture_handle          = resolve_texture(entry.occlusion_texture),
            .flags = entry.flags,
        };
        return m_registry.register_material(md);
    }

    auto ModelLoader::resolve_texture(const asset::TextureRef& ref) -> std::uint32_t {

        // Empty slot → the engine default for that usage (flat-normal for normals, white otherwise).
        const std::uint64_t hash = ref.hash ? ref.hash
            : (ref.usage == asset::TextureUsage::Normal ? asset::FLAT_NORMAL_HASH : asset::WHITE_HASH);
        if (hash == 0) return m_registry.dummy_texture_handle();            // defaults not cooked/pinned

        if (auto it = m_texture_by_hash.find(hash); it != m_texture_by_hash.end())
            return it->second;                                             // one heap slot per unique texture/default

        const auto bytes = substratum::VFS::read_file(std::format("textures/{:016x}.ktx2", hash));
        if (bytes.empty()) {
            std::println("resolve_texture: missing sidecar textures/{:016x}.ktx2 (using dummy)", hash);
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