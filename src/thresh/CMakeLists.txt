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
)

target_link_libraries(Thresh PUBLIC
        Substratum Horizon Flux Asset_Schema
        flecs::flecs
        stb
        glaze::glaze
        zstd
        ktx
)

target_precompile_headers(Thresh PUBLIC thresh/thresh.hpp)

target_include_directories(Thresh PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh
)