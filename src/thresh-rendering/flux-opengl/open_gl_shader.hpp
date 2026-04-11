//
// Created by Admin on 11/04/2026.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "glad/gl.h"
#include "flux-common/shader.hpp"

namespace flux {

class OpenGLShader {
    public:
        OpenGLShader(Shader::Stage stage, const std::string& glsl_source);

        ~OpenGLShader();

        OpenGLShader(const OpenGLShader&) = delete;
        OpenGLShader& operator=(const OpenGLShader&) = delete;

        OpenGLShader(OpenGLShader&& other) noexcept;
        OpenGLShader& operator=(OpenGLShader&& other) noexcept;

        [[nodiscard]] auto id() const -> GLuint { return m_id; }

        [[nodiscard]] auto valid() const -> bool { return m_id != 0; }

    private:
        static auto compile_to_spirv(Shader::Stage stage,
                                     const std::string& glsl_source) -> std::vector<std::uint32_t>;

        static auto to_gl_type(Shader::Stage stage) -> GLenum;

        GLuint m_id = 0;
};

} // flux
