#include "thresh/mesh_loader.hpp"
#include "substratum/log.hpp"

#include "cgltf.h"

#include <glm/glm.hpp>
#include <cstring>
#include <algorithm>

namespace thresh {

    MeshLoader::MeshLoader(flux::Device &device)
        : m_device{device} {
    }

    // ── Helpers ─────────────────────────────────────────────────────────────────

    namespace {
        /**
         * Read a float accessor element at the given index.
         * Supports SCALAR, VEC2, VEC3, VEC4.
         */
        auto read_accessor_float(const cgltf_accessor *accessor, cgltf_size index, float *out, cgltf_size component_count) -> bool {
            if (!accessor || index >= accessor->count) return false;
            return cgltf_accessor_read_float(accessor, index, out, component_count) != 0;
        }

        /**
         * Read a uint index from an accessor.
         */
        auto read_accessor_uint(const cgltf_accessor *accessor, cgltf_size index) -> std::uint32_t {
            if (!accessor || index >= accessor->count) return 0;
            return static_cast<std::uint32_t>(cgltf_accessor_read_index(accessor, index));
        }

        /**
         * Compute tangent vectors using MikkTSpace-lite approach.
         * For simplicity we compute per-triangle tangents and average.
         * If tangent data is already in the glTF, this is not called.
         */
        auto compute_tangents(std::vector<flux::Vertex> &vertices, const std::vector<std::uint32_t> &indices) -> void {
            // Initialize tangent accumulators
            std::vector<glm::vec3> tangent_accum(vertices.size(), glm::vec3(0.0f));
            std::vector<glm::vec3> bitangent_accum(vertices.size(), glm::vec3(0.0f));

            for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
                auto i0 = indices[i];
                auto i1 = indices[i + 1];
                auto i2 = indices[i + 2];

                const auto &v0 = vertices[i0];
                const auto &v1 = vertices[i1];
                const auto &v2 = vertices[i2];

                auto edge1 = v1.position - v0.position;
                auto edge2 = v2.position - v0.position;
                auto duv1 = v1.uv - v0.uv;
                auto duv2 = v2.uv - v0.uv;

                float denom = duv1.x * duv2.y - duv2.x * duv1.y;
                float f = (std::abs(denom) > 1e-6f) ? (1.0f / denom) : 0.0f;

                glm::vec3 tangent = f * (duv2.y * edge1 - duv1.y * edge2);
                glm::vec3 bitangent = f * (-duv2.x * edge1 + duv1.x * edge2);

                tangent_accum[i0] += tangent;
                tangent_accum[i1] += tangent;
                tangent_accum[i2] += tangent;

                bitangent_accum[i0] += bitangent;
                bitangent_accum[i1] += bitangent;
                bitangent_accum[i2] += bitangent;
            }

            // Orthogonalize and store w handedness
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                auto &vert = vertices[i];
                auto n = vert.normal;
                auto t = tangent_accum[i];
                auto b = bitangent_accum[i];

                // Gram-Schmidt orthogonalize
                auto t_ortho = t - n * glm::dot(n, t);
                float len = glm::length(t_ortho);
                if (len > 1e-6f) {
                    t_ortho /= len;
                } else {
                    t_ortho = glm::vec3(1.0f, 0.0f, 0.0f);
                }

                // Handedness
                float w = (glm::dot(glm::cross(n, t_ortho), b) < 0.0f) ? -1.0f : 1.0f;
                vert.tangent = glm::vec4(t_ortho, w);
            }
        }

        /**
         * Extract directory from a virtual path.
         * "/models/helmet.glb" -> "/models/"
         */
        auto get_directory(const std::string &path) -> std::string {
            auto last_slash = path.find_last_of('/');
            if (last_slash == std::string::npos) return "";
            return path.substr(0, last_slash + 1);
        }
    } // anonymous namespace

    // ── Main loader ─────────────────────────────────────────────────────────────

    auto MeshLoader::load_gltf(
        const std::vector<std::uint8_t> &data,
        const std::string &virtual_path
    ) -> MeshLoadResult {
        MeshLoadResult result;

        cgltf_options options{};
        cgltf_data *gltf_data = nullptr;

        // Parse from memory
        auto parse_result = cgltf_parse(&options, data.data(), data.size(), &gltf_data);
        if (parse_result != cgltf_result_success) {
            SUB_ERROR("Failed to parse glTF '{}': error code {}", virtual_path, static_cast<int>(parse_result));
            return result;
        }

        // Load buffers from the embedded data (glb) or external URIs
        // For glb files, all data is already in the binary blob.
        // For separate .gltf + .bin, we'd need VFS reads — for now, cgltf_load_buffers handles glb.
        auto buf_result = cgltf_load_buffers(&options, gltf_data, nullptr);
        if (buf_result != cgltf_result_success) {
            SUB_WARN("Failed to load glTF buffers for '{}' (may need VFS for external buffers)", virtual_path);
            // Don't fail — some data may still be readable
        }

        // Validate
        auto validate_result = cgltf_validate(gltf_data);
        if (validate_result != cgltf_result_success) {
            SUB_WARN("glTF validation failed for '{}': code {}", virtual_path, static_cast<int>(validate_result));
        }

        auto dir = get_directory(virtual_path);

        // ── Extract materials ───────────────────────────────────────────────

        for (cgltf_size mat_idx = 0; mat_idx < gltf_data->materials_count; ++mat_idx) {
            const auto &mat = gltf_data->materials[mat_idx];
            MaterialDesc desc;
            desc.name = mat.name ? mat.name : "";

            if (mat.has_pbr_metallic_roughness) {
                const auto &pbr = mat.pbr_metallic_roughness;

                std::memcpy(desc.base_color_factor, pbr.base_color_factor, sizeof(float) * 4);
                desc.metallic_factor = pbr.metallic_factor;
                desc.roughness_factor = pbr.roughness_factor;

                if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image) {
                    auto *uri = pbr.base_color_texture.texture->image->uri;
                    if (uri) desc.albedo_path = dir + uri;
                }

                if (pbr.metallic_roughness_texture.texture && pbr.metallic_roughness_texture.texture->image) {
                    auto *uri = pbr.metallic_roughness_texture.texture->image->uri;
                    if (uri) desc.metallic_roughness_path = dir + uri;
                }
            }

            if (mat.normal_texture.texture && mat.normal_texture.texture->image) {
                auto *uri = mat.normal_texture.texture->image->uri;
                if (uri) desc.normal_path = dir + uri;
            }

            if (mat.emissive_texture.texture && mat.emissive_texture.texture->image) {
                auto *uri = mat.emissive_texture.texture->image->uri;
                if (uri) desc.emissive_path = dir + uri;
            }

            std::memcpy(desc.emissive_factor, mat.emissive_factor, sizeof(float) * 3);

            result.materials.push_back(std::move(desc));
        }

        // If no materials, add a default
        if (result.materials.empty()) {
            MaterialDesc default_mat;
            default_mat.name = "default";
            result.materials.push_back(std::move(default_mat));
        }

        // ── Extract meshes ──────────────────────────────────────────────────

        for (cgltf_size mesh_idx = 0; mesh_idx < gltf_data->meshes_count; ++mesh_idx) {
            const auto &gltf_mesh = gltf_data->meshes[mesh_idx];

            std::vector<flux::Vertex> all_vertices;
            std::vector<std::uint32_t> all_indices;
            std::vector<flux::SubMesh> submeshes;
            bool has_tangents = false;

            for (cgltf_size prim_idx = 0; prim_idx < gltf_mesh.primitives_count; ++prim_idx) {
                const auto &prim = gltf_mesh.primitives[prim_idx];

                // Only handle triangles
                if (prim.type != cgltf_primitive_type_triangles) {
                    SUB_WARN("Skipping non-triangle primitive in mesh '{}'", gltf_mesh.name ? gltf_mesh.name : "unnamed");
                    continue;
                }

                // Find attribute accessors
                const cgltf_accessor *pos_accessor = nullptr;
                const cgltf_accessor *normal_accessor = nullptr;
                const cgltf_accessor *uv_accessor = nullptr;
                const cgltf_accessor *tangent_accessor = nullptr;

                for (cgltf_size attr_idx = 0; attr_idx < prim.attributes_count; ++attr_idx) {
                    const auto &attr = prim.attributes[attr_idx];
                    switch (attr.type) {
                        case cgltf_attribute_type_position:
                            pos_accessor = attr.data;
                            break;
                        case cgltf_attribute_type_normal:
                            normal_accessor = attr.data;
                            break;
                        case cgltf_attribute_type_texcoord:
                            if (attr.index == 0) uv_accessor = attr.data;
                            break;
                        case cgltf_attribute_type_tangent:
                            tangent_accessor = attr.data;
                            has_tangents = true;
                            break;
                        default:
                            break;
                    }
                }

                if (!pos_accessor) {
                    SUB_WARN("Primitive missing POSITION attribute in mesh '{}'", gltf_mesh.name ? gltf_mesh.name : "unnamed");
                    continue;
                }

                auto base_vertex = static_cast<std::uint32_t>(all_vertices.size());
                auto base_index = static_cast<std::uint32_t>(all_indices.size());

                // Read vertices
                for (cgltf_size v = 0; v < pos_accessor->count; ++v) {
                    flux::Vertex vert{};

                    float pos[3] = {0.0f, 0.0f, 0.0f};
                    read_accessor_float(pos_accessor, v, pos, 3);
                    vert.position = {pos[0], pos[1], pos[2]};

                    if (normal_accessor) {
                        float norm[3] = {0.0f, 1.0f, 0.0f};
                        read_accessor_float(normal_accessor, v, norm, 3);
                        vert.normal = {norm[0], norm[1], norm[2]};
                    }

                    if (uv_accessor) {
                        float tex[2] = {0.0f, 0.0f};
                        read_accessor_float(uv_accessor, v, tex, 2);
                        vert.uv = {tex[0], tex[1]};
                    }

                    if (tangent_accessor) {
                        float tang[4] = {1.0f, 0.0f, 0.0f, 1.0f};
                        read_accessor_float(tangent_accessor, v, tang, 4);
                        vert.tangent = {tang[0], tang[1], tang[2], tang[3]};
                    }

                    all_vertices.push_back(vert);
                }

                // Read indices
                std::uint32_t index_count = 0;
                if (prim.indices) {
                    index_count = static_cast<std::uint32_t>(prim.indices->count);
                    for (cgltf_size i = 0; i < prim.indices->count; ++i) {
                        all_indices.push_back(base_vertex + read_accessor_uint(prim.indices, i));
                    }
                } else {
                    // Non-indexed geometry — generate sequential indices
                    index_count = static_cast<std::uint32_t>(pos_accessor->count);
                    for (std::uint32_t i = 0; i < index_count; ++i) {
                        all_indices.push_back(base_vertex + i);
                    }
                }

                // Determine material index
                std::uint32_t material_index = 0;
                if (prim.material) {
                    // Find the index by pointer offset
                    material_index = static_cast<std::uint32_t>(prim.material - gltf_data->materials);
                }

                flux::SubMesh sub{};
                sub.index_offset = base_index;
                sub.index_count = index_count;
                sub.material_index = material_index;
                submeshes.push_back(sub);
            }

            if (all_vertices.empty() || all_indices.empty()) {
                SUB_WARN("Skipping empty mesh '{}' in '{}'", gltf_mesh.name ? gltf_mesh.name : "unnamed", virtual_path);
                continue;
            }

            // Compute tangents if not provided by the glTF
            if (!has_tangents) {
                compute_tangents(all_vertices, all_indices);
            }

            // Upload to GPU
            auto mesh = flux::Mesh::create(
                m_device,
                all_vertices,
                all_indices,
                std::move(submeshes)
            );

            if (mesh) {
                SUB_DEBUG("Loaded mesh '{}': {} vertices, {} indices",
                          gltf_mesh.name ? gltf_mesh.name : "unnamed",
                          all_vertices.size(), all_indices.size());
                result.meshes.push_back(std::move(mesh));
            }
        }

        cgltf_free(gltf_data);

        SUB_INFO("Loaded glTF '{}': {} meshes, {} materials",
                 virtual_path, result.meshes.size(), result.materials.size());

        return result;
    }

} // namespace thresh
