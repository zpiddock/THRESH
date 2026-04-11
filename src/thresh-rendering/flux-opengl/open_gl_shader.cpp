//
// Created by Admin on 11/04/2026.
//

#include "open_gl_shader.hpp"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

#include "substratum/log.hpp"

namespace flux {

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

static auto to_glslang_stage(Shader::Stage stage) -> EShLanguage {
    switch (stage) {
        case Shader::Stage::Vertex:      return EShLangVertex;
        case Shader::Stage::Fragment:    return EShLangFragment;
        case Shader::Stage::Geometry:    return EShLangGeometry;
        case Shader::Stage::TessControl: return EShLangTessControl;
        case Shader::Stage::TessEval:    return EShLangTessEvaluation;
        case Shader::Stage::Compute:     return EShLangCompute;
    }
    std::unreachable();
}

auto OpenGLShader::to_gl_type(Shader::Stage stage) -> GLenum {
    switch (stage) {
        case Shader::Stage::Vertex:      return GL_VERTEX_SHADER;
        case Shader::Stage::Fragment:    return GL_FRAGMENT_SHADER;
        case Shader::Stage::Geometry:    return GL_GEOMETRY_SHADER;
        case Shader::Stage::TessControl: return GL_TESS_CONTROL_SHADER;
        case Shader::Stage::TessEval:    return GL_TESS_EVALUATION_SHADER;
        case Shader::Stage::Compute:     return GL_COMPUTE_SHADER;
    }
    std::unreachable();
}

// ---------------------------------------------------------------------------
//  Constructor / destructor
// ---------------------------------------------------------------------------

OpenGLShader::OpenGLShader(Shader::Stage stage, const std::string& glsl_source) {
    auto spirv = compile_to_spirv(stage, glsl_source);
    if (spirv.empty()) {
        return;
    }

    m_id = glCreateShader(to_gl_type(stage));

    glShaderBinary(1, &m_id,
                   GL_SHADER_BINARY_FORMAT_SPIR_V,
                   spirv.data(),
                   static_cast<GLsizei>(spirv.size() * sizeof(std::uint32_t)));

    glSpecializeShader(m_id, "main", 0, nullptr, nullptr);

    GLint status = 0;
    glGetShaderiv(m_id, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint log_len = 0;
        glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &log_len);
        auto log = std::string(static_cast<std::size_t>(log_len), '\0');
        glGetShaderInfoLog(m_id, log_len, nullptr, log.data());
        SUB_ERROR("OpenGLShader: specialization failed:\n{}", log);
        glDeleteShader(m_id);
        m_id = 0;
    }
}

OpenGLShader::~OpenGLShader() {
    if (m_id) {
        glDeleteShader(m_id);
    }
}

OpenGLShader::OpenGLShader(OpenGLShader&& other) noexcept : m_id(other.m_id) {
    other.m_id = 0;
}

OpenGLShader& OpenGLShader::operator=(OpenGLShader&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteShader(m_id);
        m_id = other.m_id;
        other.m_id = 0;
    }
    return *this;
}

// ---------------------------------------------------------------------------
//  GLSL → SPIR-V via glslang
// ---------------------------------------------------------------------------

auto OpenGLShader::compile_to_spirv(Shader::Stage stage,
                                     const std::string& glsl_source) -> std::vector<std::uint32_t> {
    const auto glslang_stage = to_glslang_stage(stage);
    const auto* src_ptr      = glsl_source.c_str();

    glslang::TShader shader(glslang_stage);
    shader.setStrings(&src_ptr, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, glslang_stage,
                       glslang::EShClientOpenGL, 460);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EShTargetSpv,    glslang::EShTargetSpv_1_0);

    if (!shader.parse(GetDefaultResources(), 460, false, EShMsgDefault)) {
        SUB_ERROR("OpenGLShader: GLSL parse failed:\n{}\n{}",
                  shader.getInfoLog(), shader.getInfoDebugLog());
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault)) {
        SUB_ERROR("OpenGLShader: GLSL link failed:\n{}\n{}",
                  program.getInfoLog(), program.getInfoDebugLog());
        return {};
    }

    auto spirv = std::vector<std::uint32_t>{};
    glslang::GlslangToSpv(*program.getIntermediate(glslang_stage), spirv);
    return spirv;
}

} // flux
