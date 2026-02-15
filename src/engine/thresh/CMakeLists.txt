# THRESH Library - Core Utilities & Application Framework
add_library(THRESH SHARED
        src/engine.cpp
        src/renderer.cpp
        src/frame_context.cpp
        src/render_graph.cpp
)

target_include_directories(THRESH
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(THRESH PUBLIC cxx_std_26)

target_link_libraries(THRESH PUBLIC FLUX)

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