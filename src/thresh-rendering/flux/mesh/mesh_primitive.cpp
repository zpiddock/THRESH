//
// Created by Admin on 09/05/2026.
//

#include "mesh_primitive.hpp"

auto flux::primitives::box(const helix::float3 extents) -> MeshData {

    MeshData mesh;

    const float w = extents.x * 0.5f;
    const float h = extents.y * 0.5f;
    const float d = extents.z * 0.5f;

    // 24 vertices (4 per face)
    mesh.vertices = {
        // +X (right)
            {{ w,-h,-d}, { 1,0,0}, {}, {0,0}},
            {{ w, h,-d}, { 1,0,0}, {}, {1,0}},
            {{ w, h, d}, { 1,0,0}, {}, {1,1}},
            {{ w,-h, d}, { 1,0,0}, {}, {0,1}},

            // -X (left)
            {{-w,-h, d}, {-1,0,0}, {}, {0,0}},
            {{-w, h, d}, {-1,0,0}, {}, {1,0}},
            {{-w, h,-d}, {-1,0,0}, {}, {1,1}},
            {{-w,-h,-d}, {-1,0,0}, {}, {0,1}},

            // +Y (top)
            {{-w, h,-d}, {0,1,0}, {}, {0,0}},
            {{-w, h, d}, {0,1,0}, {}, {0,1}},
            {{ w, h, d}, {0,1,0}, {}, {1,1}},
            {{ w, h,-d}, {0,1,0}, {}, {1,0}},

            // -Y (bottom)
            {{-w,-h, d}, {0,-1,0}, {}, {0,0}},
            {{-w,-h,-d}, {0,-1,0}, {}, {0,1}},
            {{ w,-h,-d}, {0,-1,0}, {}, {1,1}},
            {{ w,-h, d}, {0,-1,0}, {}, {1,0}},

            // +Z (front)
            {{-w,-h, d}, {0,0,1}, {}, {0,0}},
            {{ w,-h, d}, {0,0,1}, {}, {1,0}},
            {{ w, h, d}, {0,0,1}, {}, {1,1}},
            {{-w, h, d}, {0,0,1}, {}, {0,1}},

            // -Z (back)
            {{ w,-h,-d}, {0,0,-1}, {}, {0,0}},
            {{-w,-h,-d}, {0,0,-1}, {}, {1,0}},
            {{-w, h,-d}, {0,0,-1}, {}, {1,1}},
            {{ w, h,-d}, {0,0,-1}, {}, {0,1}},
        };

    // Indices (6 faces × 2 triangles × 3 indices)
    mesh.indices = {
        0,1,2,  0,2,3,        // +X
        4,5,6,  4,6,7,        // -X
        8,9,10, 8,10,11,      // +Y
        12,13,14, 12,14,15,   // -Y
        16,17,18, 16,18,19,   // +Z
        20,21,22, 20,22,23    // -Z
    };

    return mesh;
}

auto flux::primitives::sphere(float extent, uint32_t sectors, uint32_t stacks)
    -> MeshData {

    MeshData mesh;

    constexpr auto PI = helix::pi<float>();

    mesh.vertices.reserve((stacks + 1) * (sectors + 1));
    mesh.indices.reserve(stacks * sectors * 6);

    const float radius = extent * 0.5f;

    const float sector_step = 2.f * PI / static_cast<float>(sectors);
    const float stack_step  =       PI / static_cast<float>(stacks);
    const float inv_radius  = 1.f / radius;

    for (std::uint32_t i = 0; i <= stacks; ++i) {
        const float stack_angle = PI * 0.5f - static_cast<float>(i) * stack_step; // +pi/2 .. -pi/2
        const float xy = radius * std::cos(stack_angle);
        const float z  = radius * std::sin(stack_angle);

        for (std::uint32_t j = 0; j <= sectors; ++j) {
            const float sector_angle = static_cast<float>(j) * sector_step; // 0 .. 2pi

            const helix::float3 position {
                xy * std::cos(sector_angle),
                z,
                xy * std::sin(sector_angle),
            };
            const helix::float3 normal {
                position.x * inv_radius,
                position.y * inv_radius,
                position.z * inv_radius,
            };
            const helix::float4 tangent = {

            };
            const helix::float2 uv {
                static_cast<float>(j) / static_cast<float>(sectors),
                static_cast<float>(i) / static_cast<float>(stacks),
            };
            mesh.vertices.push_back({ position, normal, tangent, uv });
        }
    }

    for (std::uint32_t i = 0; i < stacks; ++i) {
        std::uint32_t k1 =  i      * (sectors + 1);
        std::uint32_t k2 = (i + 1) * (sectors + 1);

        for (std::uint32_t j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {                       // skip top cap upper-tri
                mesh.indices.insert(mesh.indices.end(), { k1, k1 + 1, k2 });
            }
            if (i != stacks - 1) {              // skip bottom cap lower-tri
                mesh.indices.insert(mesh.indices.end(), { k1 + 1, k2 + 1, k2 });
            }
        }
    }

    return mesh;
}
