function(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set(THRESH_SHADERS_DIR ${CMAKE_SOURCE_DIR}/assets/shader)

    find_program(SLANGC_EXECUTABLE    NAMES slangc    HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
    find_program(SPIRV_VAL_EXECUTABLE NAMES spirv-val HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

    add_custom_command(
            OUTPUT ${THRESH_SHADERS_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${THRESH_SHADERS_DIR}
    )
    add_custom_command (
            OUTPUT  ${THRESH_SHADERS_DIR}/${TARGET}.spv
            COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES}
            -I ${THRESH_SHADERS_DIR}
            -target spirv
            -profile spirv_1_5
            -capability spvDescriptorHeapEXT # Enables the feature, if a shader/pipeline doesn't use it, it's ignored, so safe to enable for ALL compiled shaders
            -spirv-resource-heap-stride ${THRESH_RESOURCE_HEAP_STRIDE}
            -spirv-sampler-heap-stride ${THRESH_SAMPLER_HEAP_STRIDE}
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -depfile ${THRESH_SHADERS_DIR}/${TARGET}.spv.d
            -o ${TARGET}.spv
            # Cook-time gate: reject SPIR-V the SDK validator won't accept.
            COMMAND ${SPIRV_VAL_EXECUTABLE} --target-env vulkan1.4 ${THRESH_SHADERS_DIR}/${TARGET}.spv
            WORKING_DIRECTORY ${THRESH_SHADERS_DIR}
            DEPFILE ${THRESH_SHADERS_DIR}/${TARGET}.spv.d
            DEPENDS ${THRESH_SHADERS_DIR} ${SHADER_SOURCES}
            COMMENT "Cooking Slang shader ${TARGET} (descriptor-heap path)"
            VERBATIM
    )
    add_custom_target (${TARGET} DEPENDS ${THRESH_SHADERS_DIR}/${TARGET}.spv)
endfunction()