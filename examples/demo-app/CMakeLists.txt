add_executable(demo
    main.cpp
)

target_link_libraries(demo PUBLIC Substratum)

# Copy DLLs to output directory (Windows)
if (WIN32)
    add_custom_command(TARGET demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:demo>
            $<TARGET_FILE_DIR:demo>
            COMMAND_EXPAND_LISTS
    )
endif ()