add_library(Asset_Schema INTERFACE
        asset/material_asset.h
)

#target_link_libraries(Asset_Schema INTERFACE glaze::glaze)
#target_include_directories(Asset_Schema PUBLIC
#        ${CMAKE_SOURCE_DIR}/src/thresh-asset-schema
#)