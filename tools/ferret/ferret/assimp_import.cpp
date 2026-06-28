//
// Created by Admin on 28/06/2026.
//

#include "assimp_import.hpp"

#include "assimp/postprocess.h"

namespace ferret {
    auto import_scene(Assimp::Importer& importer, const std::filesystem::path& src) -> const aiScene* {

        constexpr unsigned flags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_GenBoundingBoxes |
            aiProcess_GlobalScale |
            aiProcess_FlipUVs; //glTF / FBX UV origin -> out convention, verify per format

        const aiScene* scene = importer.ReadFile(src.string(), flags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            return nullptr;
        }
        return scene; // freed when importer goes out of scope
    }

    auto make_node(const aiNode* n, std::int32_t parent) -> thresh::asset::Node {

        aiVector3D translation, scale;
        aiQuaternion roration;
        n->mTransformation.Decompose(scale, roration, translation);

        return {
            .name = n->mName.C_Str(),
            .parent_index = parent,
            .translation = {translation.x, translation.y, translation.z},
            .rotation = {roration.x, roration.y, roration.z, roration.w},
            .scale = {scale.x, scale.y, scale.z}
        };
    }
} // ferret