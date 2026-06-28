add_library(ferret STATIC
        ferret/cook.cpp
        ferret/cook.hpp
        ferret/hash.cpp
        ferret/hash.hpp
        ferret/assimp_import.cpp
        ferret/assimp_import.hpp
)
target_link_libraries(ferret
        PUBLIC
        Asset_Schema
        assimp
        PRIVATE
        xxhash
        stb
        ktx
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

add_subdirectory(tests)