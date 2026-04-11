//
// Created by Admin on 11/04/2026.
//

#pragma once
#include <cstdint>

#include "horizon/window.hpp"

namespace flux {

    enum class API_TYPE {
        OpenGL
    };

    class GraphicsAPI {

        public:
            GraphicsAPI() = default;

            virtual ~GraphicsAPI() = default;

            virtual auto get_api_type() -> API_TYPE = 0;

            virtual auto clear(int flags) -> void = 0;

            virtual auto clear_colour(float r, float g, float b, float a) -> void = 0;

            virtual auto swap_buffers(const thresh::Window* window) -> void = 0;

            virtual auto on_resize(uint32_t width, uint32_t height) -> void = 0;
    };

} // flux
