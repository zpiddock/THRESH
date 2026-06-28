cmake_minimum_required(VERSION 4.2)
project(THRESH
        VERSION 0.1.0
        LANGUAGES C CXX
        DESCRIPTION "A Modern Game Engine"
)

# Set C++26 standard
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# To remove the 'lib' prefix from all targets defined in the project
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Thresh Build Options
option(THRESH_BUILD_EXAMPLES "Builds the engine examples and samples, currently only the Demo FPS" ON)
option(THRESH_BUILD_TOOLS    "Builds the tooling executables (Ferret Asset CLI)"                   ON)

# Uniform descriptor stride, shared by the shader cook step and the runtime.
# Slang's default (-spirv-*-heap-stride 0) uses per-TYPE descriptor sizes, which makes the
# same slot index map to different byte offsets for images vs buffers, potentially causing an index-vs-offset size bug.
# A uniform stride wastes a few bytes per slot and removes the class.
# 64 covers every descriptor size on current desktop hardware;
# ThreshVkDevice asserts it against the real device properties at startup.
set(THRESH_RESOURCE_HEAP_STRIDE 64 CACHE STRING "byte stride of one resource-heap slot")
set(THRESH_SAMPLER_HEAP_STRIDE  64 CACHE STRING "byte stride of one sampler-heap slot")

message("Thresh Examples: ${THRESH_BUILD_EXAMPLES}")
message("Thresh Tools: ${THRESH_BUILD_TOOLS}")

# Include CPM for Dependency management
include(cmake-scripts/get_cpm.cmake)
# Find Vulkan (Khronos Official Vulkan cmake script)
include(cmake-scripts/FindVulkan.cmake)
# Include dependencies
include(cmake-scripts/dependencies.cmake)
# Tool-only dependencies (Assimp, zlib, etc.). Pulled in here, before the warning
# flags below, so third-party code never inherits our -Werror. Anything added after
# add_compile_options() picks up the strict flags; dependencies must stay above it.
if(THRESH_BUILD_TOOLS)
    include(cmake-scripts/tool_dependencies.cmake)
endif()
# Set up custom commands
include(cmake-scripts/add_slang_shader.cmake)

# Treat warnings as errors. Applies only to targets created below (src/, tools/,
# examples/) -- all dependency add_subdirectory() calls happen above this point.
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_link_options(-static-libgcc -static-libstdc++)
endif ()

# Disable logging for Release and RelWithDebInfo builds
if(CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    add_compile_definitions(SUBSTRATUM_DISABLE_LOGGING)
    message(STATUS "Logging disabled for ${CMAKE_BUILD_TYPE} build")
endif()


add_subdirectory(src)

if(THRESH_BUILD_TOOLS)
    add_subdirectory(tools)
endif ()

if(THRESH_BUILD_EXAMPLES)
    add_subdirectory(examples/demo-app)
endif ()