//
// Created by Admin on 08/05/2026.
//

#include "vulkan_pipeline.hpp"

#include "flux/graphics_types.hpp"
#include "substratum/filesystem/vfs.hpp"

namespace flux {
    ThreshVkPipeline::ThreshVkPipeline(const PipelineContext& context, ThreshVkDevice& device) {
        create_pipeline(context, device);
    }

    // DEPRECATED
    ThreshVkPipeline::ThreshVkPipeline(const std::string& shader_path,
                                       ThreshVkDevice& device, ThreshVkSwapchain& swapchain) {

        create_descriptor_set_layouts(device);
        create_graphics_pipelines(shader_path, device, swapchain);
    }

    auto ThreshVkPipeline::create_pipeline(const PipelineContext& context, ThreshVkDevice& device) -> void {

        const vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_info{
            .bindingCount = static_cast<uint32_t>(context.bindings.size()),
            .pBindings    = context.bindings.data()
        };
        m_descriptor_set_layout = vk::raii::DescriptorSetLayout(device.logical(), descriptor_set_layout_info);

        assert(substratum::VFS::is_initialized());

        const auto shader_module = load_shader(std::format("/shader/{}", context.shader_path), device);

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        shader_stages.reserve(3);

        auto append_stage = [&](const std::string& shader_path, const vk::ShaderStageFlagBits stage) {
            if (shader_path.empty()) { return; }
            shader_stages.push_back({
                .stage  = stage,
                .module = shader_module,
                .pName  = shader_path.c_str()
            });
        };

        append_stage(context.vertex_entry, vk::ShaderStageFlagBits::eVertex);
        append_stage(context.fragment_entry, vk::ShaderStageFlagBits::eFragment);
        append_stage(context.compute_entry, vk::ShaderStageFlagBits::eCompute);

        vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
        if (context.use_vertex_input) {
            const auto binding_description = Vertex::get_binding_description();
            const auto attribute_descriptions = Vertex::get_attribute_descriptions();

            vertex_input_info.vertexBindingDescriptionCount = 1;
            vertex_input_info.pVertexBindingDescriptions    = &binding_description;
            vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
            vertex_input_info.pVertexAttributeDescriptions   = attribute_descriptions.data();
        }

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{
            .topology = vk::PrimitiveTopology::eTriangleList,
        };
        vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

        vk::PipelineRasterizationStateCreateInfo rasterization_info{
            .depthClampEnable        = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode             = vk::PolygonMode::eFill,
            .cullMode                = context.cull_mode,
            .frontFace               = vk::FrontFace::eCounterClockwise,
            .depthBiasEnable         = vk::False,
            .lineWidth               = 1.0f,
        };

        vk::PipelineMultisampleStateCreateInfo multisampling_info{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable  = vk::False,
        };

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_info {
            .depthTestEnable       = context.depth_test? vk::True : vk::False,
            .depthWriteEnable      = context.depth_test? vk::True : vk::False,
            .depthCompareOp        = vk::CompareOp::eLess,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable     = vk::False,
        };

        vk::PipelineColorBlendAttachmentState blend_attachment{
            .blendEnable    = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
        vk::PipelineColorBlendStateCreateInfo color_blending_info{
            .logicOpEnable   = vk::False,
            .logicOp         = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments    = &blend_attachment,
        };

        std::vector      dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamic_state_info{
            .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
            .pDynamicStates    = dynamic_states.data()
        };

        vk::PipelineLayoutCreateInfo pipeline_layout_info{
            .setLayoutCount         = context.bindings.empty() ? 0u : 1u,
            .pSetLayouts            = context.bindings.empty() ? nullptr : &*m_descriptor_set_layout,
            .pushConstantRangeCount = static_cast<std::uint32_t>(context.push_constants.size()),
            .pPushConstantRanges    = context.push_constants.empty() ? nullptr : context.push_constants.data()
        };

        m_pipeline_layout = vk::raii::PipelineLayout(device.logical(), pipeline_layout_info);

        vk::StructureChain<
            vk::GraphicsPipelineCreateInfo,
            vk::PipelineRenderingCreateInfo> pipeline_create_info_chain = {
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
                .layout              = m_pipeline_layout,
                .renderPass          = nullptr
            },
            {
                .colorAttachmentCount    = 1,
                .pColorAttachmentFormats = &context.colour_format,
                .depthAttachmentFormat = context.depth_format
            }
            };

        m_graphics_pipeline = device.logical().createGraphicsPipeline(nullptr,
                                                              pipeline_create_info_chain.get<
                                                                  vk::GraphicsPipelineCreateInfo>());
    }

    auto ThreshVkPipeline::create_descriptor_set_layouts(ThreshVkDevice& device) -> void {

        std::array bindings = {
            vk::DescriptorSetLayoutBinding{
                .binding           = 0,
                .descriptorType    = vk::DescriptorType::eUniformBuffer,
                .descriptorCount    = 1,
                .stageFlags         = vk::ShaderStageFlagBits::eVertex,
                .pImmutableSamplers = nullptr
            },
            vk::DescriptorSetLayoutBinding{
                .binding           = 1,
                .descriptorType    = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount    = 1,
                .stageFlags         = vk::ShaderStageFlagBits::eFragment,
                .pImmutableSamplers = nullptr
            }
        };
        vk::DescriptorSetLayoutCreateInfo layout_info {

            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings    = bindings.data(),
        };
        m_descriptor_set_layout = vk::raii::DescriptorSetLayout(device.logical(), layout_info);
    }

    auto ThreshVkPipeline::create_graphics_pipelines(const std::string& shader_path, ThreshVkDevice& device, ThreshVkSwapchain& swapchain) -> void {

        assert(substratum::VFS::is_initialized());

        auto triangle_shader_module = load_shader(std::format("/shader/{}", shader_path), device);
        const vk::PipelineShaderStageCreateInfo vert_stage_info{
            .stage  = vk::ShaderStageFlagBits::eVertex,
            .module = triangle_shader_module,
            .pName  = vertex_main.c_str()
        };
        const vk::PipelineShaderStageCreateInfo frag_stage_info{
            .stage  = vk::ShaderStageFlagBits::eFragment,
            .module = triangle_shader_module,
            .pName  = fragment_main.c_str()
        };
        vk::PipelineShaderStageCreateInfo        shader_stages[] = {vert_stage_info, frag_stage_info};
        auto binding_description = Vertex::get_binding_description();
        auto attribute_descriptions = Vertex::get_attribute_descriptions();
        vk::PipelineVertexInputStateCreateInfo   vertex_input_info{
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions    = &binding_description,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
            .pVertexAttributeDescriptions   = attribute_descriptions.data()
        };
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{
            .topology = vk::PrimitiveTopology::eTriangleList,
        };
        vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

        vk::PipelineRasterizationStateCreateInfo rasterization_info{
            .depthClampEnable        = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode             = vk::PolygonMode::eFill,
            .cullMode                = vk::CullModeFlagBits::eBack,
            .frontFace               = vk::FrontFace::eCounterClockwise,
            .depthBiasEnable         = vk::False,
            .lineWidth               = 1.0f,
        };

        vk::PipelineMultisampleStateCreateInfo multisampling_info{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable  = vk::False,
        };

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_info {
            .depthTestEnable          = vk::True,
            .depthWriteEnable         = vk::True,
            .depthCompareOp           = vk::CompareOp::eLess,
            .depthBoundsTestEnable    = vk::False,
            .stencilTestEnable        = vk::False,
        };

        vk::PipelineColorBlendAttachmentState blend_attachment{
            .blendEnable    = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
        vk::PipelineColorBlendStateCreateInfo color_blending_info{
            .logicOpEnable   = vk::False,
            .logicOp         = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments    = &blend_attachment,
        };

        std::vector<vk::DynamicState>      dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamic_state_info{
            .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
            .pDynamicStates    = dynamic_states.data()
        };

        vk::PushConstantRange push_range {
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            .offset     = 0,
            .size       = sizeof(PushConstants)
        };

        vk::PipelineLayoutCreateInfo pipeline_layout_info{
            .setLayoutCount         = 1,
            .pSetLayouts            = &*m_descriptor_set_layout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges    = &push_range
        };

        m_pipeline_layout = vk::raii::PipelineLayout(device.logical(), pipeline_layout_info);

        vk::StructureChain<
            vk::GraphicsPipelineCreateInfo,
            vk::PipelineRenderingCreateInfo> pipeline_create_info_chain = {
            {
                .stageCount          = 2,
                .pStages             = shader_stages,
                .pVertexInputState   = &vertex_input_info,
                .pInputAssemblyState = &input_assembly_info,
                .pViewportState      = &viewportState,
                .pRasterizationState = &rasterization_info,
                .pMultisampleState   = &multisampling_info,
                .pDepthStencilState  = &depth_stencil_info,
                .pColorBlendState    = &color_blending_info,
                .pDynamicState       = &dynamic_state_info,
                .layout              = m_pipeline_layout,
                .renderPass          = nullptr
            },
            {
                .colorAttachmentCount    = 1,
                .pColorAttachmentFormats = &swapchain.swapchain_surface_format().format,
                .depthAttachmentFormat = device.find_depth_format()
            }
        };

        m_graphics_pipeline = device.logical().createGraphicsPipeline(nullptr,
                                                              pipeline_create_info_chain.get<
                                                                  vk::GraphicsPipelineCreateInfo>());
    }

    auto ThreshVkPipeline::load_shader(const std::string& shader_path, ThreshVkDevice& device) -> vk::raii::ShaderModule {

        const auto shader_code = substratum::VFS::read_file(shader_path);

        vk::ShaderModuleCreateInfo shader_module_info{
            .codeSize = shader_code.size() * sizeof(uint8_t),
            .pCode    = reinterpret_cast<const uint32_t*>(shader_code.data())
        };

        return {device.logical(), shader_module_info};
    }
} // flux