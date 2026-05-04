//
// Created by Admin on 03/05/2026.
//

#include "scene.hpp"

namespace thresh {
    Scene::Scene() {
    }

    Scene::~Scene() {
    }

    auto Scene::init() -> void {
    }

    auto Scene::update(float delta_time) -> void {

    }

    auto Scene::get_world() -> flecs::world& {
        return m_world;
    }

    auto Scene::create_entity(const std::string& name) -> flecs::entity {
        return m_world.entity(name.c_str());
    }

    auto Scene::create_entity() -> flecs::entity {
        return m_world.entity();
    }
} // thresh