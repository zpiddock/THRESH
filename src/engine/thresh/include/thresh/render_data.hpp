#pragma once

#include "material.hpp"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace thresh {

    /**
     * Single directional light (sun).
     */
    struct DirectionalLight {
        glm::vec3 direction = glm::normalize(glm::vec3{-0.5f, -1.0f, -0.3f});
        glm::vec3 color = {1.0f, 0.98f, 0.95f};
        float intensity = 1.5f;
    };

    /**
     * Per-object render data extracted from the scene.
     */
    struct RenderObject {
        glm::mat4 model_matrix{1.0f};
        std::uint32_t mesh_index = 0;
        MaterialHandle material_index = 0;
    };

    /**
     * All data the render thread needs for a single frame.
     * Produced by Scene::extract_render_data(), consumed by the ForwardPass.
     */
    struct FrameRenderData {
        glm::mat4 view{1.0f};
        glm::mat4 projection{1.0f};
        glm::vec3 camera_position{0.0f};
        DirectionalLight sun;
        std::vector<RenderObject> objects;
    };

} // namespace thresh
