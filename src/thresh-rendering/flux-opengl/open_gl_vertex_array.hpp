//
// Created by Admin on 11/04/2026.
//

#pragma once

#include "glad/gl.h"
#include "flux-common/vertex_array.hpp"

namespace flux {

class OpenGLVertexArray : public VertexArray {
    public:
        OpenGLVertexArray();

        ~OpenGLVertexArray() override;

        OpenGLVertexArray(const OpenGLVertexArray&) = delete;
        OpenGLVertexArray& operator=(const OpenGLVertexArray&) = delete;

        [[nodiscard]] auto id() const -> GLuint { return m_id; }

    private:
        GLuint m_id = 0;
};

} // flux
