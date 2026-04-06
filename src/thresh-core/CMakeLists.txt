add_library(Substratum STATIC
        substratum/log.cpp
        substratum/log.hpp
)

target_include_directories(Substratum PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-core
)