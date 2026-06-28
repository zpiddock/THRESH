add_library(ferret STATIC
        ferret/cook.cpp
        ferret/cook.hpp
)
target_link_libraries(ferret
        PUBLIC
        Asset_Schema
        PRIVATE
        assimp::assimp
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