//
// Created by Admin on 28/06/2026.
//

#include "mesh_build.hpp"

#include "assimp/GltfMaterial.h"
#include "thresh/asset/material_flags.hpp"

namespace ferret {

    auto append_mesh(const aiMesh* mesh, thresh::asset::ModelAsset& out) -> MeshRange {
        using thresh::asset::MeshVertex;

        const auto base_vertex = out.vertex_count; // where this mesh starts in the merged VBO
        const auto base_index  = out.index_count;  // ... and the merged IBO

        out.vertices.reserve(out.vertices.size() + mesh->mNumVertices);
        for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
            MeshVertex v{};
            // Mesh-local, UNTRANSFORMED — placement lives in Submesh.local now.
            v.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
            if (mesh->mNormals)          v.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
            if (mesh->mTextureCoords[0]) v.uv     = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            if (mesh->mTangents && mesh->mBitangents && mesh->mNormals) {
                const aiVector3D& t = mesh->mTangents[i];
                const aiVector3D& b = mesh->mBitangents[i];
                const aiVector3D& n = mesh->mNormals[i];
                const float handed = (n ^ t) * b < 0.f ? -1.f : 1.f; // aiVector3D::operator^ = cross
                v.tangent = { t.x, t.y, t.z, handed };
            }
            out.vertices.push_back(v);
        }

        out.indices.reserve(out.indices.size() + static_cast<std::size_t>(mesh->mNumFaces) * 3);
        for (unsigned f = 0; f < mesh->mNumFaces; ++f) { // triangulated → exactly 3 per face
            const aiFace& face = mesh->mFaces[f];
            for (unsigned e = 0; e < 3; ++e) out.indices.push_back(base_vertex + face.mIndices[e]); // rebase
        }

        out.vertex_count = static_cast<std::uint32_t>(out.vertices.size());
        out.index_count  = static_cast<std::uint32_t>(out.indices.size());

        return {
            .index_offset = base_index,
            .index_count  = out.index_count - base_index,
            .local_aabb   = { { mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z },  // GenBoundingBoxes
                              { mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z } },
        };
    }

    auto build_material(const aiMaterial* mat) -> thresh::asset::MaterialEntry {
        thresh::asset::MaterialEntry e;

        aiColor4D base{1, 1, 1, 1};
        if (mat->Get(AI_MATKEY_BASE_COLOR, base) != AI_SUCCESS)
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, base);                  // FBX / legacy Phong fallback
        e.base_colour_factor = { base.r, base.g, base.b, base.a };

        float metallic = 1.f, roughness = 1.f;
        mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);               // absent on legacy → stays default
        mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
        e.metallic_factor = metallic;
        e.roughness_factor = roughness;

        aiColor3D emissive{0, 0, 0};
        mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
        e.emissive_factor = { emissive.r, emissive.g, emissive.b };

        float cutoff = 0.5f;
        mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff);
        e.alpha_cutoff = cutoff;

        int twosided = 0;
        mat->Get(AI_MATKEY_TWOSIDED, twosided);
        aiString alpha_mode;
        const bool mask = mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alpha_mode) == AI_SUCCESS
                       && std::string_view{alpha_mode.C_Str()} == "MASK";
        const bool has_normal   = mat->GetTextureCount(aiTextureType_NORMALS) > 0;
        const bool has_emissive = mat->GetTextureCount(aiTextureType_EMISSIVE) > 0
                               || (emissive.r + emissive.g + emissive.b) > 0.f;
        if (mask)         e.flags |= thresh::asset::MATERIAL_FLAG_ALPHA_MASK;
        if (twosided)     e.flags |= thresh::asset::MATERIAL_FLAG_DOUBLE_SIDED;
        if (has_normal)   e.flags |= thresh::asset::MATERIAL_FLAG_HAS_NORMAL;
        if (has_emissive) e.flags |= thresh::asset::MATERIAL_FLAG_HAS_EMISSIVE;

        // Per-slot usage + colour space (the bake pass fills content hash):
        e.base_colour_texture.usage        = thresh::asset::TextureUsage::BaseColour; e.base_colour_texture.colour_space               = thresh::asset::ColourSpace::sRGB;
        e.normal_texture.usage             = thresh::asset::TextureUsage::Normal; e.normal_texture.colour_space                        = thresh::asset::ColourSpace::Linear;
        e.metallic_roughness_texture.usage = thresh::asset::TextureUsage::MetallicRoughness; e.metallic_roughness_texture.colour_space = thresh::asset::ColourSpace::Linear;
        e.occlusion_texture.usage          = thresh::asset::TextureUsage::Occlusion; e.occlusion_texture.colour_space                  = thresh::asset::ColourSpace::Linear;
        e.emissive_texture.usage           = thresh::asset::TextureUsage::Emissive; e.emissive_texture.colour_space                    = thresh::asset::ColourSpace::sRGB;
        return e;
    }
} // ferret
