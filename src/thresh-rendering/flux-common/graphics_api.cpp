//
// Created by Admin on 11/04/2026.
//
#include "graphics_api.hpp"

namespace flux {
    GraphicsAPI::GraphicsAPI() {
        m_shader_cache = std::make_unique<ShaderCache>(*this);
    }
}
