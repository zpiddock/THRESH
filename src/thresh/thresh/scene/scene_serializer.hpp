//
// Created by shad0w on 20/05/2026.
//

#pragma once
#include "scene.hpp"
#include "thresh/asset/assets_loader.hpp"

namespace thresh {

class SceneSerializer {

    public:
         /**
         *  Register reflection metadata on every serializable component + nested math types
         * @param world
         */
        static auto register_components(flecs::world& world) -> void;

        static auto save_scene(Scene& scene, const std::string& vfs_path) -> bool;

        static auto load_scene(AssetsLoader& loader, const std::string& vfs_path) -> std::unique_ptr<Scene>;
    private:
        static auto register_math_components(flecs::world& world) -> void;
};

} // namespace thresh