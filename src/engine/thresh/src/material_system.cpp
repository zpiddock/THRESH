#include "thresh/material_system.hpp"
#include "substratum/log.hpp"

#include <cstring>
#include <stdexcept>

namespace thresh {

    static constexpr std::uint32_t INITIAL_MATERIAL_CAPACITY = 64;

    MaterialSystem::MaterialSystem(flux::Device &device)
        : m_device{device} {
        // Create default textures
        flux::TextureLoader loader(device);

        auto white = loader.create_default_white();
        if (white) {
            m_default_white_index = add_texture(std::move(white));
        }

        auto normal = loader.create_default_normal();
        if (normal) {
            m_default_normal_index = add_texture(std::move(normal));
        }

        // Create default material (index 0) using white/normal defaults
        GpuMaterialData default_mat{};
        default_mat.albedo_tex_index = m_default_white_index;
        default_mat.normal_tex_index = m_default_normal_index;
        create_material(default_mat);

        // Pre-allocate SSBO buffers
        create_ssbo_buffers();

        SUB_INFO("MaterialSystem initialized: default textures (white={}, normal={})",
                 m_default_white_index, m_default_normal_index);
    }

    MaterialSystem::~MaterialSystem() {
        auto vk_device = m_device.get_logical_device();
        for (auto &ssbo : m_ssbo_buffers) {
            destroy_ssbo_buffer(ssbo);
        }
    }

    auto MaterialSystem::create_material(const GpuMaterialData &data) -> MaterialHandle {
        auto handle = static_cast<MaterialHandle>(m_materials.size());
        m_materials.push_back(data);
        mark_all_dirty();
        return handle;
    }

    auto MaterialSystem::get_material_data(MaterialHandle handle) -> GpuMaterialData & {
        if (handle >= m_materials.size()) {
            SUB_FATAL("Material handle {} out of range ({})", handle, m_materials.size());
            throw std::out_of_range("Invalid material handle");
        }
        mark_all_dirty(); // Assume modification
        return m_materials[handle];
    }

    auto MaterialSystem::get_material_data(MaterialHandle handle) const -> const GpuMaterialData & {
        if (handle >= m_materials.size()) {
            SUB_FATAL("Material handle {} out of range ({})", handle, m_materials.size());
            throw std::out_of_range("Invalid material handle");
        }
        return m_materials[handle];
    }

    auto MaterialSystem::add_texture(std::unique_ptr<flux::Texture> texture) -> std::int32_t {
        auto index = static_cast<std::int32_t>(m_textures.size());
        m_textures.push_back(std::move(texture));
        return index;
    }

    auto MaterialSystem::get_texture(std::int32_t index) const -> const flux::Texture & {
        if (index < 0 || static_cast<std::size_t>(index) >= m_textures.size()) {
            SUB_FATAL("Texture index {} out of range ({})", index, m_textures.size());
            throw std::out_of_range("Invalid texture index");
        }
        return *m_textures[static_cast<std::size_t>(index)];
    }

    auto MaterialSystem::get_texture_count() const -> std::uint32_t {
        return static_cast<std::uint32_t>(m_textures.size());
    }

    auto MaterialSystem::get_material_count() const -> std::uint32_t {
        return static_cast<std::uint32_t>(m_materials.size());
    }

    auto MaterialSystem::get_material_ssbo(std::uint32_t frame_index) const -> VkBuffer {
        return m_ssbo_buffers[frame_index].buffer;
    }

    auto MaterialSystem::get_material_ssbo_size() const -> VkDeviceSize {
        return static_cast<VkDeviceSize>(m_materials.size()) * sizeof(GpuMaterialData);
    }

    auto MaterialSystem::update_gpu_data(std::uint32_t frame_index) -> void {
        if (!m_dirty[frame_index] && m_ssbo_buffers[frame_index].buffer != VK_NULL_HANDLE) {
            return;
        }

        grow_ssbo_if_needed();

        auto &ssbo = m_ssbo_buffers[frame_index];
        if (ssbo.mapped && !m_materials.empty()) {
            std::memcpy(ssbo.mapped, m_materials.data(),
                        m_materials.size() * sizeof(GpuMaterialData));
        }

        m_dirty[frame_index] = false;
    }

    auto MaterialSystem::mark_all_dirty() -> void {
        for (auto &d : m_dirty) {
            d = true;
        }
    }

    // ── Internal: raw Vulkan buffer management ──────────────────────────────────

    auto MaterialSystem::create_ssbo_buffer(SsboBuffer &ssbo, VkDeviceSize capacity) -> void {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        m_device.create_buffer(
            capacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffer, memory
        );

        void *mapped = nullptr;
        ::vkMapMemory(m_device.get_logical_device(), memory, 0, capacity, 0, &mapped);

        ssbo.buffer = buffer;
        ssbo.memory = memory;
        ssbo.mapped = mapped;
        ssbo.capacity = capacity;
    }

    auto MaterialSystem::destroy_ssbo_buffer(SsboBuffer &ssbo) -> void {
        if (ssbo.buffer != VK_NULL_HANDLE) {
            auto vk_device = m_device.get_logical_device();
            if (ssbo.mapped) {
                ::vkUnmapMemory(vk_device, ssbo.memory);
                ssbo.mapped = nullptr;
            }
            ::vkDestroyBuffer(vk_device, ssbo.buffer, nullptr);
            ::vkFreeMemory(vk_device, ssbo.memory, nullptr);
            ssbo.buffer = VK_NULL_HANDLE;
            ssbo.memory = VK_NULL_HANDLE;
            ssbo.capacity = 0;
        }
    }

    auto MaterialSystem::create_ssbo_buffers() -> void {
        VkDeviceSize capacity = INITIAL_MATERIAL_CAPACITY * sizeof(GpuMaterialData);
        for (auto &ssbo : m_ssbo_buffers) {
            create_ssbo_buffer(ssbo, capacity);
        }
    }

    auto MaterialSystem::grow_ssbo_if_needed() -> void {
        VkDeviceSize needed = static_cast<VkDeviceSize>(m_materials.size()) * sizeof(GpuMaterialData);
        if (needed == 0) needed = sizeof(GpuMaterialData);

        bool needs_grow = false;
        for (const auto &ssbo : m_ssbo_buffers) {
            if (ssbo.capacity < needed) {
                needs_grow = true;
                break;
            }
        }

        if (!needs_grow) return;

        VkDeviceSize new_capacity = m_ssbo_buffers[0].capacity;
        while (new_capacity < needed) {
            new_capacity *= 2;
        }

        SUB_DEBUG("Growing material SSBO: {} -> {} bytes", m_ssbo_buffers[0].capacity, new_capacity);

        m_device.wait_idle();

        for (auto &ssbo : m_ssbo_buffers) {
            destroy_ssbo_buffer(ssbo);
            create_ssbo_buffer(ssbo, new_capacity);
        }

        // All frames need re-upload since buffers were recreated
        mark_all_dirty();
    }

} // namespace thresh
