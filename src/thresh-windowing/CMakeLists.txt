add_library(Horizon STATIC

        horizon/window.hpp
        horizon/window.cpp
)

target_link_libraries(Horizon PUBLIC Substratum SDL3::SDL3 glad_gl_core)

target_include_directories(Horizon PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-windowing
)