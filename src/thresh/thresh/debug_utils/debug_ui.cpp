//
// Created by Admin on 31/05/2026.
//

#include "debug_ui.hpp"

#include "imgui.h"
#include "ImGuizmo.h"
#include "thresh/engine.hpp"
#include "thresh/scene/ecs_types.hpp"

namespace {

    auto write_to_local(flecs::entity entity, const flux::float4x4& new_world) -> void {

        flux::float4x4 parent_world{1.f};
        if (auto parent = entity.parent(); parent.is_valid()) {
            if (const auto* t = parent.try_get<WorldTransform>()) {
                parent_world = t->transform;
            }
        }

        const flux::float4x4 local = flux::math::inverse(parent_world) * new_world;

        flux::float3 scale, skew, translation;
        flux::float4 perspective;
        flux::quat rotation;

        flux::math::decompose(local, scale, rotation, translation, skew, perspective);

        entity.set<Transform>({.position = translation, .rotation = rotation, .scale = scale});
    }

    auto draw_transforms(flecs::entity entity) -> void {
        if (!entity.has<Transform>()) {
            return;
        }
        if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }

        auto& transform = entity.get_mut<Transform>();
        ImGui::DragFloat3("Position", flux::math::value_ptr(transform.position), 0.05f);
        ImGui::DragFloat4("Rotation", flux::math::value_ptr(transform.rotation), 0.01f); // Raw Quat
        ImGui::DragFloat3("Scale", flux::math::value_ptr(transform.scale), 0.05f);
    }

    auto draw_camera(flecs::entity entity) -> void {
        if (!entity.has<Camera>()) {
            return;
        }
        if (!ImGui::CollapsingHeader("Camera")) {
            return;
        }
        auto& camera = entity.get_mut<Camera>();
        float fov_degrees = flux::math::degrees(camera.fov);
        if (ImGui::DragFloat("FOV (Degrees)", &fov_degrees, 0.1f, 1.f, 179.f)) {
            camera.fov = flux::math::radians(fov_degrees);
        }
        ImGui::DragFloat("Near Plane", &camera.near_plane, 0.1f, 0.1f, 100.f);
        ImGui::DragFloat("Far Plane", &camera.far_plane, 0.1f, 0.1f, 10000.f);
    }

    auto draw_light(flecs::entity entity) -> void {
        if (!entity.has<Light>()) {
            return;
        }
        if (!ImGui::CollapsingHeader("Light")) {
            return;
        }
        auto& light = entity.get_mut<Light>();
        ImGui::ColorEdit3("Colour", glm::value_ptr(light.colour));
        ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.f, 1000.f);
    }

    auto draw_mesh(flecs::entity entity) -> void {
        if (!entity.has<Mesh>()) {
            return;
        }
        if (!ImGui::CollapsingHeader("Mesh")) {
            return;
        }
        auto& mesh = entity.get<Mesh>();
        ImGui::Text("handle %u, material %u", mesh.handle, mesh.material_handle);
    }

    auto draw_sources(flecs::entity e) -> void {
        if (e.has<MeshSource>())     ImGui::Text("MeshSource: %s",     e.get<MeshSource>().path.c_str());
        if (e.has<MaterialSource>()) ImGui::Text("MaterialSource: %s", e.get<MaterialSource>().path.c_str());
    }

    auto draw_world(flecs::entity e) -> void {
        if (e.has<WorldAABB>()) {
            const auto& a = e.get<WorldAABB>().aabb;
            ImGui::Text("WorldAABB min (%.2f %.2f %.2f) max (%.2f %.2f %.2f)",
                        a.min.x, a.min.y, a.min.z, a.max.x, a.max.y, a.max.z);
        }
    }
}

namespace thresh {
    DebugUI::DebugUI() {

        Engine::get_instance().graphics()->enable_debug_line_renderer();
    }

    auto DebugUI::draw(Scene& scene) -> void {

        ImGuizmo::BeginFrame();

        if (!ImGui::GetIO().WantTextInput) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) {
                m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E)) {
                m_gizmo_operation = ImGuizmo::OPERATION::ROTATE;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                m_gizmo_operation = ImGuizmo::OPERATION::SCALE;
            }
        }

        draw_tree(scene);
        draw_inspector(scene);
        draw_gizmos(scene);
        if (show_aabbs) {
            submit_aabbs(scene);
        }
    }

    auto DebugUI::draw_tree(const Scene& scene) -> void {

        ImGui::Begin("Scene Tree");
        ImGui::Checkbox("Show AABBs", &show_aabbs);
        ImGui::Separator();
        walk_children(scene.m_scene_root);
        ImGui::End();
    }

    auto DebugUI::draw_inspector(const Scene& scene) -> void {

        ImGui::Begin("Inspector");
        if (m_selected_entity.is_alive()) {
            ImGui::Text("%s  (id %llu)", m_selected_entity.name().c_str(), static_cast<unsigned long long>(m_selected_entity.id()));
            ImGui::Separator();
            draw_transforms(m_selected_entity);
            draw_camera(m_selected_entity);
            draw_light(m_selected_entity);
            draw_mesh(m_selected_entity);
            draw_sources(m_selected_entity);
            draw_world(m_selected_entity);
        } else {
            ImGui::Text("Nothing selected");
        }
        ImGui::End();
    }

    auto DebugUI::draw_gizmos(Scene& scene) -> void {

        if (!m_selected_entity.is_alive() || !m_selected_entity.has<WorldTransform>()) {
            return;
        }
        const ImGuiIO& io = ImGui::GetIO();
        if (io.DisplaySize.x == 0.f || io.DisplaySize.y == 0.f) {
            return;
        }
        const float aspect = io.DisplaySize.x / io.DisplaySize.y;

        auto cam = scene.compute_active_camera_data(aspect);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        flux::float4x4 world = m_selected_entity.get<WorldTransform>().transform;

        ImGuizmo::Manipulate(
            flux::math::value_ptr(cam->cam_view_model),
            flux::math::value_ptr(cam->cam_proj_matrix),
            m_gizmo_operation,
            m_gizmo_mode,
            flux::math::value_ptr(world));

        write_to_local(m_selected_entity, world);
    }

    auto DebugUI::submit_aabbs(Scene& scene) -> void {

        auto* lines = Engine::get_instance().graphics()->debug_line_renderer();
        if (!lines) return;
        scene.get_world().query_builder<const WorldAABB>().with<Mesh>().build()
            .each([&](flecs::entity e, const WorldAABB& w) {
                const auto colour = (e == m_selected_entity) ? flux::float3{1, 1, 0}
                                                 : flux::float3{0.5f, 0.5f, 0.5f};
                lines->submit_aabb(w.aabb, colour);
            });
    }

    auto DebugUI::walk_children(flecs::entity parent) -> void {

        parent.children([&] (flecs::entity child) {
            const auto* name = child.name().c_str();
            const bool has_children = child.world().count(flecs::ChildOf, child) > 0;

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow
                | (m_selected_entity == child ? ImGuiTreeNodeFlags_Selected : 0)
                | (has_children ? 0 : ImGuiTreeNodeFlags_Leaf);

            const bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(child.id()), flags, "%s", name);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_selected_entity = child;
            }
            if (opened) {
                walk_children(child);
                ImGui::TreePop();
            }
        });
    }
} // thresh