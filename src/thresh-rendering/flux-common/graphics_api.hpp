//
// Created by Admin on 11/04/2026.
//

#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "horizon/window.hpp"
#include "shader.hpp"
#include "shader_cache.hpp"
#include "shader_program.hpp"
#include "vertex_array.hpp"

namespace flux {

enum class API_TYPE {
    OpenGL
};

enum class PrimitiveType {
    Triangles,
    TriangleStrip,
    Lines,
    LineStrip,
    Points
};

class GraphicsAPI {
    public:
        GraphicsAPI();

        virtual ~GraphicsAPI() = default;

        virtual auto get_api_type() -> API_TYPE = 0;

        virtual auto init() -> void = 0;

        virtual auto clear(int flags) -> void = 0;

        virtual auto clear_colour(float r, float g, float b, float a) -> void = 0;

        virtual auto swap_buffers(const thresh::Window* window) -> void = 0;

        virtual auto on_resize(uint32_t width, uint32_t height) -> void = 0;

        virtual auto create_shader_program(
            std::span<const std::pair<Shader::Stage, std::string>> stages
        ) -> std::shared_ptr<ShaderProgram> = 0;

        virtual auto create_vertex_array() -> std::unique_ptr<VertexArray> = 0;

        virtual auto draw(const VertexArray& vao,
                          PrimitiveType primitive,
                          std::uint32_t vertex_count) -> void = 0;

        template<typename... Stages>
        auto load_shader(const std::string& name, Stages... stages) -> std::shared_ptr<ShaderProgram> {

            return m_shader_cache->load_shader(name, stages...);
        }

    private:

        std::unique_ptr<ShaderCache> m_shader_cache;
};

} // flux
