add_library(Substratum SHARED
        log.cpp
        log.hpp
)

target_include_directories(Substratum PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)