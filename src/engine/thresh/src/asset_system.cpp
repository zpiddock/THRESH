#include "thresh/asset_system.hpp"
#include "thresh/scene.hpp"
#include "thresh/components.hpp"
#include "substratum/vfs.hpp"
#include "substratum/log.hpp"

#include <filesystem>
#include <stdexcept>

namespace thresh {

    AssetSystem::AssetSystem(flux::Device &device)
        : m_device{device}
        , m_texture_loader{device}
        , m_mesh_loader{device} {
        SUB_INFO("AssetSystem initialized");
    }

    // ── Mesh loading ────────────────────────────────────────────────────────

    auto AssetSystem::load_mesh(const std::string &virtual_path) -> AssetHandle<flux::Mesh> {
        // Check cache first
        auto it = m_mesh_cache.find(virtual_path);
        if (it != m_mesh_cache.end()) {
            SUB_DEBUG("Mesh cache hit: {}", virtual_path);
            return it->second;
        }

        // Read file via VFS
        auto data = substratum::VFS::read_file(virtual_path);
        if (data.empty()) {
            SUB_ERROR("Failed to read mesh file: {}", virtual_path);
            return {};
        }

        // Parse glTF
        auto result = m_mesh_loader.load_gltf(data, virtual_path);
        if (result.meshes.empty()) {
            SUB_ERROR("Failed to parse mesh: {}", virtual_path);
            return {};
        }

        // Register the first mesh (cached by the virtual path)
        auto handle = register_mesh(std::move(result.meshes[0]), virtual_path);

        // Register additional meshes with unique names
        for (std::size_t i = 1; i < result.meshes.size(); ++i) {
            auto extra_name = virtual_path + "#mesh" + std::to_string(i);
            register_mesh(std::move(result.meshes[i]), extra_name);
        }

        if (handle.is_valid()) {
            SUB_INFO("Loaded mesh: {} (index {})", virtual_path, handle.index);
        }

        return handle;
    }

    auto AssetSystem::register_mesh(
        std::unique_ptr<flux::Mesh> mesh,
        const std::string &name
    ) -> AssetHandle<flux::Mesh> {
        if (!mesh) return {};

        auto index = static_cast<std::uint32_t>(m_meshes.size());

        m_meshes.push_back(std::move(mesh));
        m_mesh_generations.push_back(1);

        auto handle = AssetHandle<flux::Mesh>{
            .index = index,
            .generation = 1
        };

        if (!name.empty()) {
            m_mesh_cache[name] = handle;
        }

        return handle;
    }

    auto AssetSystem::get_mesh(AssetHandle<flux::Mesh> handle) const -> const flux::Mesh & {
        if (!handle.is_valid() || handle.index >= m_meshes.size()) {
            throw std::runtime_error("Invalid mesh handle");
        }
        if (m_mesh_generations[handle.index] != handle.generation || !m_meshes[handle.index]) {
            throw std::runtime_error("Stale mesh handle");
        }
        return *m_meshes[handle.index];
    }

    auto AssetSystem::get_mesh(AssetHandle<flux::Mesh> handle) -> flux::Mesh & {
        if (!handle.is_valid() || handle.index >= m_meshes.size()) {
            throw std::runtime_error("Invalid mesh handle");
        }
        if (m_mesh_generations[handle.index] != handle.generation || !m_meshes[handle.index]) {
            throw std::runtime_error("Stale mesh handle");
        }
        return *m_meshes[handle.index];
    }

    auto AssetSystem::get_meshes() const -> const std::vector<std::unique_ptr<flux::Mesh>> & {
        return m_meshes;
    }

    auto AssetSystem::get_mesh_count() const -> std::uint32_t {
        return static_cast<std::uint32_t>(m_meshes.size());
    }

    // ── Texture loading ─────────────────────────────────────────────────────

    auto AssetSystem::load_texture(
        const std::string &virtual_path,
        flux::TextureType type
    ) -> AssetHandle<flux::Texture> {
        // Check cache first
        auto it = m_texture_cache.find(virtual_path);
        if (it != m_texture_cache.end()) {
            SUB_DEBUG("Texture cache hit: {}", virtual_path);
            return it->second;
        }

        // Read file via VFS
        auto data = substratum::VFS::read_file(virtual_path);
        if (data.empty()) {
            SUB_ERROR("Failed to read texture file: {}", virtual_path);
            return {};
        }

        // Load texture (auto-detects format by extension)
        auto texture = m_texture_loader.load(data, virtual_path, type);
        if (!texture) {
            SUB_ERROR("Failed to load texture: {}", virtual_path);
            return {};
        }

        auto handle = register_texture(std::move(texture), virtual_path);

        if (handle.is_valid()) {
            SUB_INFO("Loaded texture: {} (index {})", virtual_path, handle.index);
        }

        return handle;
    }

    auto AssetSystem::register_texture(
        std::unique_ptr<flux::Texture> texture,
        const std::string &name
    ) -> AssetHandle<flux::Texture> {
        if (!texture) return {};

        auto index = static_cast<std::uint32_t>(m_textures.size());

        m_textures.push_back(std::move(texture));
        m_texture_generations.push_back(1);

        auto handle = AssetHandle<flux::Texture>{
            .index = index,
            .generation = 1
        };

        if (!name.empty()) {
            m_texture_cache[name] = handle;
        }

        return handle;
    }

    auto AssetSystem::get_texture(AssetHandle<flux::Texture> handle) const -> const flux::Texture & {
        if (!handle.is_valid() || handle.index >= m_textures.size()) {
            throw std::runtime_error("Invalid texture handle");
        }
        if (m_texture_generations[handle.index] != handle.generation || !m_textures[handle.index]) {
            throw std::runtime_error("Stale texture handle");
        }
        return *m_textures[handle.index];
    }

    auto AssetSystem::get_texture_count() const -> std::uint32_t {
        return static_cast<std::uint32_t>(m_textures.size());
    }

    // ── Full glTF scene import ──────────────────────────────────────────────

    auto AssetSystem::load_gltf_scene(
        const std::string &virtual_path,
        Scene &scene,
        MaterialSystem &material_system
    ) -> std::uint32_t {
        // Read file via VFS
        auto data = substratum::VFS::read_file(virtual_path);
        if (data.empty()) {
            SUB_ERROR("Failed to read glTF file: {}", virtual_path);
            return 0;
        }

        // Parse glTF
        auto result = m_mesh_loader.load_gltf(data, virtual_path);
        if (result.meshes.empty()) {
            SUB_ERROR("Failed to parse glTF: {}", virtual_path);
            return 0;
        }

        // Get the directory of the glTF file for resolving relative texture paths
        auto gltf_dir = std::filesystem::path(virtual_path).parent_path().string();
        if (!gltf_dir.empty() && gltf_dir.back() != '/') {
            gltf_dir += '/';
        }

        // Create materials from glTF material descriptions
        std::vector<MaterialHandle> material_handles;
        material_handles.reserve(result.materials.size());

        for (const auto &mat_desc : result.materials) {
            auto gpu_mat = GpuMaterialData{};

            // Set factors
            gpu_mat.base_color_factor = {
                mat_desc.base_color_factor[0],
                mat_desc.base_color_factor[1],
                mat_desc.base_color_factor[2],
                mat_desc.base_color_factor[3]
            };
            gpu_mat.metallic_factor = mat_desc.metallic_factor;
            gpu_mat.roughness_factor = mat_desc.roughness_factor;
            gpu_mat.emissive_factor = {
                mat_desc.emissive_factor[0],
                mat_desc.emissive_factor[1],
                mat_desc.emissive_factor[2],
                0.0f
            };

            // Load textures if paths are specified
            // Note: texture indices in GpuMaterialData refer to MaterialSystem's
            // texture array. For now, we load textures into the AssetSystem but
            // defer MaterialSystem texture binding to when Set 2 (bindless) is added.
            if (!mat_desc.albedo_path.empty()) {
                auto tex_path = gltf_dir + mat_desc.albedo_path;
                auto tex_handle = load_texture(tex_path, flux::TextureType::Albedo);
                if (tex_handle.is_valid()) {
                    SUB_DEBUG("Loaded albedo texture for material '{}': {}", mat_desc.name, tex_path);
                }
            }

            if (!mat_desc.normal_path.empty()) {
                auto tex_path = gltf_dir + mat_desc.normal_path;
                auto tex_handle = load_texture(tex_path, flux::TextureType::Normal);
                if (tex_handle.is_valid()) {
                    SUB_DEBUG("Loaded normal texture for material '{}': {}", mat_desc.name, tex_path);
                }
            }

            if (!mat_desc.metallic_roughness_path.empty()) {
                auto tex_path = gltf_dir + mat_desc.metallic_roughness_path;
                auto tex_handle = load_texture(tex_path, flux::TextureType::MetallicRoughness);
                if (tex_handle.is_valid()) {
                    SUB_DEBUG("Loaded metallic-roughness texture for material '{}': {}", mat_desc.name, tex_path);
                }
            }

            if (!mat_desc.emissive_path.empty()) {
                auto tex_path = gltf_dir + mat_desc.emissive_path;
                auto tex_handle = load_texture(tex_path, flux::TextureType::Emissive);
                if (tex_handle.is_valid()) {
                    SUB_DEBUG("Loaded emissive texture for material '{}': {}", mat_desc.name, tex_path);
                }
            }

            material_handles.push_back(material_system.create_material(gpu_mat));
        }

        // Use default material if no materials were defined in the glTF
        if (material_handles.empty()) {
            material_handles.push_back(0); // Default material at index 0
        }

        // Register meshes and create entities
        std::uint32_t entity_count = 0;
        auto stem = std::filesystem::path(virtual_path).stem().string();

        for (std::size_t mesh_idx = 0; mesh_idx < result.meshes.size(); ++mesh_idx) {
            auto mesh_name = virtual_path + "#mesh" + std::to_string(mesh_idx);
            auto mesh_handle = register_mesh(std::move(result.meshes[mesh_idx]), mesh_name);

            if (!mesh_handle.is_valid()) continue;

            // Entity name from filename + mesh index
            auto entity_name = stem;
            if (result.meshes.size() > 1) {
                entity_name += "_" + std::to_string(mesh_idx);
            }

            auto entity = scene.create_entity(entity_name);

            scene.add_component<TransformComponent>(entity);

            auto &mesh_comp = scene.add_component<MeshComponent>(entity);
            mesh_comp.mesh_index = mesh_handle.index;

            // Assign first available material (submesh-level material assignment
            // comes with the full material/submesh pipeline)
            auto &mat_comp = scene.add_component<MaterialComponent>(entity);
            mat_comp.material_handle = material_handles[0];

            entity_count++;
        }

        SUB_INFO("Loaded glTF scene: {} -> {} entities, {} materials, {} textures",
                  virtual_path, entity_count, material_handles.size(), get_texture_count());

        return entity_count;
    }

} // namespace thresh
