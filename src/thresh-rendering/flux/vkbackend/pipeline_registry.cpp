//
// Created by Admin on 21/06/2026.
//

#include "pipeline_registry.hpp"

#include "substratum/log.hpp"

namespace flux {
    auto PipelineRegistry::register_pipeline(const std::string& name, const PipelineContext& ctx) -> ThreshVkPipeline* {

        SUB_DEBUG("Registering pipeline '{}'", name);
        auto pipeline = std::make_unique<ThreshVkPipeline>(ctx, m_device);

        auto* raw = pipeline.get();
        auto [it, inserted] = m_pipelines.emplace(name, std::move(pipeline));
        if (!inserted) {
            SUB_WARN("Pipeline with name {} already exists, ignoring.", name);
            return it->second.get();
        }
        return raw;
    }

    auto PipelineRegistry::get(const std::string& name) -> ThreshVkPipeline* {
        const auto it = m_pipelines.find(name);
        if (it == m_pipelines.end()) {
            SUB_FATAL("Pipeline with name {} not found.", name);
        }
        return it->second.get();
    }

    auto PipelineRegistry::register_geometry_pipeline(const vk::Format colour, const vk::Format depth) -> void {

        register_pipeline("opaque_mesh", PipelineContext{
            .shader_path = "opaque_mesh",
            .binding_model = BindingModel::DESCRIPTOR_HEAP,
            .use_vertex_input = true,
            .depth_test = true,
            .cull_mode = vk::CullModeFlagBits::eBack,
            .colour_format = colour,
            .depth_format = depth
        });
    }

    auto PipelineRegistry::register_composite_pipeline(const vk::Format swapchain_colour) -> void {

        register_pipeline("composite", PipelineContext{
            .shader_path = "composite",
            .binding_model = BindingModel::DESCRIPTOR_HEAP,
            .use_vertex_input = false,
            .depth_test = false,
            .cull_mode = vk::CullModeFlagBits::eNone,
            .colour_format = swapchain_colour,
            .depth_format = vk::Format::eUndefined
        });
    }
} // flux