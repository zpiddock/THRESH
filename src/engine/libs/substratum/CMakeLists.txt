# Substratum Library - Core Utilities & Application Framework
add_library(SUBSTRATUM SHARED
        src/core.cpp
        src/log.cpp
        src/file_watcher.cpp
        src/vfs.cpp
)

target_include_directories(SUBSTRATUM
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(SUBSTRATUM PUBLIC cxx_std_26)

# Link Cereal for JSON serialization (header-only)
target_link_libraries(SUBSTRATUM PUBLIC ser20)

# PhysicsFS for virtual filesystem (static, compiled into our DLL)
target_link_libraries(SUBSTRATUM PRIVATE physfs-static)

# Set output name and versioning
set_target_properties(SUBSTRATUM PROPERTIES
        OUTPUT_NAME "substratum"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Windows DLL export/import
if (WIN32)
    target_compile_definitions(SUBSTRATUM
            PRIVATE SUBSTRATUM_EXPORTS
            INTERFACE SUBSTRATUM_IMPORTS
    )
endif ()