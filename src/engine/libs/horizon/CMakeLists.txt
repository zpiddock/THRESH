# Horizon
# Provides GLFW abstractions like Window and Input Handling

add_library(HORIZON SHARED
        src/window.cpp
        src/input.cpp
)

target_include_directories(HORIZON
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(HORIZON PUBLIC cxx_std_26)

target_link_libraries(HORIZON
        PUBLIC
        SUBSTRATUM
        glfw
)

# Set output name and versioning
set_target_properties(HORIZON PROPERTIES
        OUTPUT_NAME "horizon"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Windows DLL export/import
if (WIN32)
    target_compile_definitions(HORIZON
            PRIVATE HORIZON_EXPORTS
            INTERFACE HORIZON_IMPORTS
    )
endif ()