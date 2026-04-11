add_library(Thresh SHARED
        thresh/engine.cpp
        thresh/engine.hpp
        thresh/application.cpp
        thresh/application.hpp
)

target_link_libraries(Thresh PUBLIC Substratum Horizon Flux)

target_include_directories(Thresh PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh
)