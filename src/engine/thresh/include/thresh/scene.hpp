#pragma once

#include "components.hpp"
#include "render_data.hpp"
#include "camera.hpp"

#include <entt/entt.hpp>
#include <string>

#ifdef _WIN32
#ifdef THRESH_EXPORTS
#define THRESH_API __declspec(dllexport)
#else
#define THRESH_API __declspec(dllimport)
#endif
#else
#define THRESH_API
#endif

namespace thresh {

    /**
     * ECS-backed scene graph.
     *
     * Wraps an entt::registry and provides high-level entity management +
     * render data extraction for the forward pass.
     */
    class THRESH_API Scene {
    public:
        Scene() = default;
        ~Scene() = default;

        Scene(const Scene &) = delete;
        auto operator=(const Scene &) -> Scene & = delete;

        /**
         * Create a new entity with a TagComponent.
         * @param name Human-readable name
         * @return The new entity handle
         */
        auto create_entity(const std::string &name = "Entity") -> entt::entity;

        /**
         * Destroy an entity and all its components.
         */
        auto destroy_entity(entt::entity entity) -> void;

        /**
         * Add a component to an entity.
         */
        template <typename T, typename... Args>
        auto add_component(entt::entity entity, Args &&...args) -> T & {
            return m_registry.emplace<T>(entity, std::forward<Args>(args)...);
        }

        /**
         * Get a component from an entity.
         */
        template <typename T>
        [[nodiscard]] auto get_component(entt::entity entity) -> T & {
            return m_registry.get<T>(entity);
        }

        /**
         * Get a const component from an entity.
         */
        template <typename T>
        [[nodiscard]] auto get_component(entt::entity entity) const -> const T & {
            return m_registry.get<T>(entity);
        }

        /**
         * Check if an entity has a component.
         */
        template <typename T>
        [[nodiscard]] auto has_component(entt::entity entity) const -> bool {
            return m_registry.all_of<T>(entity);
        }

        /**
         * Extract render data from all entities with Transform + Mesh + Material.
         * Also extracts point lights from entities with Transform + PointLightComponent.
         * @param camera Camera for view/projection
         * @param aspect_ratio Viewport width / height
         * @param sun Directional light
         * @param ambient Ambient light
         * @return FrameRenderData ready for the forward pass
         */
        [[nodiscard]] auto extract_render_data(
            const Camera &camera,
            float aspect_ratio,
            const DirectionalLight &sun = {},
            const AmbientLight &ambient = {}
        ) const -> FrameRenderData;

        /**
         * Get the underlying registry for advanced queries.
         */
        [[nodiscard]] auto get_registry() -> entt::registry & { return m_registry; }
        [[nodiscard]] auto get_registry() const -> const entt::registry & { return m_registry; }

    private:
        entt::registry m_registry;
    };

} // namespace thresh
