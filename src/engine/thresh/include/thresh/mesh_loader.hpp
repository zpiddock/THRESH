#pragma once

#include "flux/device.hpp"
#include "flux/mesh.hpp"
#include "flux/vertex.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#ifdef THRESH_EXPORTS
#define THRESH_API __declspec(dllexport)
#else
#define THRESH_API __declspec(dllimport)
#endif
#else
#define THRESH_API
#endif

namespace thresh {

    /**
     * Raw image data extracted from a glTF buffer view (embedded textures).
     * Contains the encoded bytes (PNG/JPG/etc.) and the MIME type.
     */
    struct EmbeddedTexture {
        std::vector<std::uint8_t> data;
        std::string mime_type;
    };

    /**
     * Describes a material referenced by a loaded glTF.
     * Textures may come as file paths (URI) or embedded buffer data.
     */
    struct MaterialDesc {
        std::string name;

        // External texture paths (populated when glTF image has a URI)
        std::string albedo_path;
        std::string normal_path;
        std::string metallic_roughness_path;
        std::string emissive_path;

        // Embedded texture data (populated when glTF image uses a buffer view)
        std::optional<EmbeddedTexture> albedo_embedded;
        std::optional<EmbeddedTexture> normal_embedded;
        std::optional<EmbeddedTexture> metallic_roughness_embedded;
        std::optional<EmbeddedTexture> emissive_embedded;

        float base_color_factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float metallic_factor = 1.0f;
        float roughness_factor = 1.0f;
        float emissive_factor[3] = {0.0f, 0.0f, 0.0f};
    };

    /**
     * Result of loading a glTF file.
     */
    struct MeshLoadResult {
        std::vector<std::unique_ptr<flux::Mesh>> meshes;
        std::vector<MaterialDesc> materials;
    };

    /**
     * Loads glTF 2.0 models using cgltf.
     *
     * Parses geometry into the standard flux::Vertex format and uploads
     * to device-local GPU buffers via flux::Mesh::create().
     *
     * Texture paths are extracted from glTF materials but NOT loaded —
     * the caller (AssetSystem or demo code) is responsible for loading textures.
     */
    class THRESH_API MeshLoader {
    public:
        explicit MeshLoader(flux::Device &device);
        ~MeshLoader() = default;

        MeshLoader(const MeshLoader &) = delete;
        auto operator=(const MeshLoader &) -> MeshLoader & = delete;

        /**
         * Load a glTF/glb file from raw bytes (e.g. VFS::read_file()).
         * @param data Raw file bytes
         * @param virtual_path Virtual path (used for resolving relative texture URIs)
         * @return Loaded meshes + material descriptions, or empty on failure
         */
        [[nodiscard]] auto load_gltf(
            const std::vector<std::uint8_t> &data,
            const std::string &virtual_path
        ) -> MeshLoadResult;

    private:
        flux::Device &m_device;
    };

} // namespace thresh
