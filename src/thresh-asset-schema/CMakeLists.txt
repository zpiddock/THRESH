add_library(Asset_Schema INTERFACE
        thresh/asset/material_asset.h
        thresh/asset/material_flags.hpp
        thresh/asset/model_io.hpp
        thresh/asset/model_asset.hpp
        thresh/asset/helix_glaze.hpp
)

target_link_libraries(Asset_Schema INTERFACE Helix glaze::glaze)
target_include_directories(Asset_Schema INTERFACE
        ${CMAKE_SOURCE_DIR}/src/thresh-asset-schema
)