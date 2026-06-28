//
// Created by Admin on 28/06/2026.
//

#pragma once
#include <expected>

#include "flecs.h"
#include "flux/render_resource_registry.hpp"
#include "thresh/asset/model_asset.hpp"
#include "thresh/asset/model_io.hpp"

namespace thresh {
    class ModelLoader {

        public:
            explicit ModelLoader(flux::RenderResourceRegistry& registry);

            auto load_model_asset(const std::string& vfs_path) -> std::expected<asset::ModelAsset, asset::DecodeError>;
            auto prefab_for(flecs::world& world, const std::string& vfs_path) -> flecs::entity; // Register once, cached
            auto spawn(flecs::world& world, const std::string& vfs_path, flecs::entity parent) -> flecs::entity; // IsA

        private:
            auto build_prefab(flecs::world& world, const asset::ModelAsset& asset) -> flecs::entity;
            auto upload_mesh(const asset::MeshEntry& asset) -> std::uint32_t;           // registry mesh handle
            auto register_material(const asset::MaterialEntry& asset) -> std::uint32_t; // registry material handle
            auto resolve_texture(const asset::TextureRef& asset) -> std::uint32_t;      // hash -> HeapSlot, cached
            auto default_material() -> std::uint32_t;

            flux::RenderResourceRegistry& m_registry;
            std::uint32_t m_default_material = 0;

            std::unordered_map<std::string, flecs::entity> m_prefab_by_path;
            std::unordered_map<std::uint64_t, std::uint32_t> m_texture_by_hash;
    };
} // thresh
