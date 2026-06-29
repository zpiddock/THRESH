add_library(ferret STATIC
        ferret/cook.cpp
        ferret/cook.hpp
        ferret/hash.cpp
        ferret/hash.hpp
        ferret/assimp_import.cpp
        ferret/assimp_import.hpp
        ferret/mesh_build.cpp
        ferret/mesh_build.hpp
        ferret/texture_bake.cpp
        ferret/texture_bake.hpp
)
target_link_libraries(ferret
        PUBLIC
        Asset_Schema
        assimp
        ktx
        CLI11::CLI11
        PRIVATE
        xxhash
        stb
        libzstd_static
        glaze::glaze
)
target_include_directories(ferret PUBLIC ferret
        ${CMAKE_SOURCE_DIR}/tools/ferret
)

add_executable(ferret-cli
        cli/main.cpp
)
target_link_libraries(ferret-cli
        PUBLIC
        ferret
        Substratum
)

# Copy runtime DLLs (assimp, etc.) next to the CLI so it can launch standalone.
if (WIN32)
    add_custom_command(TARGET ferret-cli POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:ferret-cli>
            $<TARGET_FILE_DIR:ferret-cli>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to ferret-cli")
endif ()

add_subdirectory(tests)