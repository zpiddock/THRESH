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
CPMAddPackage(
        NAME physfs
        GIT_REPOSITORY https://github.com/icculus/physfs.git
        GIT_TAG release-3.2.0
        GIT_SHALLOW TRUE
        OPTIONS
            "PHYSFS_BUILD_SHARED OFF"
            "PHYSFS_BUILD_TEST OFF"
            "PHYSFS_BUILD_DOCS OFF"
            "PHYSFS_DISABLE_INSTALL ON"
)

# stb - Single-file public domain libraries (stb_image.h for texture decoding)
# stb is not a CMake project — fetch sources only and wrap in an INTERFACE target below.
CPMAddPackage(
        NAME stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        DOWNLOAD_ONLY YES
)

if(stb_ADDED AND NOT TARGET stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()

# KTX-Software - KTX2 GPU-compressed texture loading + Basis Universal transcoding
CPMAddPackage(
        NAME ktx
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
        GIT_TAG v4.3.2
        GIT_SHALLOW TRUE
        OPTIONS
            "KTX_FEATURE_STATIC_LIBRARY ON"
            "KTX_FEATURE_TESTS OFF"
            "KTX_FEATURE_TOOLS OFF"
            "KTX_FEATURE_GL_UPLOAD OFF"
            "KTX_FEATURE_VK_UPLOAD ON"
            "KTX_FEATURE_DOC OFF"
            "KTX_FEATURE_LOADTEST_APPS OFF"
)

CPMAddPackage("gh:ocornut/imgui@1.92.8#docking")
CPMAddPackage("gh:cedricguillemet/imguizmo#1.10")

if(imgui_ADDED)
    add_library(imgui SHARED
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
            ${ImGuizmo_SOURCE_DIR}/src/ImGuizmo.cpp
    )
    target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
            ${ImGuizmo_SOURCE_DIR}/src
    )
    target_link_libraries(imgui PUBLIC SDL3::SDL3 Vulkan::Vulkan)
endif ()

# Glaze
CPMAddPackage("gh:stephenberry/glaze@7.6.0")