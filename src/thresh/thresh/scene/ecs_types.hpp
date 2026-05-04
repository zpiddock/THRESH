//
// Created by Admin on 03/05/2026.
//

#pragma once
#include "thresh/thresh.hpp"

struct Transform {

    glm::vec3 position = {};
    glm::quat rotation = {};
    glm::vec3 scale = {};
};

struct Camera {

    float fov = 90.0f;
    float near_plane = 0.1f;
    float far_plane = 100.0f;
};

struct Light {

    glm::vec3 colour = glm::vec3(1.f);
    float intensity = 1.f;
};

struct Mesh {
    std::uint32_t handle;
    std::uint32_t material_handle;
};

struct MeshPath {
    std::string path;
};

struct Material {
    std::vector<uint32_t> texture_handles;
};
