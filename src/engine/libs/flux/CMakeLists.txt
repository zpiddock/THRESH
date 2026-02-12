# Flux - Vulkan Abstractions
# Provides modern RAII wrappers around Vulkan resources

add_library(FLUX SHARED
        src/instance.cpp
        src/surface.cpp
        src/device.cpp
        src/buffer.cpp
        src/descriptors.cpp
        src/swapchain.cpp
        src/shader.cpp
        src/shader_compiler.cpp
        src/shader_cache.cpp
        src/pipeline.cpp
        src/command_buffer.cpp
        src/render_graph_resource.cpp
        src/barrier_batcher.cpp
        src/transient_allocator.cpp
        src/render_graph_pass.cpp
        src/image.cpp
        src/sampler.cpp
        src/image_utils.cpp
        src/sync.cpp
        src/deletion_queue.cpp
)

target_include_directories(FLUX
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(FLUX PUBLIC cxx_std_26)

target_link_libraries(FLUX
        PUBLIC
        SUBSTRATUM
        HORIZON
        Vulkan::Vulkan
        VulkanMemoryAllocator
        PRIVATE
        glslang
        SPIRV
)

# Set output name and versioning
set_target_properties(FLUX PROPERTIES
        OUTPUT_NAME "flux"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Windows DLL export/import
if (WIN32)
    target_compile_definitions(FLUX
            PRIVATE FLUX_EXPORTS
            INTERFACE FLUX_IMPORTS
    )
endif ()
