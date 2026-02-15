#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace thresh {

    /**
     * GPU-side material data packed for SSBO (std430 layout).
     * 64 bytes, aligned to 16.
     */
    struct alignas(16) GpuMaterialData {
        glm::vec4 base_color_factor = {1.0f, 1.0f, 1.0f, 1.0f};  // 16
        float metallic_factor = 1.0f;                               // 4
        float roughness_factor = 1.0f;                              // 4
        float normal_scale = 1.0f;                                  // 4
        float occlusion_strength = 1.0f;                            // 4
        glm::vec4 emissive_factor = {0.0f, 0.0f, 0.0f, 0.0f};    // 16
        std::int32_t albedo_tex_index = -1;                         // 4
        std::int32_t normal_tex_index = -1;                         // 4
        std::int32_t metallic_roughness_tex_index = -1;            // 4
        std::int32_t emissive_tex_index = -1;                      // 4
        // Total: 64 bytes
    };

    static_assert(sizeof(GpuMaterialData) == 64, "GpuMaterialData must be 64 bytes");

    /**
     * Handle to a material in the MaterialSystem.
     * Index into the material data array.
     */
    using MaterialHandle = std::uint32_t;

    /** Sentinel value indicating no material. */
    constexpr MaterialHandle INVALID_MATERIAL = ~0u;

} // namespace thresh
