#include "thresh/scene.hpp"
#include "substratum/log.hpp"

namespace thresh {

    auto Scene::create_entity(const std::string &name) -> entt::entity {
        auto entity = m_registry.create();
        m_registry.emplace<TagComponent>(entity, TagComponent{name});
        return entity;
    }

    auto Scene::destroy_entity(entt::entity entity) -> void {
        // Remove from parent's children list if part of hierarchy
        if (m_registry.all_of<HierarchyComponent>(entity)) {
            auto &hierarchy = m_registry.get<HierarchyComponent>(entity);

            // Unlink from parent
            if (hierarchy.parent != entt::null && m_registry.valid(hierarchy.parent)) {
                if (m_registry.all_of<HierarchyComponent>(hierarchy.parent)) {
                    auto &parent_hierarchy = m_registry.get<HierarchyComponent>(hierarchy.parent);
                    auto &children = parent_hierarchy.children;
                    children.erase(
                        std::remove(children.begin(), children.end(), entity),
                        children.end()
                    );
                }
            }

            // Destroy children recursively
            for (auto child : hierarchy.children) {
                if (m_registry.valid(child)) {
                    destroy_entity(child);
                }
            }
        }

        m_registry.destroy(entity);
    }

    auto Scene::extract_render_data(
        const Camera &camera,
        float aspect_ratio,
        const DirectionalLight &sun
    ) const -> FrameRenderData {
        FrameRenderData data;
        data.view = camera.get_view_matrix();
        data.projection = camera.get_projection_matrix(aspect_ratio);
        data.camera_position = camera.get_position();
        data.sun = sun;

        // Iterate all entities with Transform + Mesh + Material
        auto view = m_registry.view<const TransformComponent, const MeshComponent, const MaterialComponent>();

        for (auto [entity, transform, mesh, material] : view.each()) {
            RenderObject obj;
            obj.model_matrix = transform.get_model_matrix();
            obj.mesh_index = mesh.mesh_index;
            obj.material_index = material.material_handle;
            data.objects.push_back(obj);
        }

        return data;
    }

} // namespace thresh
