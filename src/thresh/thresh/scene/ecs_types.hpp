//
// Created by Admin on 03/05/2026.
//

#pragma once
#include "helix/math.hpp"
#include "thresh/thresh.hpp"

struct SceneRoot{};

struct Transform {

    helix::float3 position = {};
    helix::quat rotation = {1.f, 0.f, 0.f, 0.f}; //Quat identity
    helix::float3 scale = helix::float3(1.f);
};

struct ActiveCamera {};

struct Camera {

    float fov = helix::radians(90.0f);
    float near_plane = 0.1f;
    float far_plane = 100.0f;
};

struct CameraController {

    float yaw = 0.0f;
    float pitch = 0.0f;

    float movement_speed = 5.0f;
    float mouse_sensitivity = 0.1f;

    bool movement_allowed = true;
};

struct AmbientLight {
    helix::float3 colour = helix::float3(1.f);
    float intensity = 0.1f;
};

struct Light {

    helix::float3 colour = helix::float3(1.f);
    float intensity = 1.f;
};

struct Mesh {
    std::uint32_t handle;
    std::uint32_t material_handle;
    std::uint32_t index_offset = 0;
    std::uint32_t index_count = 0; // 0 = Whole buffer;
};

struct SubmeshDraw {
    std::uint32_t material_handle = 0;
    std::uint32_t index_offset = 0;
    std::uint32_t index_count = 0;  // 0 = Whole buffer
    helix::float4x4 local{1.f};     // cook-time node placement, composed with the entity transform
    helix::AABB local_aabb;         // mesh-local (pre-`local`) bounds — submesh picking/highlight
};

// One cooked model on one entity: a single merged mesh + a draw per submesh.
struct MeshRenderer {
    std::uint32_t mesh_handle = 0;
    std::vector<SubmeshDraw> submeshes;
};

struct MeshSource {
    // Relative VFS path, or primitive uri ( "primitive://box" )
    std::string path;
};

struct MaterialSource {
    // Relative VFS path to a .mat
    std::string path;
};

struct Material {
    std::vector<uint32_t> texture_handles;
};

struct WorldTransform {
    helix::float4x4 transform{1.f};
};

struct WorldAABB {
    helix::AABB aabb;
};

namespace flux::math {

    inline auto compose_local(const Transform& transform) -> helix::float4x4 {
        constexpr auto IDENTITY = helix::float4x4{1.f};
        return helix::translate(IDENTITY, transform.position)
            * helix::mat4_cast(transform.rotation)
            * helix::scale(IDENTITY, transform.scale);
    }
}
