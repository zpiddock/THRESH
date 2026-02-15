cmake_minimum_required(VERSION 4.2)
project(THRESH
        VERSION 0.1.0
        LANGUAGES CXX
        DESCRIPTION "A Modern Vulkan-based Game Engine"
)

# Set C++26 standard
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# To remove the 'lib' prefix from all targets defined in the project
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Treat warnings as errors
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter)
endif()

# Disable logging for Release and RelWithDebInfo builds
if(CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    add_compile_definitions(SUBSTRATUM_DISABLE_LOGGING)
    message(STATUS "Logging disabled for ${CMAKE_BUILD_TYPE} build")
endif()

# Include dependencies
include(cmake-scripts/dependencies.cmake)

message(STATUS "Vulkan include: ${Vulkan_INCLUDE_DIRS}")

add_subdirectory(src/engine/libs/substratum)
add_subdirectory(src/engine/libs/horizon)
add_subdirectory(src/engine/libs/flux)

add_subdirectory(src/engine/thresh)