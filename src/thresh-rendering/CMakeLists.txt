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
        flux/vkbackend/vulkan_pipeline.hpp
)

target_link_libraries(Flux PUBLIC
        Substratum
        Horizon
        Vulkan::Vulkan
        stb
)
target_compile_definitions(Flux PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)
target_include_directories(Flux PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-rendering
)

add_slang_shader_target(triangle SOURCES ${CMAKE_SOURCE_DIR}/assets/shader/triangle.slang)
add_dependencies(Flux triangle)

