//
// Created by Admin on 09/05/2026.
//
#pragma once
#include <vector>

#include "flux/graphics_types.hpp"


namespace flux {

    struct MeshData {

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    constexpr MeshData BOX = {
        .vertices = {
        },
        .indices = {
        }
    };
}
