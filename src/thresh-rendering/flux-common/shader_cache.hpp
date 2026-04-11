//
// Created by Admin on 11/04/2026.
//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

#include "shader.hpp"
#include "shader_program.hpp"

namespace flux {

class GraphicsAPI;

class ShaderCache {
    public:
        explicit ShaderCache(GraphicsAPI& api);

        template<typename... Stages>
        auto load_shader(const std::string& name, Stages... stages) -> std::shared_ptr<ShaderProgram> {
            if (auto it = m_cache.find(name); it != m_cache.end()) {
                return it->second;
            }
            return load_impl(name, { stages... });
        }

        auto get(const std::string& name) -> std::shared_ptr<ShaderProgram>;

    private:
        auto load_impl(const std::string& name,
                       std::initializer_list<Shader::Stage> stages) -> std::shared_ptr<ShaderProgram>;

        static auto stage_extension(Shader::Stage stage) -> std::string_view;

        GraphicsAPI& m_api;
        std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> m_cache;
};

} // flux
