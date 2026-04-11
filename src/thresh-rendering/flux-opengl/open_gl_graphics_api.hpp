//
// Created by Admin on 11/04/2026.
//

#pragma once
#include "flux-common/graphics_api.hpp"
#include "horizon/window.hpp"

namespace flux {

class OpenGLGraphicsAPI : public GraphicsAPI {

    public:
        ~OpenGLGraphicsAPI() override;

        auto get_api_type() -> API_TYPE override;

        auto clear(int flags) -> void override;

        auto clear_colour(float r, float g, float b, float a) -> void override;

        auto swap_buffers(const thresh::Window* window) -> void override;

        auto on_resize(uint32_t width, uint32_t height) -> void override;
};
} // flux
