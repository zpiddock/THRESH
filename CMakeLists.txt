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

# Include dependencies
include(cmake-scripts/dependencies.cmake)

# Treat warnings as errors
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


add_subdirectory(src/thresh-core)
add_subdirectory(src/thresh-windowing)

add_subdirectory(src/thresh)

add_subdirectory(examples/demo-app)