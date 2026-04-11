//
// Created by Admin on 11/04/2026.
//

#include "open_gl_graphics_api.hpp"

#include <print>
#include <vector>

#include "glad/gl.h"
#include "horizon/window.hpp"
#include "SDL3/SDL_video.h"

#include "open_gl_shader.hpp"
#include "open_gl_shader_program.hpp"
#include "open_gl_vertex_array.hpp"

namespace flux {

    void gl_debug_callback(GLenum        source,   GLenum  type,
                                    GLuint        id,       GLenum  severity,
                                    GLsizei       /*length*/,
                                    const GLchar* msg,      const void* /*userdata*/) {
    using sv = std::string_view;

    constexpr auto source_str = [](GLenum s) -> sv {
        switch (s) {
            case GL_DEBUG_SOURCE_API:             return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "Window System";
            case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
            case GL_DEBUG_SOURCE_THIRD_PARTY:     return "Third Party";
            case GL_DEBUG_SOURCE_APPLICATION:     return "Application";
            default:                              return "Unknown";
        }
    };

    constexpr auto type_str = [](GLenum t) -> sv {
        switch (t) {
            case GL_DEBUG_TYPE_ERROR:               return "Error";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated Behaviour";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "Undefined Behaviour";
            case GL_DEBUG_TYPE_PORTABILITY:         return "Portability";
            case GL_DEBUG_TYPE_PERFORMANCE:         return "Performance";
            case GL_DEBUG_TYPE_MARKER:              return "Marker";
            case GL_DEBUG_TYPE_OTHER:               return "Other";
            default:                                return "Unknown";
        }
    };

    constexpr auto severity_str = [](GLenum s) -> sv {
        switch (s) {
            case GL_DEBUG_SEVERITY_HIGH:         return "High";
            case GL_DEBUG_SEVERITY_MEDIUM:       return "Medium";
            case GL_DEBUG_SEVERITY_LOW:          return "Low";
            case GL_DEBUG_SEVERITY_NOTIFICATION: return "Notification";
            default:                             return "Unknown";
        }
    };

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    std::println("[GL] {} | source: {} | type: {} | severity: {} | {}",
                 id, source_str(source), type_str(type), severity_str(severity), msg);
}

    OpenGLGraphicsAPI::~OpenGLGraphicsAPI() = default;

    auto OpenGLGraphicsAPI::get_api_type() -> API_TYPE {
        return API_TYPE::OpenGL;
    }

    auto OpenGLGraphicsAPI::init() -> void {

        ::glEnable(GL_DEPTH_TEST);
        ::glEnable(GL_DEBUG_OUTPUT);
        ::glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        ::glDebugMessageCallback(gl_debug_callback, nullptr);
    }

    auto OpenGLGraphicsAPI::clear(const int flags) -> void {
        ::glClear(flags);
    }

    auto OpenGLGraphicsAPI::clear_colour(float r, float g, float b, float a) -> void {

        ::glClearColor(r, g, b, a);
    }

    auto OpenGLGraphicsAPI::swap_buffers(const thresh::Window* window) -> void {
        SDL_GL_SwapWindow(window->getWindow());
    }

    auto OpenGLGraphicsAPI::on_resize(uint32_t width, uint32_t height) -> void {
        ::glViewport(0, 0, width, height);
    }

    auto OpenGLGraphicsAPI::create_shader_program(
        std::span<const std::pair<Shader::Stage, std::string>> stages
    ) -> std::shared_ptr<ShaderProgram> {
        auto shaders = std::vector<OpenGLShader>{};
        shaders.reserve(stages.size());
        for (auto& [stage, src] : stages) {
            shaders.emplace_back(stage, src);
            if (!shaders.back().valid()) {
                return nullptr;
            }
        }
        auto program = std::make_shared<OpenGLShaderProgram>(std::span(shaders));
        return program->valid() ? program : nullptr;
    }

    auto OpenGLGraphicsAPI::create_vertex_array() -> std::unique_ptr<VertexArray> {
        return std::make_unique<OpenGLVertexArray>();
    }

    auto OpenGLGraphicsAPI::draw(const VertexArray& vao,
                                 PrimitiveType primitive,
                                 std::uint32_t vertex_count) -> void {
        constexpr auto to_gl_primitive = [](PrimitiveType p) -> GLenum {
            switch (p) {
                case PrimitiveType::Triangles:     return GL_TRIANGLES;
                case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
                case PrimitiveType::Lines:         return GL_LINES;
                case PrimitiveType::LineStrip:     return GL_LINE_STRIP;
                case PrimitiveType::Points:        return GL_POINTS;
            }
            std::unreachable();
        };

        const auto& gl_vao = static_cast<const OpenGLVertexArray&>(vao);
        ::glBindVertexArray(gl_vao.id());
        ::glDrawArrays(to_gl_primitive(primitive), 0, static_cast<GLsizei>(vertex_count));
        ::glBindVertexArray(0);
    }

} // flux