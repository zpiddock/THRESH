function(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set(THRESH_SHADERS_DIR ${CMAKE_SOURCE_DIR}/assets/shader)

    find_program(SLANGC_EXECUTABLE
            NAMES slangc
            HINTS $ENV{VULKAN_SDK}/bin
            REQUIRED
    )

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
                -emit-spirv-directly
                -fvk-use-entrypoint-name
                -depfile ${THRESH_SHADERS_DIR}/${TARGET}.spv.d
                -o ${TARGET}.spv
            WORKING_DIRECTORY ${THRESH_SHADERS_DIR}
            DEPFILE ${THRESH_SHADERS_DIR}/${TARGET}.spv.d
            DEPENDS ${THRESH_SHADERS_DIR} ${SHADER_SOURCES}
            COMMENT "Compiling Slang Shaders"
            VERBATIM
    )
    add_custom_target (${TARGET} DEPENDS ${THRESH_SHADERS_DIR}/${TARGET}.spv)
endfunction()