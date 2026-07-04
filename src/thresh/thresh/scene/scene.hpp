//
// Created by Admin on 03/05/2026.
//

#pragma once
#include "flux/graphics_types.hpp"
#include "thresh/thresh.hpp"
#include "thresh/physics/physics_world.hpp"

namespace thresh {

    struct PickHit {
        flecs::entity entity;
        float flags{};
        std::int32_t submesh = -1; // index into MeshRenderer::submeshes; -1 = whole entity (Mesh path)
    };

    class Scene {

        public:
            Scene();
            ~Scene();

            auto init() -> void;
            auto update(float delta_time) -> void;

            auto world() -> flecs::world&;

            auto physics() -> PhysicsWorld&;

            auto get_or_create_entity(const std::string& name) -> flecs::entity;

            auto get_or_create_entity() -> flecs::entity;

            auto compute_active_camera_data(float aspect) -> std::optional<flux::gpu::CameraData>;

            auto compute_active_light_data() -> std::optional<flux::gpu::LightData>;

            auto root() -> flecs::entity;

            auto pick_entity(int mouse_x, int mouse_y, int viewport_w, int viewport_h) -> std::optional<PickHit>;

            flecs::entity m_scene_root;

        private:

            std::unique_ptr<PhysicsWorld> m_physics_world;

            flecs::world m_world;
    };
} // thresh
