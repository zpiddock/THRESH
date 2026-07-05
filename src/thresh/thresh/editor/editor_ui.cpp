//
// Created by Admin on 05/07/2026.
//

#include "editor_ui.hpp"

#include "imgui.h"
#include "ImGuizmo.h"
#include "scene_commands.hpp"
#include "helix/colour.hpp"
#include "thresh/engine.hpp"

namespace {

    auto write_to_local(flecs::entity entity, const helix::float4x4& new_world) -> void {

        helix::float4x4 parent_world{1.f};
        if (auto parent = entity.parent(); parent.is_valid()) {
            if (const auto* t = parent.try_get<WorldTransform>()) {
                parent_world = t->transform;
            }
        }

        const helix::float4x4 local = helix::inverse(parent_world) * new_world;

        helix::float3 scale, skew, translation;
        helix::float4 perspective;
        helix::quat rotation;

        if (helix::decompose(local, scale, rotation, translation, skew, perspective)) {

            entity.set<Transform>({.position = translation, .rotation = rotation, .scale = scale});
        }
    }

    auto draw_string(const char* label, std::string* value) -> bool {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "%s", value->c_str());
        if ( ImGui::InputText(label, buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            *value = buffer;
            return true;
        }
        return false;
    }

    // Euler editing but stored in a quat
    // eulerAngles() is unstable near 90degrees, so angles aer latched while widget is active
    auto draw_rotation(const char* label, helix::quat* value) -> bool {

        static const void* s_active = nullptr;
        static helix::float3 s_euler{};

        helix::float3 euler = (s_active == value) ? s_euler : helix::degrees(helix::eulerAngles(*value));

        const bool changed = ImGui::DragFloat3(label, helix::value_ptr(euler), 0.5f);

        if (ImGui::IsItemActive()) {
            s_active = value;
            s_euler = euler;
        } else if (s_active == value) {
            s_active = nullptr;
        }

        if (changed) {
            *value = helix::quat(helix::radians(euler));
        }
        return changed;
    }

    auto draw_enum(const char* label, flecs::entity type, void* member_ptr) -> bool {

        auto* value = static_cast<std::int32_t*>(member_ptr);

        const char* preview = "?";
        std::vector<std::pair<const char*, std::int32_t>> constants;
        type.children([&] (flecs::entity constant) {
            const auto* v = static_cast<const std::int32_t*>(constant.get(flecs::Constant, flecs::I32));
            if (!v) {
                return;
            }
            constants.emplace_back(constant.name().c_str(), *v);
            if (*v == *value) {
                preview = constant.name().c_str();
            }
        });

        bool changed = false;
        if (ImGui::BeginCombo(label, preview)) {
            for (const auto& [name, const_value] : constants) {
                if (ImGui::Selectable(name, const_value == *value)) {
                    *value = const_value;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    auto draw_member(flecs::world world, const ecs_member_t& member, void* base) -> bool {
        void* ptr = static_cast<char*>(base) + member.offset;
        const flecs::entity type{world, member.type};

        if (member.type == world.id<helix::float3>()) {
            auto* v = static_cast<helix::float3*>(ptr);
            if (std::string_view{member.name}.contains("colour")) {
                return ImGui::ColorEdit3(member.name, helix::value_ptr(*v));
            }
            return ImGui::DragFloat3(member.name, helix::value_ptr(*v), 0.05f);
        }
        if (member.type == world.id<helix::Colour>()) {
            auto* v = static_cast<helix::Colour*>(ptr);
            return ImGui::ColorEdit4(member.name, v->data());
        }
        if (member.type == world.id<helix::quat>()) {
            return draw_rotation(member.name, static_cast<helix::quat*>(ptr));
        }
        if (type.has<flecs::Enum>()) {
            return draw_enum(member.name, type, ptr);
        }
        if (member.type == flecs::F32) {
            return ImGui::DragFloat(member.name, static_cast<float*>(ptr), 0.05f);
        }
        if (member.type == flecs::Bool) {
            return ImGui::Checkbox(member.name, static_cast<bool*>(ptr));
        }
        if (member.type == flecs::I32) {
            return ImGui::DragInt(member.name, static_cast<int*>(ptr));
        }
        if (member.type == flecs::U32) {
            return ImGui::DragScalar(member.name, ImGuiDataType_U32, ptr);
        }
        if (member.type == world.id<std::string>()) {
            return draw_string(member.name, static_cast<std::string*>(ptr));
        }

        ImGui::TextDisabled("%s (unsupported: %s)", member.name, type.name().c_str());
        return false;
    }
}

namespace thresh::edit {
    auto EditorUI::draw(Scene& scene) -> void {
        if (auto* world = scene.world().c_ptr(); world != m_last_world) {
            m_selected_entity  = {};
            m_selected_submesh = -1;
            m_last_world       = world;
        }

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
        if (m_show_aabb) {
            submit_aabbs(scene);
        }
        draw_not_implemented();

        if (m_deferred) {
            auto action = std::move(m_deferred);
            m_deferred = {};
            action();
        }
    }

    auto EditorUI::set_selected_entity(flecs::entity entity, std::int32_t submesh) -> void {

        if (entity == m_selected_entity && submesh == m_selected_submesh) {
            return;
        }
        m_selected_entity  = entity;
        m_selected_submesh = submesh;
    }

    auto EditorUI::selected_entity() const -> flecs::entity {
        return m_selected_entity;
    }

    auto EditorUI::selected_submesh() const -> std::int32_t {
        return m_selected_submesh;
    }

    auto EditorUI::draw_tree(Scene& scene) -> void {
        ImGui::Begin("Scene Tree");

        if (ImGui::Button("+ Add")) ImGui::OpenPopup("tree_add");
        if (ImGui::BeginPopup("tree_add")) {
            auto parent = m_selected_entity.is_alive() ? m_selected_entity : scene.root();
            if (ImGui::MenuItem("Empty")) {                              // phase 1
                m_deferred = [this, &scene, parent] {
                    begin_rename(edit::create_empty(scene, parent));
                };
            }
            if (ImGui::MenuItem("Block")) {                              // phase 1
                m_deferred = [this, &scene, parent] {
                    begin_rename(edit::create_block(
                        scene, Engine::get_instance().assets(), parent));
                };
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Show AABBs", &m_show_aabb);
        ImGui::Separator();

        walk_children(scene.root());

        // Hotkeys — same WantTextInput guard as the gizmo keys.
        if (!ImGui::GetIO().WantTextInput && m_selected_entity.is_alive()) {
            if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
                begin_rename(m_selected_entity);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                m_deferred = [e = m_selected_entity] { edit::destroy_entity(e); };
            }
        }

        ImGui::End();
    }

    auto EditorUI::draw_inspector(Scene& scene) -> void {
        ImGui::Begin("Inspector");
        if (m_selected_entity.is_alive()) {
            draw_name_field(m_selected_entity);
            ImGui::Separator();

            m_selected_entity.each([&](flecs::id id) {
                if (!id.is_entity()) return;                 // skip pairs/flags
                flecs::entity comp = id.entity();
                if (!comp.has<flecs::Struct>()) return;      // skip tags/runtime
                draw_component(m_selected_entity, comp);
            });

            ImGui::Separator();
            draw_add_component_button(m_selected_entity);    // phase 5
        } else {
            ImGui::Text("Nothing selected");
        }
        ImGui::End();

        ImGui::Begin("Scene Settings");
        // draw_component(scene.root(), scene.world().component<AmbientLight>());
        ImGui::End();
    }

    auto EditorUI::draw_gizmos(Scene& scene) -> void {

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

        helix::float4x4 world = m_selected_entity.get<WorldTransform>().transform;

        if (ImGuizmo::Manipulate(
            helix::value_ptr(cam->view),
            helix::value_ptr(cam->projection),
            m_gizmo_operation,
            m_gizmo_mode,
            helix::value_ptr(world))) {

            write_to_local(m_selected_entity, world);
            }
    }

    auto EditorUI::submit_aabbs(Scene& scene) -> void {

        auto* lines = Engine::get_instance().graphics()->debug_line_renderer();
        if (!lines) return;
        scene.world().query_builder<const WorldAABB>().with<Mesh>().or_().with<MeshRenderer>().build()
            .each([&](flecs::entity e, const WorldAABB& w) {
                const auto colour = (e == m_selected_entity) ? helix::float3{1, 1, 0}
                                                 : helix::float3{0.5f, 0.5f, 0.5f};
                lines->submit_aabb(w.aabb, colour);
            });

        // Per-submesh boxes for the selected model — the picked/inspector-selected one pops orange.
        if (m_selected_entity.is_alive() && m_selected_entity.has<MeshRenderer>()
            && m_selected_entity.has<WorldTransform>()) {
            const auto& renderer = m_selected_entity.get<MeshRenderer>();
            const auto& world    = m_selected_entity.get<WorldTransform>().transform;
            for (std::size_t i = 0; i < renderer.submeshes.size(); ++i) {
                const auto& sm  = renderer.submeshes[i];
                const auto  box = helix::transform_aabb(sm.local_aabb, world * sm.local);
                const auto colour = (static_cast<std::int32_t>(i) == m_selected_submesh)
                                  ? helix::float3{1.f, 0.5f, 0.f}
                : helix::float3{0.3f, 0.6f, 1.f};
                lines->submit_aabb(box, colour);
            }
            }
    }

    auto EditorUI::walk_children(flecs::entity parent) -> void {
        parent.children([&](flecs::entity child) {
            ImGui::PushID(static_cast<int>(child.id()));

            if (child == m_renaming_entity) {
                draw_rename_field(child);
                ImGui::PopID();
                return;
            }

            const bool has_children = child.world().count(flecs::ChildOf, child) > 0;
            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow
                | (m_selected_entity == child ? ImGuiTreeNodeFlags_Selected : 0)
                | (has_children ? 0 : ImGuiTreeNodeFlags_Leaf);

            const bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(child.id()),
                                                  flags, "%s", child.name().c_str());
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_selected_entity  = child;
                m_selected_submesh = -1;
            }

            draw_tree_context_menu(child);

            if (opened) {
                walk_children(child);
                ImGui::TreePop();
            }
            ImGui::PopID();
        });
    }

    auto EditorUI::draw_tree_context_menu(flecs::entity entity) -> void {
        if (!ImGui::BeginPopupContextItem()) {
            return;
        }
        if (ImGui::BeginMenu("Add Child")) {
            if (ImGui::MenuItem("Empty")) {
                m_deferred = [this, entity] {
                    auto* scene = Engine::get_instance().active_scene();
                    begin_rename(edit::create_empty(*scene, entity));
                };
            }
            if (ImGui::MenuItem("Block")) {
                m_deferred = [this, entity] {
                    auto& engine = Engine::get_instance();
                    begin_rename(edit::create_block(
                        *engine.active_scene(), engine.assets(), entity));
                };
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Rename", "F2")) {                           // phase 2
            begin_rename(entity);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete", "Del")) {                          // phase 3
            m_deferred = [entity] { edit::destroy_entity(entity); };
        }
        ImGui::EndPopup();
    }

    auto EditorUI::begin_rename(flecs::entity entity) -> void {
        if (!entity.is_valid()) return;     // create_* stubs return {} until phase 1
        m_renaming_entity  = entity;
        m_selected_entity  = entity;
        m_selected_submesh = -1;
        std::snprintf(m_renaming_buffer, sizeof(m_renaming_buffer), "%s", entity.name().c_str());
    }

    auto EditorUI::draw_rename_field(flecs::entity entity) -> void {
        ImGui::SetKeyboardFocusHere();
        const bool committed = ImGui::InputText("##rename", m_renaming_buffer,
                                                sizeof(m_renaming_buffer),
                                                ImGuiInputTextFlags_EnterReturnsTrue
                                                | ImGuiInputTextFlags_AutoSelectAll);
        const bool cancelled = ImGui::IsKeyPressed(ImGuiKey_Escape);
        const bool clicked_away = ImGui::IsItemDeactivated() && !committed && !cancelled;

        if (committed || clicked_away) {
            // Rejected renames (sibling collision / empty) just keep the old name.
            edit::rename(entity, m_renaming_buffer);
        }
        if (committed || cancelled || clicked_away) {
            m_renaming_entity = {};
        }
    }

    auto EditorUI::draw_name_field(flecs::entity entity) -> void {

    }

    auto EditorUI::draw_component(flecs::entity entity, flecs::entity component) -> void {
        const auto* meta = component.try_get<flecs::Struct>();
        void* base = entity.get_mut(component);
        if (!meta || !base) {
            return;
        }

        const bool open = ImGui::CollapsingHeader(component.name().c_str(),
                                                  ImGuiTreeNodeFlags_DefaultOpen);
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Remove component")) {           // phase 5
                edit::remove_component(entity, component);
                ImGui::EndPopup();
                return;
            }
            ImGui::EndPopup();
        }
        if (!open) {
            return;
        }

        ImGui::PushID(static_cast<int>(component.id()));
        bool edited = false;
        const auto* members = ecs_vec_first_t(&meta->members, ecs_member_t);
        for (std::int32_t i = 0; i < ecs_vec_count(&meta->members); ++i) {
            edited |= draw_member(entity.world(), members[i], base);
        }
        ImGui::PopID();

        if (edited) {
            entity.modified(component);
            edit::mark_physics_dirty(entity, component);                   // phase 6
        }
    }

    auto EditorUI::draw_add_component_button(flecs::entity entity) -> void {
        if (ImGui::Button("+ Add Component", {-FLT_MIN, 0.f})) {
            ImGui::OpenPopup("add_component");
        }
        if (ImGui::BeginPopup("add_component")) {
            auto world = entity.world();
            for (auto comp : edit::addable_components(world)) {
                if (entity.has(comp)) continue;
                if (ImGui::MenuItem(comp.name().c_str())) {
                    edit::add_component(entity, comp);
                }
            }
            ImGui::EndPopup();
        }
    }

    auto EditorUI::not_implemented(const char* what) -> void {

        m_not_implemented = what;
    }

    auto EditorUI::draw_not_implemented() -> void {

        if (m_not_implemented && !ImGui::IsPopupOpen("Not Implemented")) {
            ImGui::OpenPopup("Not Implemented");
        }
        if (ImGui::BeginPopupModal("Not Implemented", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", m_not_implemented);
            if (ImGui::Button("OK", {120.f, 0.f})) {
                m_not_implemented = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}
