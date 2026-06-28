//
// Created by Admin on 28/06/2026.
//

#include "texture_bake.hpp"

#include <array>
#include <cassert>
#include <format>
#include <optional>
#include <print>
#include <vector>

#include "thresh/asset/material_flags.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <fstream>
#include <stb_image.h>
#include <stb_image_resize2.h>

#include <assimp/material.h>
#include <assimp/scene.h>

#include "hash.hpp"

namespace ferret {
    auto bake_pixels(const std::uint8_t* pixels, int w, int h, thresh::asset::TextureUsage usage,
        ferret::TextureCodec codec, const std::filesystem::path& out_dir) -> std::uint64_t {
        const bool srgb   = (usage == thresh::asset::TextureUsage::BaseColour || usage == thresh::asset::TextureUsage::Emissive);
        const auto levels = 1u + static_cast<std::uint32_t>(std::floor(std::log2(std::max(w, h))));

        ktxTextureCreateInfo ci{};
        ci.vkFormat        = srgb ? VKFMT_R8G8B8A8_SRGB : VKFMT_R8G8B8A8_UNORM;
        ci.baseWidth       = static_cast<ktx_uint32_t>(w);
        ci.baseHeight      = static_cast<ktx_uint32_t>(h);
        ci.baseDepth       = 1;  ci.numDimensions = 2;
        ci.numLevels       = levels;  ci.numLayers = 1;  ci.numFaces = 1;
        ci.isArray         = KTX_FALSE;  ci.generateMipmaps = KTX_FALSE;

        ktxTexture2* tex = nullptr;
        if (ktxTexture2_Create(&ci, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &tex) != KTX_SUCCESS) return 0;

        for (std::uint32_t l = 0; l < levels; ++l) {
            const int lw = std::max(1, w >> l), lh = std::max(1, h >> l);
            std::vector<std::uint8_t> level(static_cast<std::size_t>(lw) * lh * 4);
            if (l == 0)    std::memcpy(level.data(), pixels, level.size());
            else if (srgb) stbir_resize_uint8_srgb  (pixels, w, h, 0, level.data(), lw, lh, 0, STBIR_RGBA);
            else           stbir_resize_uint8_linear(pixels, w, h, 0, level.data(), lw, lh, 0, STBIR_RGBA);
            ktxTexture_SetImageFromMemory(ktxTexture(tex), l, 0, 0, level.data(), level.size());
        }

        if (codec == ferret::TextureCodec::UASTC_BC7) {
            ktxBasisParams p{};
            p.structSize = sizeof(p);
            p.uastc = KTX_TRUE;
            p.uastcFlags = KTX_PACK_UASTC_LEVEL_DEFAULT;
            ktxTexture2_CompressBasisEx(tex, &p);
        }

        ktx_uint8_t* out = nullptr; ktx_size_t out_size = 0;
        ktxTexture_WriteToMemory(ktxTexture(tex), &out, &out_size);
        const std::uint64_t hash = ferret::hash_bytes({ out, out_size });
        const auto path = out_dir / std::format("{:016x}.ktx2", hash);
        if (!std::filesystem::exists(path)) {                        // content-addressed dedup
            std::ofstream f(path, std::ios::binary);
            f.write(reinterpret_cast<const char*>(out), static_cast<std::streamsize>(out_size));
        }
        free(out);
        ktxTexture_Destroy(ktxTexture(tex));
        return hash;
    }

    auto bake_texture(std::span<const std::uint8_t> src_file, thresh::asset::TextureUsage usage, ferret::TextureCodec codec,
        const std::filesystem::path&                out_dir) -> std::expected<std::uint64_t, std::string> {
        int w, h, c;
        stbi_uc* px = stbi_load_from_memory(src_file.data(), static_cast<int>(src_file.size()), &w, &h, &c, STBI_rgb_alpha);
        if (!px) return std::unexpected(std::format("image decode failed: {}", stbi_failure_reason()));
        const std::uint64_t hash = bake_pixels(px, w, h, usage, codec, out_dir);
        stbi_image_free(px);
        if (hash == 0) return std::unexpected("ktx2 create/write failed");
        return hash;
    }

    auto bake_raw_rgba(std::span<const std::uint8_t> rgba, int w, int h, thresh::asset::TextureUsage usage,
        const std::filesystem::path& out_dir) -> std::uint64_t {
        return ferret::bake_pixels(rgba.data(), w, h, usage, ferret::TextureCodec::RGBA8, out_dir);
    }

    namespace {
        auto slot_type(thresh::asset::TextureUsage usage) -> aiTextureType {
            using thresh::asset::TextureUsage;
            switch (usage) {
                case TextureUsage::BaseColour:        return aiTextureType_BASE_COLOR;
                case TextureUsage::Normal:            return aiTextureType_NORMALS;
                case TextureUsage::MetallicRoughness: return aiTextureType_DIFFUSE_ROUGHNESS;  // glTF MR maps here
                case TextureUsage::Occlusion:         return aiTextureType_AMBIENT_OCCLUSION;
                case TextureUsage::Emissive:          return aiTextureType_EMISSIVE;
            }
            return aiTextureType_NONE;
        }

        // FBX/glTF external texture paths are unreliable (bare names, wrong folders, absolute artist paths).
        // Try: as-given relative to the model dir → basename in the model dir → basename one level down in
        // any subdir (covers e.g. Reaper_Textures/). Returns {} if nothing matches.
        auto resolve_external(const std::filesystem::path& src_dir, const char* tex_path) -> std::filesystem::path {
            namespace fs = std::filesystem;
            std::error_code ec;
            const fs::path given{ tex_path };
            if (fs::exists(src_dir / given, ec)) return src_dir / given;
            const fs::path name = given.filename();
            if (fs::exists(src_dir / name, ec)) return src_dir / name;
            for (const auto& e : fs::directory_iterator(src_dir, ec)) {
                if (e.is_directory(ec) && fs::exists(e.path() / name, ec)) return e.path() / name;
            }
            return {};
        }

        // Resolve a material texture slot's source bytes (external file OR embedded aiTexture), bake it to a
        // <hash>.ktx2 sidecar, and stamp the ref. No texture in the slot → ref.hash stays 0 (runtime → dummy).
        auto bake_slot(const aiScene* scene, const std::filesystem::path& src_dir, const aiMaterial* mat,
                       thresh::asset::TextureUsage usage, thresh::asset::ColourSpace colour_space,
                       ferret::TextureCodec codec, const std::filesystem::path& out_dir,
                       thresh::asset::TextureRef& ref) -> void {
            ref.usage        = usage;
            ref.colour_space = colour_space;

            aiString path;
            if (mat->GetTexture(slot_type(usage), 0, &path) != AI_SUCCESS) {
                if (usage != thresh::asset::TextureUsage::BaseColour) return;
                if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) != AI_SUCCESS) return;  // FBX/legacy fallback
            }

            std::optional<std::uint64_t> hash;
            if (const aiTexture* t = scene->GetEmbeddedTexture(path.C_Str())) {
                if (t->mHeight == 0) {                                   // compressed blob (png/jpg)
                    if (auto r = ferret::bake_texture(
                            { reinterpret_cast<const std::uint8_t*>(t->pcData), t->mWidth }, usage, codec, out_dir))
                        hash = *r;
                } else {                                                 // raw BGRA8888 aiTexel[w*h]
                    const std::size_t count = static_cast<std::size_t>(t->mWidth) * t->mHeight;
                    std::vector<std::uint8_t> rgba(count * 4);
                    for (std::size_t i = 0; i < count; ++i) {
                        rgba[i * 4 + 0] = t->pcData[i].r; rgba[i * 4 + 1] = t->pcData[i].g;
                        rgba[i * 4 + 2] = t->pcData[i].b; rgba[i * 4 + 3] = t->pcData[i].a;
                    }
                    hash = ferret::bake_raw_rgba(rgba, t->mWidth, t->mHeight, usage, out_dir);
                }
            } else {                                                     // external file
                const auto file = resolve_external(src_dir, path.C_Str());
                if (file.empty()) {
                    std::println(stderr, "  texture not found: '{}' (slot {})", path.C_Str(), static_cast<int>(usage));
                } else {
                    std::ifstream f(file, std::ios::binary | std::ios::ate);
                    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(f.tellg()));
                    f.seekg(0);
                    f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
                    auto r = ferret::bake_texture(bytes, usage, codec, out_dir);
                    if (r) hash = *r;
                    else   std::println(stderr, "  bake failed for '{}': {}", file.string(), r.error());
                }
            }
            if (hash) {
                ref.hash = *hash;
                std::println("  baked slot {} -> {:016x}", static_cast<int>(usage), *hash);
            }
        }
    } // namespace

    auto cook_material_textures(const aiScene* scene, const std::filesystem::path& src_dir, const aiMaterial* mat,
                                ferret::TextureCodec codec, const std::filesystem::path& out_dir,
                                thresh::asset::MaterialEntry& entry) -> void {
        using thresh::asset::ColourSpace;
        using thresh::asset::TextureUsage;
        bake_slot(scene, src_dir, mat, TextureUsage::BaseColour,        ColourSpace::sRGB,   codec, out_dir, entry.base_colour_texture);
        bake_slot(scene, src_dir, mat, TextureUsage::Normal,            ColourSpace::Linear, codec, out_dir, entry.normal_texture);
        bake_slot(scene, src_dir, mat, TextureUsage::MetallicRoughness, ColourSpace::Linear, codec, out_dir, entry.metallic_roughness_texture);
        bake_slot(scene, src_dir, mat, TextureUsage::Occlusion,         ColourSpace::Linear, codec, out_dir, entry.occlusion_texture);
        bake_slot(scene, src_dir, mat, TextureUsage::Emissive,          ColourSpace::sRGB,   codec, out_dir, entry.emissive_texture);
    }

    auto cook_default_textures(const std::filesystem::path& out_dir) -> void {
        std::filesystem::create_directories(out_dir);
        constexpr std::array<std::uint8_t, 4> white  { 255, 255, 255, 255 };
        constexpr std::array<std::uint8_t, 4> normal { 128, 128, 255, 255 };   // flat tangent-space normal
        const std::uint64_t w = bake_raw_rgba(white,  1, 1, thresh::asset::TextureUsage::BaseColour, out_dir);
        const std::uint64_t n = bake_raw_rgba(normal, 1, 1, thresh::asset::TextureUsage::Normal,     out_dir);
        std::println("WHITE_HASH       = 0x{:016x}", w);
        std::println("FLAT_NORMAL_HASH = 0x{:016x}", n);
        // 0 = not yet pinned; once pinned, these guard codec/format drift on re-cook.
        assert((thresh::asset::WHITE_HASH == 0       || w == thresh::asset::WHITE_HASH)       && "re-pin WHITE_HASH");
        assert((thresh::asset::FLAT_NORMAL_HASH == 0 || n == thresh::asset::FLAT_NORMAL_HASH) && "re-pin FLAT_NORMAL_HASH");
    }
} // ferret