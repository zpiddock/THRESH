//
// Created by Admin on 06/04/2026.
//

#include "game_app.hpp"

#include "substratum/log.hpp"
#include "thresh/engine.hpp"

namespace demo {

    auto GameApp::startup() -> void {
        m_shader = flux::load_shader("mesh_shader",
                       { flux::ShaderStage::Vertex, flux::ShaderStage::Fragment });

        m_vao = flux::create_vertex_array();
    }

    auto GameApp::update(float /*delta_time*/) -> void {

        auto* input = thresh::Engine::get_instance().input();
        if (input->key_just_released(SDL_SCANCODE_ESCAPE)) {
            thresh::Engine::get_instance().window()->setShouldClose(true);
        }
    }

    auto GameApp::render() -> void {
        if (!m_shader.valid() || !m_vao.valid()) {
            return;
        }

        flux::bind_shader(m_shader);
        flux::draw(m_vao, flux::PrimitiveType::Triangles, 3);
        flux::unbind_shader();
    }

    auto GameApp::shutdown() -> void {
        flux::destroy_shader(m_shader);
        flux::destroy_vertex_array(m_vao);
        m_shader = {};
        m_vao    = {};
        SUB_INFO("Shutting Down Demo Game, Flushing to disk, etc.");
    }

} // demo
