add_library(Horizon STATIC

        horizon/window.hpp
        horizon/window.cpp
        horizon/input_manager.cpp
        horizon/input_manager.hpp
)

target_link_libraries(Horizon PUBLIC Substratum SDL3::SDL3)

target_include_directories(Horizon PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-windowing
)