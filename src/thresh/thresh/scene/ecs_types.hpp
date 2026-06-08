//
// Created by Admin on 03/05/2026.
//

#pragma once
#include "flux/math.hpp"
#include "thresh/thresh.hpp"

struct SceneRoot{};

struct Transform {

    flux::float3 position = {};
    flux::quat rotation = {};
    flux::float3 scale = flux::float3(1.f);
};

struct ActiveCamera {};

struct Camera {

    float fov = flux::math::radians(90.0f);
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
    flux::float3 colour = flux::float3(1.f);
    float intensity = 0.1f;
};

struct Light {

    flux::float3 colour = flux::float3(1.f);
    float intensity = 1.f;
};

struct Mesh {
    std::uint32_t handle;
    std::uint32_t material_handle;
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
    flux::float4x4 transform{1.f};
};

struct WorldAABB {
    flux::AABB aabb;
};

namespace flux::math {

    inline auto compose_local(const Transform& transform) -> flux::float4x4 {
        constexpr auto IDENTITY = flux::float4x4{1.f};
        return flux::math::translate(IDENTITY, transform.position)
            * flux::math::mat4_cast(transform.rotation)
            * flux::math::scale(IDENTITY, transform.scale);
    }
}
