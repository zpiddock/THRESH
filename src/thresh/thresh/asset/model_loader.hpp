//
// Created by Admin on 28/06/2026.
//

#pragma once
#include <expected>

#include "flecs.h"
#include "flux/render_resource_registry.hpp"
#include "thresh/asset/model_asset.hpp"
#include "thresh/asset/model_io.hpp"
#include "thresh/scene/ecs_types.hpp"

namespace thresh {
    class ModelLoader {

        public:
            explicit ModelLoader(flux::RenderResourceRegistry& registry);

            struct LoadedModel {
                std::uint32_t mesh_handle = 0;      // ONE merged VBO/IBO in the registry
                std::vector<SubmeshDraw> submeshes; // materials resolved to registry handles
                std::string name;
            };

            auto load_model_asset(const std::string& vfs_path) -> std::expected<asset::ModelAsset, asset::DecodeError>;
            auto get_or_load(const std::string& vfs_path) -> const LoadedModel*; // decode+upload once, cached
            auto spawn(flecs::world& world, const std::string& vfs_path, flecs::entity parent) -> flecs::entity;

        private:
            auto register_material(const asset::MaterialEntry& entry, const std::string& model_name) -> std::uint32_t;
            auto resolve_texture(const asset::TextureRef& ref, const std::string& model_name) -> std::uint32_t; // hash -> HeapSlot, cached
            auto default_material() -> std::uint32_t;

            flux::RenderResourceRegistry& m_registry;
            std::uint32_t m_default_material = 0;

            std::unordered_map<std::string, LoadedModel> m_models; // vfs path -> uploaded model
            std::unordered_map<std::uint64_t, std::uint32_t> m_texture_by_hash;
    };
} // thresh
