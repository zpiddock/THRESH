//
// Created by shad0w on 17/05/2026.
//

#pragma once
#include "flux/graphics_utils.hpp"

#include <string>

namespace thresh {
    class AssetsLoader {
        public:
            explicit AssetsLoader(helix::GraphicsUtils& gfx);

            auto load_texture(const std::string& path) -> uint32_t;

            auto load_material(const std::string& path) -> uint32_t;

            auto resolve_mesh(const std::string & path) -> uint32_t;

          private:
            helix::GraphicsUtils& m_graphics;
            std::unordered_map<std::string, uint32_t> m_texture_cache;
            std::unordered_map<std::string, uint32_t> m_material_cache;
            std::unordered_map<std::string, uint32_t> m_mesh_cache;
    };
} // thresh
