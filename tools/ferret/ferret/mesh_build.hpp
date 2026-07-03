//
// Created by Admin on 28/06/2026.
//

#pragma once
#include "assimp/scene.h"
#include "thresh/asset/model_asset.hpp"

namespace ferret {

    // Where one aiMesh landed in the model's merged buffers, plus its local-space bounds.
    struct MeshRange {
        std::uint32_t index_offset = 0;
        std::uint32_t index_count = 0;
        helix::AABB local_aabb{};
    };

    // Appends one aiMesh's geometry to the merged vertex/index buffers (indices rebased so the
    // range is self-sufficient — no per-draw vertex offset needed). Call at most once per aiMesh.
    auto append_mesh(const aiMesh* mesh, thresh::asset::ModelAsset& out) -> MeshRange;

    auto build_material(const aiMaterial* mat) -> thresh::asset::MaterialEntry;
} // ferret
