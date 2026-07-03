#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "helix/math.hpp"
#include "helix_glaze.hpp"

namespace thresh::asset {
    inline constexpr std::array<char, 4> TMODEL_MAGIC = {'T', 'M', 'D', 'L'};
    inline constexpr std::uint32_t TMODEL_VERSION = 3; // v2: merged buffers + per-submesh transform
                                                       // v3: per-submesh mesh-local AABB (picking)

    struct alignas(8) ModelFileHeader {
        std::array<char, 4> magic              = TMODEL_MAGIC;
        std::uint32_t       version            = TMODEL_VERSION;
        std::uint32_t       flags              = 0; // bit0: payload is zstd compressed
        std::uint32_t       _pad               = 0;
        std::uint64_t       model_content_hash = 0; // xxh3 of BEVE payload for dedup/integrity/scene ref
        std::uint64_t       payload_size       = 0; // Decompressed BEVE bytes
        std::uint64_t       compressed_size    = 0; // On-disk bytes after header
    };
    static_assert(sizeof(ModelFileHeader) == 40);

    struct MeshVertex {

        helix::float3 position;
        helix::float3 normal;
        helix::float4 tangent; // xyz + handedness in w
        helix::float2 uv;
    };
    static_assert(sizeof(MeshVertex) == 48);
    static_assert(std::is_trivially_copyable_v<MeshVertex>);

    // One draw: an index range into the merged buffers, placed by the source node's world transform.
    // Two source nodes referencing the same mesh emit two Submeshes sharing one range.
    struct Submesh {
        helix::float4x4 local{1.f}; // node-accumulated, model-space
        helix::AABB aabb;           // mesh-local (pre-`local`) bounds — per-submesh picking/highlight
        std::uint32_t index_offset{0};
        std::uint32_t index_count{0};
        std::uint32_t material_index{0}; // into materials[]; out-of-range = engine default
    };

    enum class ColourSpace : std::uint8_t {
        sRGB   = 0,
        Linear = 1
    };
    enum class TextureUsage : std::uint8_t {
        BaseColour,
        Normal,
        MetallicRoughness,
        Emissive,
        Occlusion,
    };
    struct TextureRef {
        std::uint64_t hash = 0; // 0 = engine default for the slot
        ColourSpace   colour_space = ColourSpace::sRGB;
        TextureUsage  usage = TextureUsage::BaseColour;
    };

    struct MaterialEntry {
        std::string shader_type = "opaque"; // registry material_type / pipeline name
        helix::float4 base_colour_factor{1.f, 1.f, 1.f, 1.f};
        helix::float3 emissive_factor{0.f, 0.f, 0.f};
        float metallic_factor = 1.f;
        float roughness_factor = 1.f;
        float normal_scale = 1.f;
        float occlusion_strength = 1.f;
        float alpha_cutoff = 0.5f;
        std::uint32_t flags = 0; // asset::MATERIAL_FLAG_* (== flux::gpu values)
        TextureRef base_colour_texture;
        TextureRef normal_texture;
        TextureRef emissive_texture;
        TextureRef metallic_roughness_texture;
        TextureRef occlusion_texture;
    };

    struct ModelAsset { // BEVE Root
        std::string name;                    // asset stem; also the texture subdir name
        std::uint32_t vertex_count = 0;
        std::uint32_t index_count = 0;
        std::vector<MeshVertex> vertices;    // one merged VBO
        std::vector<std::uint32_t> indices;  // one merged IBO
        std::vector<Submesh> submeshes;
        std::vector<MaterialEntry> materials;
        helix::AABB aabb;                    // whole model; starts sentinel-invalid, expand()-only
    };
}

// Serialize MeshVertex as 12 packed floats — without this, BEVE writes each vertex as an object
// with repeated field keys, bloating the payload by an order of magnitude on real meshes.
template<> struct glz::meta<thresh::asset::MeshVertex> {
    static constexpr auto value = thresh::asset::detail::float_span<12>;
};
