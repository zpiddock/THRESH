function(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set(THRESH_SHADERS_DIR ${CMAKE_SOURCE_DIR}/assets/shader)
    set(THRESH_SHADER_ENTRY_POINTS -entry vertexMain -entry fragmentMain)

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
            COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${THRESH_SHADER_ENTRY_POINTS} -o ${TARGET}.spv
            WORKING_DIRECTORY ${THRESH_SHADERS_DIR}
            DEPENDS ${THRESH_SHADERS_DIR} ${SHADER_SOURCES}
            COMMENT "Compiling Slang Shaders"
            VERBATIM
    )
    add_custom_target (${TARGET} DEPENDS ${THRESH_SHADERS_DIR}/${TARGET}.spv)
endfunction()