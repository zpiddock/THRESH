# Dependencies that only get pulled in if we are building the Thresh Tools

# Assimp, only build what we absolutely require, being FBX and GLTF importers
set(_thresh_build_shared_libs_saved "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
CPMAddPackage(
        NAME Assimp
        GITHUB_REPOSITORY assimp/assimp
        GIT_TAG v6.0.5
        GIT_SHALLOW TRUE
        OPTIONS "ASSIMP_BUILD_TESTS OFF" "ASSIMP_INSTALL OFF" "ASSIMP_BUILD_ASSIMP_TOOLS OFF"
        "ASSIMP_NO_EXPORT ON" "ASSIMP_WARNINGS_AS_ERRORS OFF" "ASSIMP_BUILD_DOCS OFF"
        "ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF"
        "ASSIMP_BUILD_GLTF_IMPORTER ON" "ASSIMP_BUILD_FBX_IMPORTER ON"
        "ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF"
)
set(BUILD_SHARED_LIBS "${_thresh_build_shared_libs_saved}")

# xxHash for blazing fast hashing
CPMAddPackage(
        NAME xxhash
        GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
        GIT_SHALLOW TRUE
        GIT_TAG v0.8.3
        DOWNLOAD_ONLY YES
)

# DocTest
CPMAddPackage("gh:doctest/doctest#v2.5.2")

if(xxhash_ADDED AND NOT TARGET xxhash)
    add_library(xxhash INTERFACE)
    target_include_directories(xxhash INTERFACE ${xxhash_SOURCE_DIR})
endif ()
