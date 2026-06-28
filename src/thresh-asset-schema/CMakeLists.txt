add_library(Asset_Schema INTERFACE
        thresh/asset/material_asset.h
        thresh/asset/model_io.hpp
)

target_link_libraries(Asset_Schema INTERFACE glaze::glaze)
target_include_directories(Asset_Schema INTERFACE
        ${CMAKE_SOURCE_DIR}/src/thresh-asset-schema
)