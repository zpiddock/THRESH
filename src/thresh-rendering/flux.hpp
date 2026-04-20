//
// flux.hpp — single public header for the Flux rendering backend.
// Include only this header from engine and application code.
//

#pragma once

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string_view>

#include "horizon/window.hpp"

namespace flux {

// ---------------------------------------------------------------------------
//  Handles — typed opaque uint32 IDs; zero is always invalid
// ---------------------------------------------------------------------------

template<typename Tag>
struct Handle {
    uint32_t id = 0;
    [[nodiscard]] auto valid() const -> bool { return id != 0; }
    auto operator==(const Handle&) const -> bool = default;
};

using ShaderHandle      = Handle<struct ShaderTag>;
using VertexArrayHandle = Handle<struct VertexArrayTag>;

// ---------------------------------------------------------------------------
//  Enumerations
// ---------------------------------------------------------------------------

enum class API_TYPE : uint8_t {
    OpenGL
};

enum class PrimitiveType : uint8_t {
    Triangles,
    TriangleStrip,
    Lines,
    LineStrip,
    Points
};

enum class ShaderStage : uint8_t {
    Vertex,
    Fragment,
    Geometry,
    TessControl,
    TessEval,
    Compute
};

enum class ClearFlags : uint32_t {
    None    = 0,
    Color   = 1 << 0,
    Depth   = 1 << 1,
    Stencil = 1 << 2,
};

[[nodiscard]] inline auto operator|(ClearFlags a, ClearFlags b) -> ClearFlags {
    return static_cast<ClearFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline auto operator&(ClearFlags a, ClearFlags b) -> uint32_t {
    return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
}

// ---------------------------------------------------------------------------
//  Window factory
//  Creates the appropriate window for the chosen backend.
//  Call before init().
// ---------------------------------------------------------------------------

auto create_window(API_TYPE type,
                             const thresh::WindowContext& ctx) -> std::unique_ptr<thresh::Window>;

// ---------------------------------------------------------------------------
//  Lifecycle
// ---------------------------------------------------------------------------

// Initialises the chosen backend, GL debug output, and glslang.
auto init(API_TYPE type) -> void;

// Destroys all live GPU resources and finalises glslang.
auto shutdown() -> void;

// ---------------------------------------------------------------------------
//  Frame control
// ---------------------------------------------------------------------------

auto clear(ClearFlags flags) -> void;
auto clear_colour(float r, float g, float b, float a) -> void;
auto swap_buffers(const thresh::Window* window) -> void;
auto on_resize(uint32_t width, uint32_t height) -> void;

// ---------------------------------------------------------------------------
//  Shaders
//  load_shader reads GLSL from "shader/<name>.<ext>" for each stage, compiles
//  via glslang (GLSL→SPIR-V→GL), links, and caches by name.
//  Subsequent calls with the same name return the cached handle.
// ---------------------------------------------------------------------------

auto load_shader(std::string_view name,
                           std::initializer_list<ShaderStage> stages) -> ShaderHandle;
auto bind_shader(ShaderHandle handle) -> void;
auto unbind_shader() -> void;
auto destroy_shader(ShaderHandle handle) -> void;

// ---------------------------------------------------------------------------
//  Vertex arrays
// ---------------------------------------------------------------------------

auto create_vertex_array() -> VertexArrayHandle;
auto destroy_vertex_array(VertexArrayHandle handle) -> void;

// ---------------------------------------------------------------------------
//  Drawing
// ---------------------------------------------------------------------------

auto draw(VertexArrayHandle vao, PrimitiveType primitive,
                   uint32_t vertex_count) -> void;

} // namespace flux
