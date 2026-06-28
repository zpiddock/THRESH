//
// Created by Admin on 28/06/2026.
//

#pragma once
#include "assimp/scene.h"
#include "thresh/asset/model_asset.hpp"

namespace ferret {

    auto build_mesh_entry(const aiScene* scene, const aiNode* node) -> thresh::asset::MeshEntry;
    auto build_material(const aiMaterial* mat) -> thresh::asset::MaterialEntry;

    auto component_min(std::array<float, 3> a, std::array<float, 3> b) -> std::array<float, 3>;
    auto component_max(std::array<float, 3> a, std::array<float, 3> b) -> std::array<float, 3>;
} // ferret
