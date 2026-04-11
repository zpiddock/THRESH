//
// Created by Admin on 11/04/2026.
//

#include "open_gl_vertex_array.hpp"

namespace flux {

OpenGLVertexArray::OpenGLVertexArray() {
    ::glCreateVertexArrays(1, &m_id);
}

OpenGLVertexArray::~OpenGLVertexArray() {
    if (m_id) {
        ::glDeleteVertexArrays(1, &m_id);
    }
}

} // flux
