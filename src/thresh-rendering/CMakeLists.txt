add_library(Flux STATIC
        flux.hpp
        opengl_backend.cpp
        flux-opengl/open_gl_window.hpp
        flux-opengl/open_gl_window.cpp
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
