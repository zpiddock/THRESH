//
// Created by Admin on 28/06/2026.
//

#pragma once
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "thresh/asset/model_asset.hpp"

namespace ferret {
    enum TextureCodec {
        RGBA8,
        UASTC_BC7
    };
    struct CookOptions {
        std::string texture_out_dir;
        TextureCodec codec = UASTC_BC7;
        bool should_compress = true;
    };
    struct CookResult {
        std::filesystem::path output_path;
        std::vector<std::uint64_t> texture_hashes;
    };

    auto encode_thresh_model(
        const thresh::asset::ModelAsset& asset,
        bool should_compress) -> std::expected<std::vector<std::uint8_t>, std::string>;

    auto cook_model(
        const std::filesystem::path& model_path,
        const std::filesystem::path& out_model,
        const CookOptions& options) -> std::expected<CookResult, std::string>;

    auto write_thresh_model(
        const std::filesystem::path& path,
        const thresh::asset::ModelAsset& asset,
        bool should_compress) -> std::expected<void, std::string>;

} // ferret
