add_library(Horizon SHARED

        horizon/window.hpp
        horizon/window.cpp
)

target_link_libraries(Horizon PUBLIC Substratum SDL3::SDL3)

target_include_directories(Horizon PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-windowing
)