include(FetchContent)

# Set FetchContent options
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)

# SPIRV-Headers - Required by SPIRV-Tools
FetchContent_Declare(
        spirv-headers
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
        GIT_TAG vulkan-sdk-1.4.341.0
        GIT_SHALLOW TRUE
)

# SPIRV-Tools - Required by GLSLang for optimization
FetchContent_Declare(
        spirv-tools
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
        GIT_TAG v2026.1
        GIT_SHALLOW TRUE
)

set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)

# GLSLang - GLSL to SPIR-V compiler (built from source for ABI compatibility)
FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
        GIT_TAG 16.2.0
        GIT_SHALLOW TRUE
)

# Configure glslang build options
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_JS OFF CACHE BOOL "" FORCE)
set(ENABLE_RTTI ON CACHE BOOL "" FORCE)
set(ENABLE_EXCEPTIONS ON CACHE BOOL "" FORCE)
set(ENABLE_OPT ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)


# Ser20 Serialisation Library
FetchContent_Declare(
        ser20
        GIT_REPOSITORY https://github.com/royjacobson/ser20
        GIT_TAG v0.9.1
        GIT_SHALLOW TRUE
)

# GLFW - Windowing library
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE SHARED CACHE STRING "" FORCE)
FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
        FIND_PACKAGE_ARGS 3.4
)

# VulkanMemoryAllocator
FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG v3.3.0
        GIT_SHALLOW TRUE
)

# GLM - Mathematics library
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW TRUE
)

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
set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        ktx
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
        GIT_TAG v4.3.2
        GIT_SHALLOW TRUE
)

# cgltf - Single-header glTF 2.0 parser
FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG v1.14
        GIT_SHALLOW TRUE
)

# entt - Entity Component System (header-only)
FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.14.0
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(spirv-headers spirv-tools glslang)
FetchContent_MakeAvailable(ser20 glfw vma glm physfs stb ktx cgltf entt)

# stb is not a CMake project — create an INTERFACE target for include path
if(NOT TARGET stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()

# cgltf is not a CMake project — create an INTERFACE target for include path
if(NOT TARGET cgltf)
    add_library(cgltf INTERFACE)
    target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})
endif()

# Disable warnings for third-party libraries
if(TARGET glfw)
    target_compile_options(glfw PRIVATE -w)
endif()
# VMA is header-only, so we disable warnings via INTERFACE
if(TARGET VulkanMemoryAllocator)
    target_compile_options(VulkanMemoryAllocator INTERFACE -w)
endif()
# Disable warnings-as-errors for third-party libraries
if(TARGET glslang)
    target_compile_options(glslang PRIVATE -Wno-error)
endif()
if(TARGET SPIRV)
    target_compile_options(SPIRV PRIVATE -w)
endif()
if(TARGET SPIRV-Tools-static)
    target_compile_options(SPIRV-Tools-static PRIVATE -w)
endif()
if(TARGET SPIRV-Tools-opt)
    target_compile_options(SPIRV-Tools-opt PRIVATE -w)
endif()
if(TARGET physfs-static)
    target_compile_options(physfs-static PRIVATE -w)
endif()
if(TARGET ktx)
    target_compile_options(ktx PRIVATE -w)
endif()
if(TARGET ktx_read)
    target_compile_options(ktx_read PRIVATE -w)
endif()
if(TARGET obj_basisu_cbind)
    target_compile_options(obj_basisu_cbind PRIVATE -w)
endif()
if(TARGET objUtil)
    target_compile_options(objUtil PRIVATE -w)
endif()