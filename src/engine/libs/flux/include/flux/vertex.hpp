#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#ifdef _WIN32
#ifdef FLUX_EXPORTS
#define FLUX_API __declspec(dllexport)
#else
#define FLUX_API __declspec(dllimport)
#endif
#else
#define FLUX_API
#endif

namespace flux {

    /**
     * Standard vertex format used by all meshes in the engine.
     *
     * 48 bytes, tightly packed:
     *   position  (vec3)  — location 0, R32G32B32_SFLOAT
     *   normal    (vec3)  — location 1, R32G32B32_SFLOAT
     *   uv        (vec2)  — location 2, R32G32_SFLOAT
     *   tangent   (vec4)  — location 3, R32G32B32A32_SFLOAT (w = handedness ±1)
     */
    struct Vertex {
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        glm::vec2 uv{0.0f};
        glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
    };

    static_assert(sizeof(Vertex) == 48, "Vertex must be 48 bytes");

    /**
     * Get vertex input binding descriptions for VK_EXT_vertex_input_dynamic_state.
     * Single binding (0), per-vertex rate, stride = sizeof(Vertex).
     */
    [[nodiscard]] FLUX_API auto get_vertex_bindings()
        -> std::vector<VkVertexInputBindingDescription2EXT>;

    /**
     * Get vertex input attribute descriptions for VK_EXT_vertex_input_dynamic_state.
     * Matches the Vertex struct layout: position(0), normal(1), uv(2), tangent(3).
     */
    [[nodiscard]] FLUX_API auto get_vertex_attributes()
        -> std::vector<VkVertexInputAttributeDescription2EXT>;

} // namespace flux
