//
// Created by Admin on 28/06/2026.
//

#include "cook.hpp"

#include <glaze/glaze.hpp>

#include "assimp_import.hpp"
#include "hash.hpp"
#include "mesh_build.hpp"
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
        asset.src_uri = model_path.filename().string();

        Assimp::Importer importer;
        const aiScene* scene = import_scene(importer, model_path);
        if (!scene) {
            return std::unexpected(importer.GetErrorString());
        }
        auto walk = [&](this auto& self, const aiNode* node, std::int32_t parent) -> void {
            auto thresh_node = make_node(node, parent);
            if (node->mNumMeshes > 0) {
                thresh_node.mesh = static_cast<std::int32_t>(asset.meshes.size());
                asset.meshes.push_back(build_mesh_entry(scene, node));
            }

            const auto self_idx = static_cast<std::int32_t>(asset.nodes.size());
            asset.nodes.push_back(std::move(thresh_node));
            for (unsigned i = 0; i < node->mNumChildren; ++i) {
                self(node->mChildren[i], self_idx);
            }
        };
        walk(scene->mRootNode, -1);

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