//
// Created by Admin on 28/06/2026.
//

#include "mesh_build.hpp"

#include "assimp/GltfMaterial.h"
#include "thresh/asset/material_flags.hpp"

namespace ferret {

    auto build_mesh_entry(const aiScene* scene, const aiNode* node) -> thresh::asset::MeshEntry {
        thresh::asset::MeshEntry           out;
        std::vector<thresh::asset::MeshVertex> verts;                 // staged, then byte-copied into out.vertices
        std::vector<std::uint32_t>             idx;

        for (unsigned k = 0; k < node->mNumMeshes; ++k) {
            const aiMesh* m = scene->mMeshes[node->mMeshes[k]];
            const auto base_vtx = static_cast<std::uint32_t>(verts.size());
            const auto base_idx = static_cast<std::uint32_t>(idx.size());

            for (unsigned i = 0; i < m->mNumVertices; ++i) {
                thresh::asset::MeshVertex v{};
                v.position = { m->mVertices[i].x, m->mVertices[i].y, m->mVertices[i].z };
                if (m->mNormals)            v.normal = { m->mNormals[i].x, m->mNormals[i].y, m->mNormals[i].z };
                if (m->mTextureCoords[0])   v.uv     = { m->mTextureCoords[0][i].x, m->mTextureCoords[0][i].y };
                if (m->mTangents) {
                    const aiVector3D& t = m->mTangents[i]; const aiVector3D& b = m->mBitangents[i];
                    const aiVector3D& n = m->mNormals[i];
                    const float handed = (n ^ t) * b < 0.f ? -1.f : 1.f;            // aiVector3D::operator^ = cross
                    v.tangent = { t.x, t.y, t.z, handed };
                }
                verts.push_back(v);
                out.aabb_min = component_min(out.aabb_min, v.position);
                out.aabb_max = component_max(out.aabb_max, v.position);
            }
            for (unsigned f = 0; f < m->mNumFaces; ++f) {                          // triangulated → 3 idx/face
                const aiFace& face = m->mFaces[f];
                for (unsigned e = 0; e < 3; ++e) idx.push_back(base_vtx + face.mIndices[e]);
            }
            out.submeshes.push_back({ .index_offset   = base_idx,
                                      .index_count    = static_cast<std::uint32_t>(idx.size()) - base_idx,
                                      .material_index = m->mMaterialIndex });
        }

        out.vertex_count = static_cast<std::uint32_t>(verts.size());
        out.index_count  = static_cast<std::uint32_t>(idx.size());
        out.vertices.resize(verts.size() * sizeof(thresh::asset::MeshVertex));
        std::memcpy(out.vertices.data(), verts.data(), out.vertices.size());
        // index_type: keep U32 here; (yours) optionally re-narrow to U16 when vertex_count <= 0xFFFF
        out.indices.resize(idx.size() * sizeof(std::uint32_t));
        std::memcpy(out.indices.data(), idx.data(), out.indices.size());
        return out;
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

        // Per-slot usage + colour space (the bake pass in Step 16 fills content_hash):
        e.base_colour_texture.usage        = thresh::asset::TextureUsage::BaseColour; e.base_colour_texture.colour_space               = thresh::asset::ColourSpace::sRGB;
        e.normal_texture.usage             = thresh::asset::TextureUsage::Normal; e.normal_texture.colour_space                        = thresh::asset::ColourSpace::Linear;
        e.metallic_roughness_texture.usage = thresh::asset::TextureUsage::MetallicRoughness; e.metallic_roughness_texture.colour_space = thresh::asset::ColourSpace::Linear;
        e.occlusion_texture.usage          = thresh::asset::TextureUsage::Occlusion; e.occlusion_texture.colour_space                  = thresh::asset::ColourSpace::Linear;
        e.emissive_texture.usage           = thresh::asset::TextureUsage::Emissive; e.emissive_texture.colour_space                    = thresh::asset::ColourSpace::sRGB;
        return e;
    }

    auto component_min(std::array<float, 3> a, std::array<float, 3> b) -> std::array<float, 3> {
        return { std::min(a[0], b[0]), std::min(a[1], b[1]), std::min(a[2], b[2]) };
    }

    auto component_max(std::array<float, 3> a, std::array<float, 3> b) -> std::array<float, 3> {
        return { std::max(a[0], b[0]), std::max(a[1], b[1]), std::max(a[2], b[2]) };
    }
} // ferret