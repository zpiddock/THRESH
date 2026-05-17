//
// Created by shad0w on 17/05/2026.
//

#include "assets_loader.h"

#include "../../../thresh-asset-schema/asset/material_asset.h"
#include "substratum/log.hpp"

#include <glaze/glaze.hpp>

#include "substratum/filesystem/vfs.hpp"

namespace thresh {

    AssetsLoader::AssetsLoader(flux::GraphicsUtils &gfx) : m_graphics(gfx) {}

    auto AssetsLoader::load_texture(const std::string &path) -> uint32_t {

        if (m_texture_cache.contains(path)) {
            return m_texture_cache.at(path);
        } else {
            auto texture = m_graphics.register_texture(path);
            m_texture_cache.insert(std::make_pair(path, texture));
            return texture;
        }
    }

    auto AssetsLoader::load_material(const std::string &path) -> uint32_t {

        if (m_material_cache.contains(path)) {
            return m_material_cache.at(path);
        }

        auto contents = substratum::VFS::read_file_string(path);
        auto asset = glz::read_json<thresh::asset::MaterialAsset>(contents);

        if (!asset.has_value()) {

          const auto error = asset.error();
          SUB_WARN("Error processing material {}\n"
                   "Error Code {}"
                   "Error message: {}",
                   path, glz::format_error(error.ec),
                   error.custom_error_message);
        }

        auto albedo_handle = load_texture(asset->albedo_path);
        auto albedo_tint = asset->albedo_tint;
        auto material_handle = m_graphics.register_material(
            albedo_handle,
            flux::float4{albedo_tint[0], albedo_tint[1], albedo_tint[2], 1.f});
        m_material_cache.insert(std::make_pair(path, material_handle));
        return material_handle;
    }
} // thresh