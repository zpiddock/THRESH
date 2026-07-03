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

        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

        const aiScene* scene = importer.ReadFile(src.string(), flags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            return nullptr;
        }
        return scene; // freed when importer goes out of scope
    }

    auto to_helix(aiMatrix4x4 m) -> helix::float4x4 {
        return {
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        };
    }
} // ferret