//
// Created by Admin on 03/05/2026.
//

#pragma once
#include "thresh/thresh.hpp"

namespace thresh {
    class Scene {

        public:
            Scene();
            ~Scene();

            auto init() -> void;
            auto update(float delta_time) -> void;

            auto get_world() -> flecs::world&;

            auto create_entity(const std::string& name) -> flecs::entity;

            auto create_entity() -> flecs::entity;
        private:
            flecs::world m_world;
    };
} // thresh
