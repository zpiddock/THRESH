//
// Created by Admin on 28/06/2026.
//

#include "cook.hpp"

#include <unordered_map>

#include <glaze/glaze.hpp>

#include "assimp_import.hpp"
#include "hash.hpp"
#include "mesh_build.hpp"
#include "texture_bake.hpp"
#include "zstd.h"
#include "assimp/Importer.hpp"
#include "thresh/asset/model_asset.hpp"

namespace ferret {

    auto encode_thresh_model(const thresh::asset::ModelAsset& asset, bool should_compress) -> std::expected<std::vector<std::uint8_t>, std::string> {

        std::string beve;

        if (auto error = glz::write_beve(asset, beve)) {
            return std::unexpected(std::string(glz::format_error(error, beve)));
        }
        thresh::asset::ModelFileHeader header{};
        header.payload_size = beve.size();
        header.model_content_hash = hash_bytes({reinterpret_cast<const std::uint8_t*>(beve.data()), beve.size()});

        std::vector<std::uint8_t> body;
        if (should_compress) {
            header.flags |= 1u;
            body.resize(ZSTD_compressBound(header.payload_size));
            const std::size_t bytes_compressed = ZSTD_compress(body.data(), body.size(), beve.data(), beve.size(), 19);
            if (ZSTD_isError(bytes_compressed)) {
                return std::unexpected(ZSTD_getErrorName(bytes_compressed));
            }
            body.resize(bytes_compressed);
        } else {
            body.assign(beve.begin(), beve.end());
        }
        header.compressed_size = body.size();

        std::vector<std::uint8_t> result(sizeof(thresh::asset::ModelFileHeader) + body.size());
        std::memcpy(result.data(), &header, sizeof(thresh::asset::ModelFileHeader));
        std::memcpy(result.data() + sizeof(thresh::asset::ModelFileHeader), body.data(), body.size());
        return result;
    }

    auto cook_model(const std::filesystem::path& model_path, const std::filesystem::path& out_model,
        const CookOptions& options) -> std::expected<CookResult, std::string> {

        if (!std::filesystem::exists(model_path)) {
            return std::unexpected("Model file does not exist!");
        }
        thresh::asset::ModelAsset asset{};
        asset.name = out_model.stem().string();

        Assimp::Importer importer;
        const aiScene* scene = import_scene(importer, model_path);
        if (!scene) {
            return std::unexpected(importer.GetErrorString());
        }

        // The hierarchy is consumed here: transforms accumulate into Submesh.local, geometry is
        // cooked once per aiMesh (instanced nodes share the range), and nothing node-shaped survives.
        std::unordered_map<unsigned, MeshRange> cooked; // aiScene mesh index -> merged-buffer range
        auto walk = [&](this auto& self, const aiNode* node, const helix::float4x4& parent_global) -> void {
            const helix::float4x4 global = parent_global * to_helix(node->mTransformation);

            for (unsigned k = 0; k < node->mNumMeshes; ++k) {
                const unsigned mesh_idx = node->mMeshes[k];
                auto it = cooked.find(mesh_idx);
                if (it == cooked.end()) {
                    it = cooked.emplace(mesh_idx, append_mesh(scene->mMeshes[mesh_idx], asset)).first;
                }
                const MeshRange& range = it->second;
                asset.submeshes.push_back({
                    .local          = global,
                    .aabb           = range.local_aabb,
                    .index_offset   = range.index_offset,
                    .index_count    = range.index_count,
                    .material_index = scene->mMeshes[mesh_idx]->mMaterialIndex,
                });
                asset.aabb.expand(helix::transform_aabb(range.local_aabb, global));
            }
            for (unsigned i = 0; i < node->mNumChildren; ++i) {
                self(node->mChildren[i], global); // root's own transform participates — exporters
            }                                     // park unit/axis fixes there; don't identity it away
        };
        walk(scene->mRootNode, helix::float4x4{1.f});

        // Textures: per-asset subdir keyed by the cooked name. texture_bake already content-hashes,
        // dedups, and skips existing files within whatever dir it's handed.
        std::filesystem::path tex_dir;
        if (!options.texture_out_dir.empty()) {
            tex_dir = std::filesystem::path{options.texture_out_dir} / asset.name;
            std::filesystem::create_directories(tex_dir);
        }
        for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
            auto entry = build_material(scene->mMaterials[i]);
            cook_material_textures(scene, model_path.parent_path(), scene->mMaterials[i],
                                   options.codec, tex_dir, entry);
            asset.materials.push_back(std::move(entry));
        }
        if (auto w = write_thresh_model(out_model, asset, options.should_compress); !w) {
            return std::unexpected(w.error());
        }
        return CookResult{out_model, {}};
    }

    auto write_thresh_model(const std::filesystem::path& path, const thresh::asset::ModelAsset& asset,
        bool should_compress) -> std::expected<void, std::string> {
        auto bytes = encode_thresh_model(asset, should_compress);
        if (!bytes) return std::unexpected(bytes.error());
        std::ofstream f(path, std::ios::binary);
        if (!f) return std::unexpected(std::format("cannot open '{}' for writing", path.string()));
        f.write(reinterpret_cast<const char*>(bytes->data()), static_cast<std::streamsize>(bytes->size()));
        if (!f) return std::unexpected(std::format("write to '{}' failed", path.string()));
        return {};
    }
} // ferret