//
// Created by Admin on 08/05/2026.
//

#pragma once
#include <string>

#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_swapchain.hpp"

namespace flux {

    enum class BindingModel {
        DESCRIPTOR_HEAP,
        DESCRIPTOR_SET,
    };

    struct PipelineContext {
        // Each slang shader should have all entrypoints in main file
        // Will be loaded from VFS "/shader/{shader_path}"
        std::string shader_path;

        BindingModel binding_model = BindingModel::DESCRIPTOR_HEAP; // By default, we want everything through the Heap

        // If string is empty stage will be omitted
        std::string vertex_entry = "vertexMain";
        std::string fragment_entry = "fragmentMain";
        std::string compute_entry; // future use
        bool use_vertex_input = true;
        bool depth_test = true;
        bool depth_write = true;
        vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;
        vk::Format colour_format = vk::Format::eUndefined;
        vk::Format depth_format = vk::Format::eUndefined;
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;

        // For use with explicit descriptor sets only, will be ignored under descriptor heap pipelines
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        std::vector<vk::PushConstantRange>          push_constants;

        // If empty, ThreshVkPipeline falls back to the hardcoded flux::Vertex layout (existing behaviour).
        std::vector<vk::VertexInputBindingDescription>   vertex_bindings;
        std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
    };

    class ThreshVkPipeline {

        public:
            ThreshVkPipeline(const PipelineContext& context, ThreshVkDevice& device);

            auto descriptor_set_layout() -> const vk::raii::DescriptorSetLayout& {
                return m_descriptor_set_layout;
            }
            auto pipeline_layout() -> const vk::raii::PipelineLayout& {
                return m_pipeline_layout;
            }
            auto graphics_pipeline() -> const vk::raii::Pipeline& {
                return m_graphics_pipeline;
            }

        private:

            auto create_pipeline(const PipelineContext& context, ThreshVkDevice& device) -> void;

            // Shader Functions - TODO: Create Shader Wrapper Class
            auto load_shader(const std::string& shader_path, ThreshVkDevice& device) -> vk::raii::ShaderModule;

            vk::raii::DescriptorSetLayout m_descriptor_set_layout = nullptr;
            vk::raii::PipelineLayout      m_pipeline_layout       = nullptr;
            vk::raii::Pipeline            m_graphics_pipeline     = nullptr;

            constexpr static std::string vertex_main   = "vertexMain";
            constexpr static std::string fragment_main = "fragmentMain";
            constexpr static std::string compute_main  = "computeMain";
    };
} // flux
