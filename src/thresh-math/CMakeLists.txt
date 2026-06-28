add_library(Helix INTERFACE
        helix/math.hpp
        helix/colour.hpp
)
target_link_libraries(Helix INTERFACE glm::glm)
target_include_directories(Helix INTERFACE
        ${CMAKE_SOURCE_DIR}/src/thresh-math
)