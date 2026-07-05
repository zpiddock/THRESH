//
// Created by Admin on 05/07/2026.
//

#include "scene_commands.hpp"

#include "ImVectorEditor.h"
#include "substratum/log.hpp"
#include "thresh/asset/assets_loader.hpp"
#include "thresh/physics/physics_components.hpp"
#include "thresh/scene/ecs_types.hpp"

auto unique_child_name(flecs::entity parent, std::string_view base_name) -> std::string {

    std::string name{base_name};
    int suffix = 2;
    while (parent.lookup(name.c_str()).is_valid()) {
        name = std::format("{} {}", base_name, suffix++);
    }
    return name;
}


namespace thresh::edit {

    auto create_empty(Scene& scene, flecs::entity parent,
                      std::string_view base_name) -> flecs::entity {

        if (!parent.is_valid()) {
            parent = scene.root();
        }

        auto entity = scene.world().entity().child_of(parent);
        entity.set_name(unique_child_name(parent, base_name).c_str());
        entity.set<Transform>({});

        return entity;
    }

    auto create_block(Scene& scene, AssetsLoader& assets,
                      flecs::entity parent) -> flecs::entity {

        auto entity = create_empty(scene, parent, "Block");
        entity.set<MeshSource>({.path = "primitive://box"});
        entity.set<MaterialSource>({.path = "material/default.mat"});
        entity.set<Mesh>({assets.resolve_mesh("primitive://box"),
                          assets.load_material("material/default.mat")});
        return entity;
    }

    auto destroy_entity(flecs::entity entity) -> void {
        if (entity.is_alive()) {
            entity.destruct();
        }
    }

    auto rename(flecs::entity entity, std::string_view new_name) -> bool {
        // '.' and ':' are flecs path separators, a name containing them would
        // become a nested path.
        std::string name{new_name};
        std::erase_if(name, [](char c) { return c == '.' || c == ':'; });
        if (name.empty()) {
            SUB_WARN("rename() called with . or : in name");
            return false;
        }
        if (auto sibling = entity.parent().lookup(name.c_str());
            sibling.is_valid() && sibling != entity) {
            return false;
        }
        entity.set_name(name.c_str());
        return true;
    }

    auto add_component(flecs::entity entity, flecs::entity component) -> void {
        entity.add(component);
        mark_physics_dirty(entity, component);
    }

    auto remove_component(flecs::entity entity, flecs::entity component) -> void {
        entity.remove(component);
        mark_physics_dirty(entity, component);
    }

    auto addable_components(flecs::world& world) -> std::vector<flecs::entity> {
        // Deliberately curated, runtime-derived
        // components must not be hand-added.
        return {
            world.component<Transform>(),
            world.component<Camera>(),
            world.component<Light>(),
            world.component<MeshSource>(),
            world.component<MaterialSource>(),
            world.component<RigidBody>(),
            world.component<BoxCollider>(),
            world.component<SphereCollider>(),
            world.component<CapsuleCollider>(),
            world.component<CharacterController>(),
        };
    }

    auto mark_physics_dirty(flecs::entity entity,
                            flecs::entity edited_component) -> void {
        auto world = entity.world();
        const bool relevant =
            !edited_component.is_valid()
            || edited_component == world.component<Transform>()
            || edited_component == world.component<RigidBody>()
            || edited_component == world.component<BoxCollider>()
            || edited_component == world.component<SphereCollider>()
            || edited_component == world.component<CapsuleCollider>();
        if (!relevant) {
            return;
        }

        // Observer destroys the Jolt body, PhysicsBodyCreate rebuilds it
        // next frame from the current WorldTransform/collider/RigidBody.
        if (entity.has<PhysicsBody>()) {
            entity.remove<PhysicsBody>();
        }

        // Bodies live in world space, so moving a parent stales every
        // descendant's body too.
        const bool affects_subtree =
            !edited_component.is_valid()
            || edited_component == world.component<Transform>();
        if (!affects_subtree) {
            return;
        }
        auto drop = [](this auto& self, flecs::entity e) -> void {
            if (e.has<PhysicsBody>()) {
                e.remove<PhysicsBody>();
            }
            e.children([&](flecs::entity child) { self(child); });
        };
        entity.children([&](flecs::entity child) { drop(child); });
    }

} // namespace thresh::edit
