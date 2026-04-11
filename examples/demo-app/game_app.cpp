//
// Created by Admin on 06/04/2026.
//

#include "game_app.hpp"

#include "substratum/log.hpp"
#include "thresh/engine.hpp"

namespace demo {

    auto GameApp::startup() -> void {
        auto& engine = thresh::Engine::get_instance();

        m_shader = engine.get_graphics_api().load_shader("mesh_shader",
                       flux::Shader::Stage::Vertex,
                       flux::Shader::Stage::Fragment);

        m_vao = engine.get_graphics_api().create_vertex_array();
    }

    auto GameApp::update(float delta_time) -> void {
    }

    auto GameApp::render() -> void {
        if (!m_shader || !m_vao) {
            return;
        }

        auto& api = thresh::Engine::get_instance().get_graphics_api();

        m_shader->bind();
        api.draw(*m_vao, flux::PrimitiveType::Triangles, 3);
        m_shader->unbind();
    }

    auto GameApp::shutdown() -> void {
        m_shader.reset();
        m_vao.reset();
        SUB_INFO("Shutting Down Demo Game, Flushing to disk, etc.");
    }

} // demo
