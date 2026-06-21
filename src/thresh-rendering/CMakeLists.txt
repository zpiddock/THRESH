set(THRESH_SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/assets/shader/src)

add_library(Flux STATIC
        flux/graphics_utils.hpp
        flux/graphics_utils.cpp
        flux/vkbackend/vk_structs.hpp
        flux/vkbackend/vulkan_instance.cpp
        flux/vkbackend/vulkan_instance.hpp
        flux/vkbackend/vulkan_context.cpp
        flux/vkbackend/vulkan_context.hpp
        flux/graphics_types.hpp
        flux/vkbackend/vulkan_device.cpp
        flux/vkbackend/vulkan_device.hpp
        flux/vkbackend/vulkan_swapchain.cpp
        flux/vkbackend/vulkan_swapchain.hpp
        flux/vkbackend/vulkan_pipeline.cpp
        flux/mesh/mesh_primitive.hpp
        flux/render_resources.hpp
        flux/mesh/mesh_primitive.cpp
        flux/dear_im_gui_context.cpp
        flux/dear_im_gui_context.hpp
        flux/debug_line_renderer.cpp
        flux/debug_line_renderer.hpp
        flux/vkbackend/buffer.cpp
        flux/vkbackend/buffer.hpp
        flux/vkbackend/image.cpp
        flux/vkbackend/image.hpp
        flux/vkbackend/command_buffer.cpp
        flux/vkbackend/command_buffer.hpp
        flux/gpu_data.hpp
        flux/vkbackend/frame_context.cpp
        flux/vkbackend/frame_context.hpp
        flux/vkbackend/descriptor_heap.cpp
        flux/vkbackend/descriptor_heap.hpp
        flux/vkbackend/descriptor_handle.hpp
        flux/material_buffer.cpp
        flux/material_buffer.hpp
        flux/render_config.hpp
)

target_link_libraries(Flux PUBLIC
        Substratum
        Horizon
        Helix
        Vulkan::Vulkan
        stb
        imgui
)
target_compile_definitions(Flux PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
        THRESH_RESOURCE_HEAP_STRIDE=${THRESH_RESOURCE_HEAP_STRIDE}
        THRESH_SAMPLER_HEAP_STRIDE=${THRESH_SAMPLER_HEAP_STRIDE}
)
target_include_directories(Flux PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-rendering
)

# Spike test the descriptor heap in pinned Vulkan SDK / Slang Compiler Version
add_slang_shader_target(spike_heap SOURCES ${CMAKE_SOURCE_DIR}/assets/shader/spike/spike_heap.slang)
add_slang_shader_target(spike_material SOURCES ${CMAKE_SOURCE_DIR}/assets/shader/spike/spike_material.slang)

add_slang_shader_target(opaque_mesh SOURCES ${THRESH_SHADER_SOURCE_DIR}/opaque_mesh.slang)
add_slang_shader_target(composite SOURCES ${THRESH_SHADER_SOURCE_DIR}/composite.slang)
add_slang_shader_target(debug_lines SOURCES ${THRESH_SHADER_SOURCE_DIR}/debug_line.slang)
add_dependencies(Flux spike_heap spike_material opaque_mesh composite debug_lines)

