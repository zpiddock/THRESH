//
// Created by Admin on 09/05/2026.
//
#pragma once
#include <vector>

#include "flux/graphics_types.hpp"


namespace flux {

    struct MeshData {

        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
    };

    namespace primitives {

        auto box(flux::float3 extents = {1.f, 1.f, 1.f}) -> MeshData;

        auto sphere(float extent = 1.f, uint32_t sectors = 32, uint32_t stacks = 16) -> MeshData;
    }
}
