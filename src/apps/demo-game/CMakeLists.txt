# Demo Game
add_executable(game
        src/main.cpp
)

target_include_directories(game
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(game PUBLIC cxx_std_26)

target_link_libraries(game PUBLIC THRESH)

# Set output name and versioning
set_target_properties(game PROPERTIES
        OUTPUT_NAME "game"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Copy shaders to output directory
add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:game>/shaders
)

# Copy DLLs to output directory (Windows)
if (WIN32)
    add_custom_command(TARGET game POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:game>
            $<TARGET_FILE_DIR:game>
            COMMAND_EXPAND_LISTS
    )
endif ()