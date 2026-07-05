//
// Created by Admin on 05/07/2026.
//

#pragma once
#include "thresh/scene/scene.hpp"

#include <string_view>
#include <vector>

namespace thresh { class AssetsLoader; }

// Editor commands: every world mutation the editor UI can perform.
// No ImGui in here — EditorUI is one caller; the future standalone editor
// is the other. Reference implementations: docs/scene-editor-ui.md §4.
namespace thresh::edit {

    // Creates an empty child of `parent` (scene root if invalid) with a
    // sibling-unique name derived from base_name, and a default Transform.
    auto create_empty(Scene& scene, flecs::entity parent,
                      std::string_view base_name = "Entity") -> flecs::entity;

    // Empty + box primitive/default material via source components, so the
    // entity round-trips through save/load like any hand-authored block.
    auto create_block(Scene& scene, AssetsLoader& assets,
                      flecs::entity parent) -> flecs::entity;

    // destruct() — children go too, and the PhysicsBodyDestroy observer
    // releases any Jolt body.
    auto destroy_entity(flecs::entity entity) -> void;

    // Sanitises path separators, rejects empty names and sibling collisions.
    // Returns false (entity keeps its old name) on rejection.
    auto rename(flecs::entity entity, std::string_view new_name) -> bool;

    auto add_component(flecs::entity entity, flecs::entity component) -> void;
    auto remove_component(flecs::entity entity, flecs::entity component) -> void;

    // Components offered by the inspector's "+ Add Component" popup.
    // Curated, not "everything with meta" — runtime-derived components
    // must not be hand-added.
    auto addable_components(flecs::world& world) -> std::vector<flecs::entity>;

    // If `entity` has a live PhysicsBody and `edited_component` affects it
    // (or is invalid = "assume it does"), drop the PhysicsBody so the create
    // system rebuilds the Jolt body next frame from current component values.
    auto mark_physics_dirty(flecs::entity entity,
                            flecs::entity edited_component = {}) -> void;

} // namespace thresh::edit
