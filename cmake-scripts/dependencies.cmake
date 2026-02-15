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

FetchContent_MakeAvailable(spirv-headers spirv-tools glslang)
FetchContent_MakeAvailable(ser20 glfw vma glm)

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