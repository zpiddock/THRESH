add_library(Helix STATIC
        helix/math.hpp
)
target_link_libraries(Helix PRIVATE glm::glm)
target_include_directories(Helix PUBLIC
        ${CMAKE_SOURCE_DIR}/src/thresh-math
)