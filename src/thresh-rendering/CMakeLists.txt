add_library(Flux STATIC
        flux-common/graphics_api.hpp
        flux-common/graphics_api.cpp
        flux-common/shader.hpp
        flux-common/shader.cpp
        flux-common/shader_program.hpp
        flux-common/vertex_array.hpp
        flux-common/shader_cache.hpp
        flux-common/shader_cache.cpp

        flux-opengl/open_gl_window.cpp
        flux-opengl/open_gl_window.hpp
        flux-opengl/open_gl_graphics_api.cpp
        flux-opengl/open_gl_graphics_api.hpp
        flux-opengl/open_gl_shader.hpp
        flux-opengl/open_gl_shader.cpp
        flux-opengl/open_gl_shader_program.hpp
        flux-opengl/open_gl_shader_program.cpp
        flux-opengl/open_gl_vertex_array.hpp
        flux-opengl/open_gl_vertex_array.cpp
)

target_link_libraries(Flux PUBLIC
        Substratum
        Horizon
        glad_gl_core
        glslang
        SPIRV
        glslang-default-resource-limits
)

target_include_directories(Flux PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-rendering
)
