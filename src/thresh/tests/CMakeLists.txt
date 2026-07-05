add_executable(thresh-tests test-scene-serializer.cpp)
target_link_libraries(thresh-tests PRIVATE Thresh doctest::doctest)
add_test(NAME thresh-tests COMMAND thresh-tests)

# Same runtime-DLL staging as the demo app; the test links the full engine.
if (WIN32)
    add_custom_command(TARGET thresh-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:thresh-tests>
            $<TARGET_FILE_DIR:thresh-tests>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying DLL's to output directory"
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH _clang_bin)
    if (EXISTS "${_clang_bin}/libc++.dll")
        add_custom_command(TARGET thresh-tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_clang_bin}/libc++.dll"
                $<TARGET_FILE_DIR:thresh-tests>
                COMMENT "Copying libc++.dll to output directory"
        )
    endif ()
endif ()

if (UNIX AND NOT APPLE)
    target_link_options(thresh-tests PRIVATE "-Wl,-rpath,$ORIGIN")
    add_custom_command(TARGET thresh-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Thresh>
            $<TARGET_FILE:flecs>
            $<TARGET_SONAME_FILE:SDL3::SDL3>
            $<TARGET_FILE:imgui>
            $<TARGET_FILE_DIR:thresh-tests>
            COMMENT "Copying shared libraries to output directory"
    )
endif ()
