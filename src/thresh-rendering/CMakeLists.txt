add_library(Flux STATIC
        flux/graphics_utils.hpp
        flux/graphics_utils.cpp
        flux/vkbackend/vk_structs.hpp
        flux/vkbackend/vulkan_instance.cpp
        flux/vkbackend/vulkan_instance.hpp
        flux/vkbackend/vulkan_context.cpp
        flux/vkbackend/vulkan_context.hpp
        flux/graphics_types.hpp
        flux/vkbackend/texture.cpp
        flux/vkbackend/texture.hpp
        flux/vkbackend/vulkan_device.cpp
        flux/vkbackend/vulkan_device.hpp
        flux/vkbackend/vulkan_swapchain.cpp
        flux/vkbackend/vulkan_swapchain.hpp
        flux/vkbackend/vulkan_pipeline.cpp
        flux/math.hpp
        flux/mesh/mesh_primitive.hpp
        flux/render_resources.hpp
        flux/mesh/mesh_primitive.cpp
        flux/dear_im_gui_context.cpp
        flux/dear_im_gui_context.hpp
        flux/debug_line_renderer.cpp
        flux/debug_line_renderer.hpp
)

target_link_libraries(Flux PUBLIC
        Substratum
        Horizon
        Vulkan::Vulkan
        stb
        imgui
        PRIVATE
        glm
)
target_compile_definitions(Flux PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)
target_include_directories(Flux PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-rendering
)

add_slang_shader_target(opaque_mesh SOURCES ${CMAKE_SOURCE_DIR}/assets/shader/opaque_mesh.slang)
add_slang_shader_target(composite SOURCES ${CMAKE_SOURCE_DIR}/assets/shader/composite.slang)
add_slang_shader_target(debug_lines SOURCES ${CMAKE_SOURCE_DIR}/assets/shader/debug_line.slang)
add_dependencies(Flux opaque_mesh composite debug_lines)

