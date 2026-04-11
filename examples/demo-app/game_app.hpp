//
// Created by Admin on 06/04/2026.
//

#pragma once

#include <memory>

#include "thresh/application.hpp"
#include "flux-common/shader_program.hpp"
#include "flux-common/vertex_array.hpp"

namespace demo {

class GameApp : public thresh::Application {
    public:
        auto startup() -> void override;

        auto update(float delta_time) -> void override;

        auto render() -> void override;

        auto shutdown() -> void override;

    private:
        std::shared_ptr<flux::ShaderProgram> m_shader;
        std::unique_ptr<flux::VertexArray> m_vao;
};

} // demo
