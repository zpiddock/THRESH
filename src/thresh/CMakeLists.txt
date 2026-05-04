add_library(Thresh SHARED
        thresh/engine.cpp
        thresh/engine.hpp
        thresh/application.cpp
        thresh/application.hpp
        thresh/scene/scene.cpp
        thresh/scene/scene.hpp
        thresh/thresh.cpp
        thresh/thresh.hpp
        thresh/scene/ecs_types.hpp
)

target_link_libraries(Thresh PUBLIC Substratum Horizon Flux flecs::flecs_static stb)

target_precompile_headers(Thresh PUBLIC thresh/thresh.hpp)

target_include_directories(Thresh PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh
)