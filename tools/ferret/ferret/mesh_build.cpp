//
// Created by Admin on 28/06/2026.
//

#include "mesh_build.hpp"

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

    auto component_min(std::array<float, 3> a, std::array<float, 3> b) -> std::array<float, 3> {
        return { std::min(a[0], b[0]), std::min(a[1], b[1]), std::min(a[2], b[2]) };
    }

    auto component_max(std::array<float, 3> a, std::array<float, 3> b) -> std::array<float, 3> {
        return { std::max(a[0], b[0]), std::max(a[1], b[1]), std::max(a[2], b[2]) };
    }
} // ferret