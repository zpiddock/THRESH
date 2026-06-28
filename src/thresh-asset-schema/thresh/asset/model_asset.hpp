#pragma once
#include <array>
#include <vector>

namespace thresh::asset {
    inline constexpr std::array<char, 4> TMODEL_MAGIC = {'T', 'M', 'D', 'L'};
    inline constexpr std::uint32_t TMODEL_VERSION = 1;

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

        std::array<float, 3> position;
        std::array<float, 3> normal;
        std::array<float, 4> tangent;
        std::array<float, 2> uv;
    };
    static_assert(sizeof(MeshVertex) == 48);

    enum class VertexLayout : std::uint8_t {
        PositionNormalUV        = 0,
        PositionNormalTangentUV = 1,
    };
    enum class IndexType : std::uint8_t {
        Uint16 = 0,
        Uint32 = 1,
    };

    struct Submesh {
        std::uint32_t index_offset;
        std::uint32_t index_count;
        std::uint32_t material_index;
    };

    struct MeshEntry {
        VertexLayout layout = VertexLayout::PositionNormalTangentUV;
        IndexType    index_type = IndexType::Uint32;
        std::uint32_t vertex_count;
        std::uint32_t index_count;
        std::vector<std::uint8_t> vertices; // vertex_count * sizeof(MeshEntry)
        std::vector<std::uint8_t> indices; // index_count * stride(index_type)
        std::vector<Submesh> submeshes;
        std::array<float, 3> aabb_min{  std::numeric_limits<float>::max(),
                                        std::numeric_limits<float>::max(),
                                        std::numeric_limits<float>::max() };
        std::array<float, 3> aabb_max{  -std::numeric_limits<float>::max(),
                                        -std::numeric_limits<float>::max(),
                                        -std::numeric_limits<float>::max() }; // precomputed for WorldAABB/picking
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
        std::uint64_t hash = 0;
        ColourSpace   colour_space = ColourSpace::sRGB;
        TextureUsage  usage = TextureUsage::BaseColour;
    };

    struct MaterialEntry {
        std::array<float, 4> base_colour_factor{1, 1, 1, 1};
        std::array<float, 3> emissive_factor{0, 0, 0};
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

    struct Node {
        std::string name;
        std::int32_t parent_index = -1; // index in to nodes, -1 = root node
        std::array<float, 3> translation{0, 0, 0};
        std::array<float, 4> rotation{0, 0, 0, 1}; // quaternion xyzw
        std::array<float, 3> scale{1, 1, 1};
        std::int32_t mesh = -1; // Index in to meshes, -1 transform only
    };

    struct ModelAsset { // BEVE Root, nodes topologically orderer
        std::string src_uri;
        std::vector<MeshEntry> meshes;
        std::vector<MaterialEntry> materials;
        std::vector<Node> nodes;
    };
}
