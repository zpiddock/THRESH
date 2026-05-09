//
// Created by Admin on 09/05/2026.
//

#include "mesh_primitive.hpp"

auto flux::primitives::box(const flux::float3 extents) -> MeshData {

    MeshData mesh;

    const float w = extents.x * 0.5f;
    const float h = extents.y * 0.5f;
    const float d = extents.z * 0.5f;

    // 24 vertices (4 per face)
    mesh.vertices = {
        // +X (right)
            {{ w,-h,-d}, { 1,0,0}, {0,0}},
            {{ w, h,-d}, { 1,0,0}, {1,0}},
            {{ w, h, d}, { 1,0,0}, {1,1}},
            {{ w,-h, d}, { 1,0,0}, {0,1}},

            // -X (left)
            {{-w,-h, d}, {-1,0,0}, {0,0}},
            {{-w, h, d}, {-1,0,0}, {1,0}},
            {{-w, h,-d}, {-1,0,0}, {1,1}},
            {{-w,-h,-d}, {-1,0,0}, {0,1}},

            // +Y (top)
            {{-w, h,-d}, {0,1,0}, {0,0}},
            {{-w, h, d}, {0,1,0}, {0,1}},
            {{ w, h, d}, {0,1,0}, {1,1}},
            {{ w, h,-d}, {0,1,0}, {1,0}},

            // -Y (bottom)
            {{-w,-h, d}, {0,-1,0}, {0,0}},
            {{-w,-h,-d}, {0,-1,0}, {0,1}},
            {{ w,-h,-d}, {0,-1,0}, {1,1}},
            {{ w,-h, d}, {0,-1,0}, {1,0}},

            // +Z (front)
            {{-w,-h, d}, {0,0,1}, {0,0}},
            {{ w,-h, d}, {0,0,1}, {1,0}},
            {{ w, h, d}, {0,0,1}, {1,1}},
            {{-w, h, d}, {0,0,1}, {0,1}},

            // -Z (back)
            {{ w,-h,-d}, {0,0,-1}, {0,0}},
            {{-w,-h,-d}, {0,0,-1}, {1,0}},
            {{-w, h,-d}, {0,0,-1}, {1,1}},
            {{ w, h,-d}, {0,0,-1}, {0,1}},
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
