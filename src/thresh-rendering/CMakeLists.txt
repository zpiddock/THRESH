add_library(Flux STATIC
        flux/graphics_utils.hpp
        flux/graphics_utils.cpp
        flux/vkbackend/vulkan_context.cpp
        flux/vkbackend/vulkan_context.hpp
)

target_link_libraries(Flux PUBLIC
        Substratum
        Horizon
        Vulkan::Vulkan
        glslang
        SPIRV
        glslang-default-resource-limits
)
target_compile_definitions(Flux PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)
target_include_directories(Flux PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-rendering
)

add_slang_shader_target(triangle SOURCES ${CMAKE_SOURCE_DIR}/assets/shader/triangle.slang)
add_dependencies(Flux triangle)

