#pragma once

#include "material.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <entt/entt.hpp>

namespace thresh {

    /**
     * Transform component — position, rotation, scale.
     */
    struct TransformComponent {
        glm::vec3 position{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // Identity quaternion
        glm::vec3 scale{1.0f};

        /**
         * Compute the model matrix: T * R * S
         */
        [[nodiscard]] auto get_model_matrix() const -> glm::mat4 {
            auto mat = glm::translate(glm::mat4{1.0f}, position);
            mat *= glm::mat4_cast(rotation);
            mat = glm::scale(mat, scale);
            return mat;
        }
    };

    /**
     * Mesh component — references a mesh by index into the mesh array.
     */
    struct MeshComponent {
        std::uint32_t mesh_index = 0;
    };

    /**
     * Material component — references a material by handle.
     */
    struct MaterialComponent {
        MaterialHandle material_handle = 0;
    };

    /**
     * Tag component — human-readable name for debugging/editor.
     */
    struct TagComponent {
        std::string name;
    };

    /**
     * Hierarchy component — parent-child relationships.
     */
    struct HierarchyComponent {
        entt::entity parent = entt::null;
        std::vector<entt::entity> children;
    };

    /**
     * Point light component — omnidirectional light source.
     * Attach to an entity with a TransformComponent; position comes from the transform.
     */
    struct PointLightComponent {
        glm::vec3 color = {1.0f, 1.0f, 1.0f};
        float intensity = 1.0f;
        float radius = 10.0f;
    };

} // namespace thresh
