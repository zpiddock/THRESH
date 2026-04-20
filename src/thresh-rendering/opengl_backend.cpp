//
// opengl_backend.cpp — OpenGL implementation of the flux rendering API.
// All GL types and handles are confined to this translation unit.
//

#include "flux.hpp"

#include <print>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "glad/gl.h"
#include "SDL3/SDL_video.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

#include "horizon/window.hpp"
#include "flux-opengl/open_gl_window.hpp"
#include "substratum/log.hpp"
#include "substratum/filesystem/vfs.hpp"

namespace flux {

namespace {

struct OpenGLState {
    std::unordered_map<uint32_t, GLuint> programs;          // ShaderHandle.id → GL program
    std::unordered_map<uint32_t, GLuint> vaos;              // VertexArrayHandle.id → GL VAO
    std::unordered_map<std::string, ShaderHandle> shader_name_cache;
    uint32_t next_handle = 1;

    auto alloc_id() -> uint32_t { return next_handle++; }
};

static OpenGLState g_state;

// ---------------------------------------------------------------------------
//  GL debug output callback
// ---------------------------------------------------------------------------

auto gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                        GLsizei /*length*/, const GLchar* msg,
                        const void* /*userdata*/) -> void {
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

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }

    std::println("[GL] {} | source: {} | type: {} | severity: {} | {}",
                 id, source_str(source), type_str(type), severity_str(severity), msg);
}

auto to_glslang_stage(ShaderStage stage) -> EShLanguage {
    switch (stage) {
        case ShaderStage::Vertex:      return EShLangVertex;
        case ShaderStage::Fragment:    return EShLangFragment;
        case ShaderStage::Geometry:    return EShLangGeometry;
        case ShaderStage::TessControl: return EShLangTessControl;
        case ShaderStage::TessEval:    return EShLangTessEvaluation;
        case ShaderStage::Compute:     return EShLangCompute;
    }
    std::unreachable();
}

auto to_gl_shader_type(ShaderStage stage) -> GLenum {
    switch (stage) {
        case ShaderStage::Vertex:      return GL_VERTEX_SHADER;
        case ShaderStage::Fragment:    return GL_FRAGMENT_SHADER;
        case ShaderStage::Geometry:    return GL_GEOMETRY_SHADER;
        case ShaderStage::TessControl: return GL_TESS_CONTROL_SHADER;
        case ShaderStage::TessEval:    return GL_TESS_EVALUATION_SHADER;
        case ShaderStage::Compute:     return GL_COMPUTE_SHADER;
    }
    std::unreachable();
}

auto stage_file_extension(ShaderStage stage) -> std::string_view {
    switch (stage) {
        case ShaderStage::Vertex:      return ".vert";
        case ShaderStage::Fragment:    return ".frag";
        case ShaderStage::Geometry:    return ".geom";
        case ShaderStage::TessControl: return ".tesc";
        case ShaderStage::TessEval:    return ".tese";
        case ShaderStage::Compute:     return ".comp";
    }
    std::unreachable();
}

auto compile_glsl_to_spirv(ShaderStage stage,
                            const std::string& glsl_source) -> std::vector<uint32_t> {
    const auto glslang_stage = to_glslang_stage(stage);
    const auto* src_ptr      = glsl_source.c_str();

    glslang::TShader shader(glslang_stage);
    shader.setStrings(&src_ptr, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, glslang_stage,
                       glslang::EShClientOpenGL, 460);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EShTargetSpv,    glslang::EShTargetSpv_1_0);

    if (!shader.parse(GetDefaultResources(), 460, false, EShMsgDefault)) {
        SUB_ERROR("flux: GLSL parse failed:\n{}\n{}",
                  shader.getInfoLog(), shader.getInfoDebugLog());
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault)) {
        SUB_ERROR("flux: GLSL link failed:\n{}\n{}",
                  program.getInfoLog(), program.getInfoDebugLog());
        return {};
    }

    auto spirv = std::vector<uint32_t>{};
    glslang::GlslangToSpv(*program.getIntermediate(glslang_stage), spirv);
    return spirv;
}

auto create_gl_program(std::span<const std::pair<ShaderStage, std::string>> stages) -> GLuint {
    auto shader_ids = std::vector<GLuint>{};
    shader_ids.reserve(stages.size());

    const auto cleanup_shaders = [&] {
        for (auto id : shader_ids) { glDeleteShader(id); }
        shader_ids.clear();
    };

    for (auto& [stage, src] : stages) {
        auto spirv = compile_glsl_to_spirv(stage, src);
        if (spirv.empty()) {
            cleanup_shaders();
            return 0;
        }

        auto gl_shader = glCreateShader(to_gl_shader_type(stage));
        glShaderBinary(1, &gl_shader,
                       GL_SHADER_BINARY_FORMAT_SPIR_V,
                       spirv.data(),
                       static_cast<GLsizei>(spirv.size() * sizeof(uint32_t)));
        glSpecializeShader(gl_shader, "main", 0, nullptr, nullptr);

        GLint status = 0;
        glGetShaderiv(gl_shader, GL_COMPILE_STATUS, &status);
        if (!status) {
            GLint log_len = 0;
            glGetShaderiv(gl_shader, GL_INFO_LOG_LENGTH, &log_len);
            auto log = std::string(static_cast<std::size_t>(log_len), '\0');
            glGetShaderInfoLog(gl_shader, log_len, nullptr, log.data());
            SUB_ERROR("flux: shader specialization failed:\n{}", log);
            glDeleteShader(gl_shader);
            cleanup_shaders();
            return 0;
        }

        shader_ids.push_back(gl_shader);
    }

    auto program = glCreateProgram();
    for (auto id : shader_ids) { glAttachShader(program, id); }
    glLinkProgram(program);

    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        GLint log_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
        auto log = std::string(static_cast<std::size_t>(log_len), '\0');
        glGetProgramInfoLog(program, log_len, nullptr, log.data());
        SUB_ERROR("flux: program link failed:\n{}", log);
        glDeleteProgram(program);
        cleanup_shaders();
        return 0;
    }

    for (auto id : shader_ids) {
        glDetachShader(program, id);
        glDeleteShader(id);
    }

    SUB_INFO("flux: shader program linked (gl_id={})", program);
    return program;
}

} // anonymous namespace

auto create_window(API_TYPE type,
                   const thresh::WindowContext& ctx) -> std::unique_ptr<thresh::Window> {
    switch (type) {
        case API_TYPE::OpenGL:
            return OpenGLWindow::create(ctx);
    }
    std::unreachable();
}

auto init(API_TYPE /*type*/) -> void {
    glslang::InitializeProcess();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_callback, nullptr);

    SUB_INFO("flux: OpenGL backend initialised");
}

auto shutdown() -> void {
    for (auto& [id, gl_prog] : g_state.programs) {
        glDeleteProgram(gl_prog);
    }
    for (auto& [id, gl_vao] : g_state.vaos) {
        glDeleteVertexArrays(1, &gl_vao);
    }

    g_state.programs.clear();
    g_state.vaos.clear();
    g_state.shader_name_cache.clear();
    g_state.next_handle = 1;

    glslang::FinalizeProcess();

    SUB_INFO("flux: OpenGL backend shut down");
}

auto clear(ClearFlags flags) -> void {
    GLbitfield bits = 0;
    if (flags & ClearFlags::Color)   { bits |= GL_COLOR_BUFFER_BIT;   }
    if (flags & ClearFlags::Depth)   { bits |= GL_DEPTH_BUFFER_BIT;   }
    if (flags & ClearFlags::Stencil) { bits |= GL_STENCIL_BUFFER_BIT; }
    glClear(bits);
}

auto clear_colour(float r, float g, float b, float a) -> void {
    glClearColor(r, g, b, a);
}

auto swap_buffers(const thresh::Window* window) -> void {
    SDL_GL_SwapWindow(window->getWindow());
}

auto on_resize(uint32_t width, uint32_t height) -> void {
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

auto load_shader(std::string_view name,
                 std::initializer_list<ShaderStage> stages) -> ShaderHandle {
    auto key = std::string(name);

    if (auto it = g_state.shader_name_cache.find(key);
        it != g_state.shader_name_cache.end()) {
        return it->second;
    }

    auto stage_sources = std::vector<std::pair<ShaderStage, std::string>>{};
    stage_sources.reserve(stages.size());

    for (auto stage : stages) {
        auto path = "shader/" + key + std::string(stage_file_extension(stage));
        auto src  = substratum::VFS::read_file_string(path);
        if (src.empty()) {
            SUB_ERROR("flux: could not read shader source '{}'", path);
            return ShaderHandle{};
        }
        stage_sources.emplace_back(stage, std::move(src));
    }

    auto gl_prog = create_gl_program(std::span(stage_sources));
    if (gl_prog == 0) {
        SUB_ERROR("flux: load_shader failed for '{}'", name);
        return ShaderHandle{};
    }

    auto handle = ShaderHandle{ g_state.alloc_id() };
    g_state.programs.emplace(handle.id, gl_prog);
    g_state.shader_name_cache.emplace(key, handle);
    SUB_INFO("flux: loaded shader '{}' (handle={})", name, handle.id);
    return handle;
}

auto bind_shader(ShaderHandle handle) -> void {
    if (auto it = g_state.programs.find(handle.id); it != g_state.programs.end()) {
        glUseProgram(it->second);
    }
}

auto unbind_shader() -> void {
    glUseProgram(0);
}

auto destroy_shader(ShaderHandle handle) -> void {
    if (auto it = g_state.programs.find(handle.id); it != g_state.programs.end()) {
        glDeleteProgram(it->second);
        g_state.programs.erase(it);
    }
    std::erase_if(g_state.shader_name_cache,
                  [&](const auto& pair) { return pair.second.id == handle.id; });
}

auto create_vertex_array() -> VertexArrayHandle {
    GLuint gl_vao = 0;
    glCreateVertexArrays(1, &gl_vao);
    auto handle = VertexArrayHandle{ g_state.alloc_id() };
    g_state.vaos.emplace(handle.id, gl_vao);
    return handle;
}

auto destroy_vertex_array(VertexArrayHandle handle) -> void {
    if (auto it = g_state.vaos.find(handle.id); it != g_state.vaos.end()) {
        glDeleteVertexArrays(1, &it->second);
        g_state.vaos.erase(it);
    }
}

auto draw(VertexArrayHandle vao, PrimitiveType primitive, uint32_t vertex_count) -> void {
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

    auto it = g_state.vaos.find(vao.id);
    if (it == g_state.vaos.end()) {
        SUB_WARN("flux::draw: invalid VertexArrayHandle {}", vao.id);
        return;
    }

    glBindVertexArray(it->second);
    glDrawArrays(to_gl_primitive(primitive), 0, static_cast<GLsizei>(vertex_count));
    glBindVertexArray(0);
}
} // namespace flux
