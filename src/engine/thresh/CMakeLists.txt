# THRESH Library - Core Utilities & Application Framework
add_library(THRESH SHARED
        src/engine.cpp
        src/renderer.cpp
        src/frame_context.cpp
        src/render_graph.cpp
        src/camera.cpp
        src/cgltf_impl.cpp
        src/mesh_loader.cpp
        src/material_system.cpp
        src/scene.cpp
        src/forward_pass.cpp
        src/asset_system.cpp
)

target_include_directories(THRESH
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(THRESH PUBLIC cxx_std_26)

target_link_libraries(THRESH
        PUBLIC
        FLUX
        glm::glm
        EnTT::EnTT
        PRIVATE
        cgltf
)

# GLM configuration - Vulkan depth range and radians
target_compile_definitions(THRESH
        PUBLIC
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
)

# Set output name and versioning
set_target_properties(THRESH PROPERTIES
        OUTPUT_NAME "thresh"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Windows DLL export/import
if (WIN32)
    target_compile_definitions(THRESH
            PRIVATE THRESH_EXPORTS
            INTERFACE THRESH_IMPORTS
    )
endif ()
