//
// Created by Admin on 11/04/2026.
//

#include "open_gl_shader_program.hpp"

#include "substratum/log.hpp"

namespace flux {

OpenGLShaderProgram::OpenGLShaderProgram(std::span<OpenGLShader> shaders) {
    m_id = glCreateProgram();

    for (auto& shader : shaders) {
        if (shader.valid()) {
            glAttachShader(m_id, shader.id());
        }
    }

    glLinkProgram(m_id);

    GLint status = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &status);
    if (!status) {
        GLint log_len = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &log_len);
        auto log = std::string(static_cast<std::size_t>(log_len), '\0');
        glGetProgramInfoLog(m_id, log_len, nullptr, log.data());
        SUB_ERROR("OpenGLShaderProgram: link failed:\n{}", log);
        glDeleteProgram(m_id);
        m_id = 0;
        return;
    }

    // Detach after successful link — shaders can be deleted independently
    for (auto& shader : shaders) {
        if (shader.valid()) {
            glDetachShader(m_id, shader.id());
        }
    }

    SUB_INFO("OpenGLShaderProgram: linked (id={})", m_id);
}

OpenGLShaderProgram::~OpenGLShaderProgram() {
    if (m_id) {
        glDeleteProgram(m_id);
    }
}

auto OpenGLShaderProgram::bind() -> void {
    glUseProgram(m_id);
}

auto OpenGLShaderProgram::unbind() -> void {
    glUseProgram(0);
}

} // flux
