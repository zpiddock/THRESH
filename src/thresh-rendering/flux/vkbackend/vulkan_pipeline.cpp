//
// Created by Admin on 08/05/2026.
//

#include "vulkan_pipeline.hpp"

#include "flux/graphics_types.hpp"
#include "substratum/filesystem/vfs.hpp"
#include "substratum/log.hpp"

namespace flux {
    ThreshVkPipeline::ThreshVkPipeline(const PipelineContext& context, ThreshVkDevice& device) {
        create_pipeline(context, device);
    }

    auto ThreshVkPipeline::create_pipeline(const PipelineContext& context, ThreshVkDevice& device) -> void {

        const bool heap = context.binding_model == BindingModel::DESCRIPTOR_HEAP;

        SUB_DEBUG("Creating pipeline (shader='{}', model={}, bindings={}, vertex_input={}, depth_test={})",
                  context.shader_path, heap ? "descriptor_heap" : "descriptor_set",
                  context.bindings.size(), context.use_vertex_input, context.depth_test);

        // Layout: explicit_sets builds a set layout + pipeline layout; the heap path has NEITHER.
        // VU: with the heap flag, layout MUST be VK_NULL_HANDLE, and the SPIR-V must carry no
        // DescriptorSet/Binding decorations (the cook step guarantees that, so no mapping struct
        // is needed). Both members stay null under heap; pipeline_layout() asserts if called.
        if (!heap) {
            const vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_info{
                .bindingCount = static_cast<uint32_t>(context.bindings.size()),
                .pBindings    = context.bindings.data(),
            };
            m_descriptor_set_layout = vk::raii::DescriptorSetLayout(device.logical(), descriptor_set_layout_info);

        assert(substratum::VFS::is_initialized());

        const auto shader_module = load_shader(std::format("/shader/{}", context.shader_path), device);

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        shader_stages.reserve(3);

        auto append_stage = [&](const std::string& entry, const vk::ShaderStageFlagBits stage) {
            if (entry.empty()) { return; }
            shader_stages.push_back({ .stage = stage, .module = shader_module, .pName = entry.c_str() });
        };
        append_stage(context.vertex_entry,   vk::ShaderStageFlagBits::eVertex);
        append_stage(context.fragment_entry, vk::ShaderStageFlagBits::eFragment);
        append_stage(context.compute_entry,  vk::ShaderStageFlagBits::eCompute);

        auto default_bindings   = Vertex::get_binding_description();
        auto default_attributes = Vertex::get_attribute_descriptions();

        vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
        if (context.use_vertex_input) {
            if (!context.vertex_bindings.empty()) {
                vertex_input_info.vertexBindingDescriptionCount   = static_cast<uint32_t>(context.vertex_bindings.size());
                vertex_input_info.pVertexBindingDescriptions      = context.vertex_bindings.data();
                vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(context.vertex_attributes.size());
                vertex_input_info.pVertexAttributeDescriptions    = context.vertex_attributes.data();
            } else {
                vertex_input_info.vertexBindingDescriptionCount   = 1;
                vertex_input_info.pVertexBindingDescriptions      = &default_bindings;
                vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(default_attributes.size());
                vertex_input_info.pVertexAttributeDescriptions    = default_attributes.data();
            }
        }

        const vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{ .topology = context.topology };
        const vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

        const vk::PipelineRasterizationStateCreateInfo rasterization_info{
            .depthClampEnable        = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode             = vk::PolygonMode::eFill,
            .cullMode                = context.cull_mode,
            .frontFace               = vk::FrontFace::eCounterClockwise,
            .depthBiasEnable         = vk::False,
            .lineWidth               = 1.0f,
        };

        const vk::PipelineMultisampleStateCreateInfo multisampling_info{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable  = vk::False,
        };

        const vk::PipelineDepthStencilStateCreateInfo depth_stencil_info{
            .depthTestEnable       = context.depth_test  ? vk::True : vk::False,
            .depthWriteEnable      = context.depth_write ? vk::True : vk::False,
            .depthCompareOp        = vk::CompareOp::eLess,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable     = vk::False,
        };

        const vk::PipelineColorBlendAttachmentState blend_attachment{
            .blendEnable    = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
        const vk::PipelineColorBlendStateCreateInfo color_blending_info{
            .logicOpEnable   = vk::False,
            .logicOp         = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments    = &blend_attachment,
        };

        const std::vector dynamic_states = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        const vk::PipelineDynamicStateCreateInfo dynamic_state_info{
            .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
            .pDynamicStates    = dynamic_states.data(),
        };

        // The heap flag is 64-bit (VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT = 0x10'0000'0000),
        // so it can't fit GraphicsPipelineCreateInfo::flags (32-bit) — it rides
        // PipelineCreateFlags2CreateInfo, added as a third StructureChain link. With flags = {}
        // on the explicit path it's a no-op, so there's a single code path, not two chains.
        vk::StructureChain<
            vk::GraphicsPipelineCreateInfo,
            vk::PipelineRenderingCreateInfo,
            vk::PipelineCreateFlags2CreateInfo> pipeline_create_info_chain = {
            {
                .stageCount          = static_cast<std::uint32_t>(shader_stages.size()),
                .pStages             = shader_stages.data(),
                .pVertexInputState   = &vertex_input_info,
                .pInputAssemblyState = &input_assembly_info,
                .pViewportState      = &viewportState,
                .pRasterizationState = &rasterization_info,
                .pMultisampleState   = &multisampling_info,
                .pDepthStencilState  = context.depth_test ? &depth_stencil_info : nullptr,
                .pColorBlendState    = &color_blending_info,
                .pDynamicState       = &dynamic_state_info,
                .layout              = heap ? nullptr : *m_pipeline_layout,   // null under heap (VU)
                .renderPass          = nullptr,
            },
            {
                .colorAttachmentCount    = 1,
                .pColorAttachmentFormats = &context.colour_format,
                .depthAttachmentFormat   = context.depth_format,
            },
            {
                .flags = heap ? vk::PipelineCreateFlagBits2::eDescriptorHeapEXT
                              : vk::PipelineCreateFlags2{},
            },
        };

        m_graphics_pipeline = device.logical().createGraphicsPipeline(
            nullptr, pipeline_create_info_chain.get<vk::GraphicsPipelineCreateInfo>());

        SUB_TRACE("Pipeline '{}' created ({} stages, {})", context.shader_path, shader_stages.size(),
                  heap ? "heap" : "sets");
    }

    auto ThreshVkPipeline::load_shader(const std::string& shader_path, ThreshVkDevice& device) -> vk::raii::ShaderModule {

        SUB_TRACE("Loading shader module '{}'", shader_path);
        const auto shader_code = substratum::VFS::read_file(shader_path);

        vk::ShaderModuleCreateInfo shader_module_info{
            .codeSize = shader_code.size() * sizeof(uint8_t),
            .pCode    = reinterpret_cast<const uint32_t*>(shader_code.data())
        };

        return {device.logical(), shader_module_info};
    }
} // flux