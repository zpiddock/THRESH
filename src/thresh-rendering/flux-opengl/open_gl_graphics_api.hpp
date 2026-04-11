//
// Created by Admin on 11/04/2026.
//

#pragma once

#include <memory>
#include <span>
#include <string>
#include <utility>

#include "flux-common/graphics_api.hpp"
#include "horizon/window.hpp"

namespace flux {

class OpenGLGraphicsAPI : public GraphicsAPI {
    public:
        ~OpenGLGraphicsAPI() override;

        auto get_api_type() -> API_TYPE override;

        auto init() -> void override;

        auto clear(int flags) -> void override;

        auto clear_colour(float r, float g, float b, float a) -> void override;

        auto swap_buffers(const thresh::Window* window) -> void override;

        auto on_resize(uint32_t width, uint32_t height) -> void override;

        auto create_shader_program(
            std::span<const std::pair<Shader::Stage, std::string>> stages
        ) -> std::shared_ptr<ShaderProgram> override;

        auto create_vertex_array() -> std::unique_ptr<VertexArray> override;

        auto draw(const VertexArray& vao,
                  PrimitiveType primitive,
                  std::uint32_t vertex_count) -> void override;
};

} // flux
