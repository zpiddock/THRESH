//
// Created by Admin on 28/06/2026.
//

#pragma once
#include <filesystem>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "thresh/asset/model_asset.hpp"

namespace ferret {

    auto import_scene(Assimp::Importer& importer, const std::filesystem::path& src) -> const aiScene*;

    auto make_node(const aiNode* n, std::int32_t parent) -> thresh::asset::Node;
} // ferret
