#pragma once

#include "material.hpp"
#include "flux/device.hpp"
#include "flux/texture.hpp"
#include "flux/texture_loader.hpp"
#include "renderer/frame_context.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

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
     * Manages materials and their GPU-side SSBO representation.
     *
     * - Owns all material data and textures
     * - Provides per-frame SSBO buffers for shader access
     * - Creates default white and flat-normal textures on init
     * - Default material (index 0) is always available
     */
    class THRESH_API MaterialSystem {
    public:
        explicit MaterialSystem(flux::Device &device);
        ~MaterialSystem();

        MaterialSystem(const MaterialSystem &) = delete;
        auto operator=(const MaterialSystem &) -> MaterialSystem & = delete;

        /**
         * Create a material from GPU data. Returns the material handle (index).
         */
        [[nodiscard]] auto create_material(const GpuMaterialData &data) -> MaterialHandle;

        /**
         * Get mutable reference to material data for modification.
         */
        [[nodiscard]] auto get_material_data(MaterialHandle handle) -> GpuMaterialData &;

        /**
         * Get const reference to material data.
         */
        [[nodiscard]] auto get_material_data(MaterialHandle handle) const -> const GpuMaterialData &;

        /**
         * Add a texture to the system. Returns the texture index for use in GpuMaterialData.
         * Ownership is transferred to the MaterialSystem.
         */
        auto add_texture(std::unique_ptr<flux::Texture> texture) -> std::int32_t;

        /**
         * Get a texture by index.
         */
        [[nodiscard]] auto get_texture(std::int32_t index) const -> const flux::Texture &;

        /**
         * Get the number of textures registered.
         */
        [[nodiscard]] auto get_texture_count() const -> std::uint32_t;

        /**
         * Get the number of materials.
         */
        [[nodiscard]] auto get_material_count() const -> std::uint32_t;

        /**
         * Get the material SSBO for a given frame index.
         * Call update_gpu_data() first to ensure data is current.
         */
        [[nodiscard]] auto get_material_ssbo(std::uint32_t frame_index) const -> VkBuffer;

        /**
         * Get the total size of the material SSBO in bytes.
         */
        [[nodiscard]] auto get_material_ssbo_size() const -> VkDeviceSize;

        /**
         * Re-upload material data to the GPU SSBO for the given frame.
         * Call once per frame before rendering.
         */
        auto update_gpu_data(std::uint32_t frame_index) -> void;

        /**
         * Index of the default white texture.
         */
        [[nodiscard]] auto get_default_white_index() const -> std::int32_t { return m_default_white_index; }

        /**
         * Index of the default flat normal texture.
         */
        [[nodiscard]] auto get_default_normal_index() const -> std::int32_t { return m_default_normal_index; }

    private:
        // Per-frame SSBO buffers (host-visible + coherent for easy upload)
        struct SsboBuffer {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            void *mapped = nullptr;
            VkDeviceSize capacity = 0;
        };

        auto create_ssbo_buffer(SsboBuffer &ssbo, VkDeviceSize capacity) -> void;
        auto destroy_ssbo_buffer(SsboBuffer &ssbo) -> void;
        auto create_ssbo_buffers() -> void;
        auto grow_ssbo_if_needed() -> void;

        flux::Device &m_device;

        // Material data (CPU side)
        std::vector<GpuMaterialData> m_materials;
        // Per-frame dirty flags — each frame's SSBO must be updated independently
        std::array<bool, MAX_FRAMES_IN_FLIGHT> m_dirty{};
        auto mark_all_dirty() -> void;

        // Textures
        std::vector<std::unique_ptr<flux::Texture>> m_textures;
        std::int32_t m_default_white_index = -1;
        std::int32_t m_default_normal_index = -1;

        std::array<SsboBuffer, MAX_FRAMES_IN_FLIGHT> m_ssbo_buffers{};
    };

} // namespace thresh
