add_library(Substratum STATIC
        substratum/log.cpp
        substratum/log.hpp
        substratum/result.hpp
        substratum/filesystem/vfs.cpp
        substratum/filesystem/vfs.hpp
)
target_link_libraries(Substratum PUBLIC physfs-static)
target_include_directories(Substratum PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-core
)