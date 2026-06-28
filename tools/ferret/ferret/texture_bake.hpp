//
// Created by Admin on 28/06/2026.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <ktx.h>

#include "cook.hpp"
#include "thresh/asset/model_asset.hpp"

struct aiScene;
struct aiMaterial;

namespace ferret {

    constexpr ktx_uint32_t VKFMT_R8G8B8A8_SRGB  = 43;   // VK_FORMAT_R8G8B8A8_SRGB
    constexpr ktx_uint32_t VKFMT_R8G8B8A8_UNORM = 37;   // VK_FORMAT_R8G8B8A8_UNORM

    auto bake_pixels(const std::uint8_t* pixels, int w, int h, thresh::asset::TextureUsage usage,
                     ferret::TextureCodec codec, const std::filesystem::path& out_dir) -> std::uint64_t;

    auto bake_texture(std::span<const std::uint8_t> src_file, thresh::asset::TextureUsage usage,
                          TextureCodec codec, const std::filesystem::path& out_dir) -> std::expected<std::uint64_t, std::string>;

    auto bake_raw_rgba(std::span<const std::uint8_t> rgba, int w, int h,
                       thresh::asset::TextureUsage usage, const std::filesystem::path& out_dir) -> std::uint64_t;

    // Resolve every texture slot of `mat` (external file or embedded), bake each to a <hash>.ktx2 sidecar
    // in `out_dir`, and fill `entry`'s TextureRefs (hash + usage + colour space). src_dir = the model's
    // directory (for resolving external texture paths).
    auto cook_material_textures(const aiScene* scene, const std::filesystem::path& src_dir, const aiMaterial* mat,
                                TextureCodec codec, const std::filesystem::path& out_dir,
                                thresh::asset::MaterialEntry& entry) -> void;
} // ferret
