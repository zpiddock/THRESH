//
// Created by Admin on 11/04/2026.
//

#pragma once

#include <span>
#include <utility>

#include "glad/gl.h"
#include "flux-common/shader.hpp"
#include "flux-common/shader_program.hpp"
#include "open_gl_shader.hpp"

namespace flux {

class OpenGLShaderProgram : public ShaderProgram {
    public:
        explicit OpenGLShaderProgram(std::span<OpenGLShader> shaders);

        ~OpenGLShaderProgram() override;

        OpenGLShaderProgram(const OpenGLShaderProgram&) = delete;
        OpenGLShaderProgram& operator=(const OpenGLShaderProgram&) = delete;

        auto bind() -> void override;

        auto unbind() -> void override;

        [[nodiscard]] auto valid() const -> bool { return m_id != 0; }

    private:
        GLuint m_id = 0;
};

} // flux
