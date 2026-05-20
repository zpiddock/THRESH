//
// Created by Admin on 03/05/2026.
//

#pragma once
#include "flux/graphics_types.hpp"
#include "thresh/thresh.hpp"

namespace thresh {
    class Scene {

        public:
            Scene();
            ~Scene();

            auto init() -> void;
            auto update(float delta_time) -> void;

            auto get_world() -> flecs::world&;

            auto get_or_create_entity(const std::string& name) -> flecs::entity;

            auto get_or_create_entity() -> flecs::entity;

            auto compute_active_camera_data(float aspect) -> std::optional<flux::CameraData>;

            auto root() -> flecs::entity;


        private:
            flecs::world m_world;

            flecs::entity m_scene_root;
    };
} // thresh
