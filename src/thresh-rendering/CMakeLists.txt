add_library(Flux STATIC
        flux.hpp
        flux.cpp
)

target_link_libraries(Flux PUBLIC
        Substratum
        Horizon
        Vulkan::Vulkan
        glslang
        SPIRV
        glslang-default-resource-limits
)

target_include_directories(Flux PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-rendering
)
