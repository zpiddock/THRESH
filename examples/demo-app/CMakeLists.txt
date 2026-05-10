add_executable(demo
    main.cpp
        game_app.cpp
        game_app.hpp
)

target_compile_features(demo PUBLIC cxx_std_26)
target_link_libraries(demo PUBLIC Thresh)

# Copy DLLs to output directory (Windows)
if (WIN32)
    add_custom_command(TARGET demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:demo>
            $<TARGET_FILE_DIR:demo>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying DDL's to output directory"
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH _clang_bin)
    if (EXISTS "${_clang_bin}/libc++.dll")
        add_custom_command(TARGET demo POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_clang_bin}/libc++.dll"
                $<TARGET_FILE_DIR:demo>
                COMMENT "Copying libc++.dll to output directory"
        )
    endif ()
endif ()