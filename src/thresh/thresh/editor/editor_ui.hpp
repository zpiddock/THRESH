//
// Created by Admin on 05/07/2026.
//

#pragma once
#include "imgui.h"
#include "ImGuizmo.h"
#include "thresh/scene/scene.hpp"

namespace thresh::edit {
    class EditorUI {

        public:
            auto draw(Scene& scene) -> void;

            auto set_selected_entity(flecs::entity entity, std::int32_t submesh = -1) -> void;

            [[nodiscard]] auto selected_entity() const -> flecs::entity;
            [[nodiscard]] auto selected_submesh() const -> std::int32_t;

            bool m_show_aabb{true};

        private:

            auto draw_tree(Scene& scene) -> void;
            auto draw_inspector(Scene& scene) -> void;
            auto draw_gizmos(Scene& scene) -> void;
            auto submit_aabbs(Scene& scene) -> void;

            auto walk_children(flecs::entity parent) -> void;
            auto draw_tree_context_menu(flecs::entity entity) -> void;
            auto begin_rename(flecs::entity entity) -> void;
            auto draw_rename_field(flecs::entity) -> void;

            auto draw_name_field(flecs::entity entity) -> void;
            auto draw_component(flecs::entity entity, flecs::entity component) -> void;
            auto draw_add_component_button(flecs::entity entity) -> void;

            auto not_implemented(const char* what) -> void;
            auto draw_not_implemented() -> void;

            flecs::entity   m_selected_entity {};
            std::int32_t    m_selected_submesh{-1};
            flecs::world_t* m_last_world      {nullptr};

            flecs::entity   m_renaming_entity     {};
            [[maybe_unused]]char            m_renaming_buffer[128]{};

            std::function<void()> m_deferred{};

            const char* m_not_implemented{nullptr};

            [[maybe_unused]]ImGuizmo::OPERATION m_gizmo_operation{ImGuizmo::TRANSLATE};
            [[maybe_unused]]ImGuizmo::MODE      m_gizmo_mode     {ImGuizmo::WORLD};
    };
}
