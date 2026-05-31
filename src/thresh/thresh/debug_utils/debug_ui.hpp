//
// Created by Admin on 31/05/2026.
//

#pragma once
#include "imgui.h"
#include "ImGuizmo.h"
#include "thresh/scene/scene.hpp"

namespace thresh {
    class DebugUI {

        public:
            explicit DebugUI();

            auto draw(Scene& scene) -> void;

            auto set_selected_entity(flecs::entity entity) -> void {
                m_selected_entity = entity;
            }

            auto selected_entity() const -> flecs::entity {
                return m_selected_entity;
            }

            bool show_aabbs{true};

        private:
            auto draw_tree(const Scene& scene) -> void;
            auto draw_inspector(const Scene& scene) -> void;
            auto draw_gizmos(Scene& scene) -> void;
            auto submit_aabbs(Scene& scene) -> void;

            auto walk_children(flecs::entity parent) -> void;

            flecs::entity m_selected_entity{};

            ImGuizmo::OPERATION m_gizmo_operation{ImGuizmo::TRANSLATE};
            ImGuizmo::MODE m_gizmo_mode{ImGuizmo::WORLD};
    };
} // thresh
