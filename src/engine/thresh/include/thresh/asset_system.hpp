#pragma once

#include "asset_handle.hpp"
#include "material.hpp"
#include "material_system.hpp"
#include "mesh_loader.hpp"
#include "flux/device.hpp"
#include "flux/mesh.hpp"
#include "flux/texture.hpp"
#include "flux/texture_loader.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
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
    class Scene;

    /**
     * Handle-based resource manager with VFS integration and deduplication.
     *
     * All file reads go through substratum::VFS, so assets work transparently
     * with mounted directories (dev mode) or archives (release mode).
     *
     * Features:
     * - Virtual-path-based cache: loading the same path twice returns the same handle
     * - Generation counter on handles for dangling-handle detection
     * - Auto-detect texture format: .ktx2 -> KTX2 loader, .png/.jpg -> stb_image
     * - Full glTF scene import: creates entities with meshes + materials in a Scene
     *
     * Meshes are stored in a flat vector indexed directly by AssetHandle::index,
     * which is the same index used in MeshComponent::mesh_index for rendering.
     *
     * Usage:
     *   AssetSystem assets(device);
     *   auto mesh_h = assets.load_mesh("models/cube.glb");
     *   auto tex_h  = assets.load_texture("textures/stone.png", TextureType::Albedo);
     *   auto &mesh  = assets.get_mesh(mesh_h);
     */
    class THRESH_API AssetSystem {
    public:
        explicit AssetSystem(flux::Device &device);
        ~AssetSystem() = default;

        AssetSystem(const AssetSystem &) = delete;
        auto operator=(const AssetSystem &) -> AssetSystem & = delete;

        // ── Mesh loading ────────────────────────────────────────────────────

        /**
         * Load a mesh from a glTF/glb file via VFS. Returns the first mesh.
         * Deduplicates by virtual path.
         * @param virtual_path VFS path (e.g. "models/cube.glb")
         * @return Handle to the loaded mesh, or invalid handle on failure
         */
        [[nodiscard]] auto load_mesh(const std::string &virtual_path) -> AssetHandle<flux::Mesh>;

        /**
         * Register an externally-created mesh. Takes ownership.
         * @param mesh The mesh to register
         * @param name Optional name for cache key (empty = not cached)
         * @return Handle to the registered mesh
         */
        auto register_mesh(std::unique_ptr<flux::Mesh> mesh, const std::string &name = "") -> AssetHandle<flux::Mesh>;

        /**
         * Get a mesh by handle.
         * @throws std::runtime_error if handle is invalid or stale
         */
        [[nodiscard]] auto get_mesh(AssetHandle<flux::Mesh> handle) const -> const flux::Mesh &;

        /**
         * Get a mutable mesh by handle.
         * @throws std::runtime_error if handle is invalid or stale
         */
        [[nodiscard]] auto get_mesh(AssetHandle<flux::Mesh> handle) -> flux::Mesh &;

        /**
         * Get direct access to the mesh array (for ForwardPass rendering).
         * Indexed by AssetHandle<Mesh>::index / MeshComponent::mesh_index.
         */
        [[nodiscard]] auto get_meshes() const -> const std::vector<std::unique_ptr<flux::Mesh>> &;

        /**
         * Get the total number of loaded meshes.
         */
        [[nodiscard]] auto get_mesh_count() const -> std::uint32_t;

        // ── Texture loading ─────────────────────────────────────────────────

        /**
         * Load a texture from a file via VFS.
         * Auto-detects format by extension (.ktx2 -> KTX2, .png/.jpg -> stb).
         * Deduplicates by virtual path.
         * @param virtual_path VFS path
         * @param type Semantic texture type (Albedo, Normal, etc.)
         * @return Handle to the loaded texture, or invalid handle on failure
         */
        [[nodiscard]] auto load_texture(
            const std::string &virtual_path,
            flux::TextureType type = flux::TextureType::Albedo
        ) -> AssetHandle<flux::Texture>;

        /**
         * Register an externally-created texture. Takes ownership.
         */
        auto register_texture(
            std::unique_ptr<flux::Texture> texture,
            const std::string &name = ""
        ) -> AssetHandle<flux::Texture>;

        /**
         * Get a texture by handle.
         * @throws std::runtime_error if handle is invalid or stale
         */
        [[nodiscard]] auto get_texture(AssetHandle<flux::Texture> handle) const -> const flux::Texture &;

        /**
         * Get the total number of loaded textures.
         */
        [[nodiscard]] auto get_texture_count() const -> std::uint32_t;

        // ── Full glTF scene import ──────────────────────────────────────────

        /**
         * Load a glTF file and populate a Scene with entities.
         *
         * For each mesh/material in the glTF:
         * - Loads meshes as AssetSystem resources
         * - Loads textures (if present) and creates materials in the MaterialSystem
         * - Creates entities in the Scene with Transform, Mesh, and Material components
         *
         * @param virtual_path VFS path to .gltf or .glb file
         * @param scene Target scene to populate
         * @param material_system Material system for creating materials
         * @return Number of entities created, or 0 on failure
         */
        auto load_gltf_scene(
            const std::string &virtual_path,
            Scene &scene,
            MaterialSystem &material_system
        ) -> std::uint32_t;

    private:
        flux::Device &m_device;
        flux::TextureLoader m_texture_loader;
        MeshLoader m_mesh_loader;

        // Mesh storage — flat vector, index = handle.index = MeshComponent::mesh_index
        std::vector<std::unique_ptr<flux::Mesh>> m_meshes;
        std::vector<std::uint32_t> m_mesh_generations;
        std::unordered_map<std::string, AssetHandle<flux::Mesh>> m_mesh_cache;

        // Texture storage
        std::vector<std::unique_ptr<flux::Texture>> m_textures;
        std::vector<std::uint32_t> m_texture_generations;
        std::unordered_map<std::string, AssetHandle<flux::Texture>> m_texture_cache;
    };

} // namespace thresh
