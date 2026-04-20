//
// Created by Admin on 06/04/2026.
//

#pragma once

#include "thresh/application.hpp"
#include "flux.hpp"

namespace demo {

class GameApp : public thresh::Application {
    public:
        auto startup() -> void override;

        auto update(float delta_time) -> void override;

        auto render() -> void override;

        auto shutdown() -> void override;

    private:
        flux::ShaderHandle      m_shader{};
        flux::VertexArrayHandle m_vao{};
};

} // demo
