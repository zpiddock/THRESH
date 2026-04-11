//
// Created by Admin on 11/04/2026.
//

#include "shader_cache.hpp"

#include <span>
#include <stdexcept>

#include "graphics_api.hpp"
#include "substratum/filesystem/vfs.hpp"
#include "substratum/log.hpp"

namespace flux {

ShaderCache::ShaderCache(GraphicsAPI& api) : m_api(api) {}

auto ShaderCache::get(const std::string& name) -> std::shared_ptr<ShaderProgram> {
    auto it = m_cache.find(name);
    return it != m_cache.end() ? it->second : nullptr;
}

auto ShaderCache::stage_extension(Shader::Stage stage) -> std::string_view {
    switch (stage) {
        case Shader::Stage::Vertex:      return ".vert";
        case Shader::Stage::Fragment:    return ".frag";
        case Shader::Stage::Geometry:    return ".geom";
        case Shader::Stage::TessControl: return ".tesc";
        case Shader::Stage::TessEval:    return ".tese";
        case Shader::Stage::Compute:     return ".comp";
    }
    std::unreachable();
}

auto ShaderCache::load_impl(const std::string& name,
                            std::initializer_list<Shader::Stage> stages) -> std::shared_ptr<ShaderProgram> {
    auto stage_sources = std::vector<std::pair<Shader::Stage, std::string>>{};
    stage_sources.reserve(stages.size());

    for (auto stage : stages) {
        auto path = "shader/" + name + std::string(stage_extension(stage));
        auto src  = substratum::VFS::read_file_string(path);
        if (src.empty()) {
            SUB_ERROR("ShaderCache: failed to load '{}'", path);
            return nullptr;
        }
        stage_sources.emplace_back(stage, std::move(src));
    }

    auto program = m_api.create_shader_program(std::span(stage_sources));
    if (!program) {
        SUB_ERROR("ShaderCache: create_shader_program failed for '{}'", name);
        return nullptr;
    }

    m_cache.emplace(name, program);
    SUB_INFO("ShaderCache: loaded '{}'", name);
    return program;
}

} // flux
