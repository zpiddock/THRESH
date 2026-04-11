add_library(Flux STATIC
        flux-common/graphics_api.hpp

        flux-opengl/open_gl_window.cpp
        flux-opengl/open_gl_window.hpp
        flux-opengl/open_gl_graphics_api.cpp
        flux-opengl/open_gl_graphics_api.hpp
)

target_link_libraries(Flux PUBLIC Substratum Horizon glad_gl_core)

target_include_directories(Flux PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-rendering
)