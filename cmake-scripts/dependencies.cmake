include(FetchContent)

# Set FetchContent options
set(FETCHCONTENT_QUIET OFF)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)

# SDL3 - Windowing & Controller Support
# SDL3 must be shared — static SDL3 embedded in a DLL violates Windows DLL init rules
CPMAddPackage(
        NAME sdl
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.8
        GIT_SHALLOW TRUE
        OPTIONS "SDL_SHARED ON" "SDL_STATIC OFF" "SDL_WERROR OFF"
        SYSTEM
)

# GLM - Mathematics library
CPMAddPackage("gh:g-truc/glm#1.0.3")

# Flecs ECS
CPMAddPackage("gh:SanderMertens/flecs#master")

# PhysicsFS - Virtual filesystem abstraction (mount folders or archives)
# PhysicsFS 3.2.0 uses cmake_minimum_required(VERSION 2.8.12) which CMake 4.x rejects.
# Allow it via the compatibility policy variable.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(PHYSFS_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
        physfs
        GIT_REPOSITORY https://github.com/icculus/physfs.git
        GIT_TAG release-3.2.0
        GIT_SHALLOW TRUE
)

# stb - Single-file public domain libraries (stb_image.h for texture decoding)
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)

# KTX-Software - KTX2 GPU-compressed texture loading + Basis Universal transcoding
set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        ktx
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
        GIT_TAG v4.3.2
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(physfs stb ktx)

# stb is not a CMake project — create an INTERFACE target for includ path
if(NOT TARGET stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()
