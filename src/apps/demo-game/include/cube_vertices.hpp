#pragma once

#include <glm/glm.hpp>
#include <array>

namespace demo {

    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    // Brand colors (linear approximations)
    // Cyan:      #00F0FF -> (0.000, 0.941, 1.000)
    // Magenta:   #FF2D6A -> (1.000, 0.176, 0.416)
    // Amber:     #FFB800 -> (1.000, 0.722, 0.000)
    // Cyan Dim:  #00A0AA -> (0.000, 0.627, 0.667)
    // Primary:   #E8EDF5 -> (0.910, 0.929, 0.961)
    // Secondary: #6A7A90 -> (0.416, 0.478, 0.565)

    inline constexpr glm::vec3 CYAN    = {0.000f, 0.941f, 1.000f};
    inline constexpr glm::vec3 MAGENTA = {1.000f, 0.176f, 0.416f};
    inline constexpr glm::vec3 AMBER   = {1.000f, 0.722f, 0.000f};
    inline constexpr glm::vec3 CYAN_DIM = {0.000f, 0.627f, 0.667f};
    inline constexpr glm::vec3 PRIMARY  = {0.910f, 0.929f, 0.961f};
    inline constexpr glm::vec3 SECONDARY = {0.416f, 0.478f, 0.565f};

    // 36 vertices: 6 faces x 2 triangles x 3 vertices, CCW winding
    // Cube from (-0.5, -0.5, -0.5) to (0.5, 0.5, 0.5)
    inline constexpr std::array<Vertex, 36> CUBE_VERTICES = {{
        // Front face (+Z) — Cyan
        {{ -0.5f, -0.5f,  0.5f }, CYAN},
        {{  0.5f, -0.5f,  0.5f }, CYAN},
        {{  0.5f,  0.5f,  0.5f }, CYAN},
        {{  0.5f,  0.5f,  0.5f }, CYAN},
        {{ -0.5f,  0.5f,  0.5f }, CYAN},
        {{ -0.5f, -0.5f,  0.5f }, CYAN},

        // Back face (-Z) — Magenta
        {{  0.5f, -0.5f, -0.5f }, MAGENTA},
        {{ -0.5f, -0.5f, -0.5f }, MAGENTA},
        {{ -0.5f,  0.5f, -0.5f }, MAGENTA},
        {{ -0.5f,  0.5f, -0.5f }, MAGENTA},
        {{  0.5f,  0.5f, -0.5f }, MAGENTA},
        {{  0.5f, -0.5f, -0.5f }, MAGENTA},

        // Right face (+X) — Amber
        {{  0.5f, -0.5f,  0.5f }, AMBER},
        {{  0.5f, -0.5f, -0.5f }, AMBER},
        {{  0.5f,  0.5f, -0.5f }, AMBER},
        {{  0.5f,  0.5f, -0.5f }, AMBER},
        {{  0.5f,  0.5f,  0.5f }, AMBER},
        {{  0.5f, -0.5f,  0.5f }, AMBER},

        // Left face (-X) — Cyan Dim
        {{ -0.5f, -0.5f, -0.5f }, CYAN_DIM},
        {{ -0.5f, -0.5f,  0.5f }, CYAN_DIM},
        {{ -0.5f,  0.5f,  0.5f }, CYAN_DIM},
        {{ -0.5f,  0.5f,  0.5f }, CYAN_DIM},
        {{ -0.5f,  0.5f, -0.5f }, CYAN_DIM},
        {{ -0.5f, -0.5f, -0.5f }, CYAN_DIM},

        // Top face (+Y) — Primary
        {{ -0.5f,  0.5f,  0.5f }, PRIMARY},
        {{  0.5f,  0.5f,  0.5f }, PRIMARY},
        {{  0.5f,  0.5f, -0.5f }, PRIMARY},
        {{  0.5f,  0.5f, -0.5f }, PRIMARY},
        {{ -0.5f,  0.5f, -0.5f }, PRIMARY},
        {{ -0.5f,  0.5f,  0.5f }, PRIMARY},

        // Bottom face (-Y) — Secondary
        {{ -0.5f, -0.5f, -0.5f }, SECONDARY},
        {{  0.5f, -0.5f, -0.5f }, SECONDARY},
        {{  0.5f, -0.5f,  0.5f }, SECONDARY},
        {{  0.5f, -0.5f,  0.5f }, SECONDARY},
        {{ -0.5f, -0.5f,  0.5f }, SECONDARY},
        {{ -0.5f, -0.5f, -0.5f }, SECONDARY},
    }};

} // namespace demo
