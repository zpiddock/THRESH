#pragma once

#include "flux/vertex.hpp"
#include "flux/mesh.hpp"
#include "flux/device.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace demo {

    /**
     * Create a unit cube mesh [-0.5, 0.5] with proper normals, UVs, and tangents.
     * Uses indexed geometry (24 vertices, 36 indices) for the standard flux::Vertex format.
     */
    inline auto create_cube_mesh(flux::Device &device) -> std::unique_ptr<flux::Mesh> {
        // 24 vertices: 4 per face, 6 faces
        // Each face has its own normal + tangent + UVs
        std::array<flux::Vertex, 24> vertices = {{
            // Front face (+Z) — normal = (0, 0, 1), tangent = (1, 0, 0, 1)
            {{ -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{ -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},

            // Back face (-Z) — normal = (0, 0, -1), tangent = (-1, 0, 0, 1)
            {{  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f, 1.0f }},
            {{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f, 1.0f }},
            {{ -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f, 1.0f }},
            {{  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f, 1.0f }},

            // Right face (+X) — normal = (1, 0, 0), tangent = (0, 0, -1, 1)
            {{  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f, 1.0f }},
            {{  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f, 1.0f }},
            {{  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f, 1.0f }},
            {{  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f, 1.0f }},

            // Left face (-X) — normal = (-1, 0, 0), tangent = (0, 0, 1, 1)
            {{ -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }},
            {{ -0.5f, -0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }},
            {{ -0.5f,  0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }},
            {{ -0.5f,  0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }},

            // Top face (+Y) — normal = (0, 1, 0), tangent = (1, 0, 0, 1)
            {{ -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{ -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},

            // Bottom face (-Y) — normal = (0, -1, 0), tangent = (1, 0, 0, 1)
            {{ -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{  0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{  0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
            {{ -0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
        }};

        // 36 indices: 6 faces x 2 triangles x 3 vertices, CCW winding
        std::array<std::uint32_t, 36> indices = {{
            // Front
             0,  1,  2,   2,  3,  0,
            // Back
             4,  5,  6,   6,  7,  4,
            // Right
             8,  9, 10,  10, 11,  8,
            // Left
            12, 13, 14,  14, 15, 12,
            // Top
            16, 17, 18,  18, 19, 16,
            // Bottom
            20, 21, 22,  22, 23, 20,
        }};

        auto submeshes = std::vector<flux::SubMesh>{
            { .index_offset = 0, .index_count = 36, .material_index = 0 }
        };

        return flux::Mesh::create(
            device,
            std::span<const flux::Vertex>(vertices.data(), vertices.size()),
            std::span<const std::uint32_t>(indices.data(), indices.size()),
            std::move(submeshes)
        );
    }

} // namespace demo
