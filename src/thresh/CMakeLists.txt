add_library(Thresh STATIC
        thresh/engine.cpp
        thresh/engine.hpp
        thresh/application.cpp
        thresh/application.hpp
        thresh/scene/scene.cpp
        thresh/scene/scene.hpp
        thresh/thresh.cpp
        thresh/thresh.hpp
        thresh/scene/ecs_types.hpp
        thresh/asset/assets_loader.cpp
        thresh/asset/assets_loader.hpp
        thresh/scene/scene_serializer.cpp
        thresh/scene/scene_serializer.hpp
        thresh/debug_utils/debug_ui.cpp
        thresh/debug_utils/debug_ui.hpp
        thresh/asset/model_loader.cpp
        thresh/asset/model_loader.hpp
        thresh/physics/physics_layers.hpp
        thresh/physics/physics_world.cpp
        thresh/physics/physics_world.hpp
        thresh/physics/jolt_math.hpp
        thresh/physics/jolt_debug_draw.cpp
        thresh/physics/jolt_debug_draw.hpp
        thresh/physics/physics_components.hpp
        thresh/physics/physics_components.cpp
)

target_link_libraries(Thresh PUBLIC
        Substratum Horizon Flux Asset_Schema
        flecs::flecs
        stb
        glaze::glaze
        zstd
        ktx
        Jolt
)

target_precompile_headers(Thresh PUBLIC thresh/thresh.hpp)

target_include_directories(Thresh PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh
)

# doctest ships with the tool dependencies; no tools, no test runner.
if(TARGET doctest::doctest)
    add_subdirectory(tests)
endif()