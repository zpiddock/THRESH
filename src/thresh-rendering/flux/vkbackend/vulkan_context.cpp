//
// Created by Admin on 23/04/2026.
//

#include "vulkan_context.hpp"

#include <iostream>
#include <set>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

#include "flux/graphics_types.hpp"
#include "SDL3/SDL_vulkan.h"
#include "substratum/log.hpp"
#include "substratum/filesystem/vfs.hpp"

namespace flux {
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    const std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    // static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
    //     vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    //     vk::DebugUtilsMessageTypeFlagsEXT             messageType,
    //     const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    //     void*                                         pUserData) {
    //     std::cerr << "Vulkan Validation Type: " << vk::to_string(messageSeverity) << " " << vk::to_string(messageType)
    //             << "\n";
    //     std::cerr << "Validation Message: " << pCallbackData->pMessage << std::endl;
    //     return vk::False;
    // }

    VulkanContext::VulkanContext(const VulkanInstanceContext& ctx, const thresh::Window& window) :
    m_vk_instance(ctx, window),
    m_vk_device(m_vk_instance.instance(), m_vk_instance.surface()) {

        // pick_suitable_device();
        // create_logical_device();
        // create_command_pool();
        create_swapchain(window);
        create_image_views();
        create_descriptor_set_layouts();
        create_graphics_pipelines();
        create_depth_resources();
        create_texture_image();
        create_texture_image_view();
        create_texture_sampler();
        create_vertex_buffer();
        create_index_buffer();
        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_sets();
        create_command_buffers();
        create_sync_objects();
    }

    VulkanContext::~VulkanContext() {
    }

    // auto VulkanContext::create_instance(const VulkanInstanceContext& ctx) -> void {
    //     const vk::ApplicationInfo app_info{
    //         .pApplicationName   = ctx.application_name.c_str(),
    //         .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    //         .pEngineName        = ctx.engine_name.c_str(),
    //         .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
    //         .apiVersion         = VK_API_VERSION_1_4
    //     };
    //
    //     // Get required vulkan layers
    //     std::vector<const char*> layers_required;
    //     if (ctx.enable_validation_layers) {
    //         layers_required.assign(ctx.enabled_validation_layers.begin(),
    //                                ctx.enabled_validation_layers.end());
    //     }
    //
    //     // Check required layers are supported
    //     auto vk_layer_properties = m_context.enumerateInstanceLayerProperties();
    //     if (std::ranges::any_of(ctx.enabled_validation_layers, [&vk_layer_properties](const auto& layer) -> bool {
    //         return std::ranges::none_of(vk_layer_properties, [layer](const auto& prop) {
    //             return strcmp(prop.layerName, layer) == 0;
    //         });
    //     })) {
    //         throw std::runtime_error("Required validation layers not supported");
    //     }
    //
    //     auto extensions_required   = get_required_extensions(ctx);
    //     auto extensions_properties = m_context.enumerateInstanceExtensionProperties();
    //
    //     const auto unsupported_extensions_iterator =
    //         std::ranges::find_if(extensions_required,
    //             [&extensions_properties](const auto& extension) {
    //                   return std::ranges::none_of(
    //                       extensions_properties,[extension](const auto& prop) {
    //                                   return strcmp(prop.extensionName, extension)== 0;
    //                               });
    //               });
    //
    //     for (const auto& layer : layers_required) {
    //         SUB_TRACE("Required layer: {}", layer);
    //     }
    //     for (const auto& extension : extensions_required) {
    //         SUB_TRACE("Required extension: {}", extension);
    //     }
    //
    //     if (unsupported_extensions_iterator != extensions_required.end()) {
    //         throw std::runtime_error("Required extension not supported: " +
    //                                  std::string(*unsupported_extensions_iterator));
    //     }
    //
    //     const vk::InstanceCreateInfo instance_info{
    //         .pApplicationInfo        = &app_info,
    //         .enabledLayerCount       = static_cast<uint32_t>(layers_required.size()),
    //         .ppEnabledLayerNames     = layers_required.data(),
    //         .enabledExtensionCount   = static_cast<uint32_t>(extensions_required.size()),
    //         .ppEnabledExtensionNames = extensions_required.data()
    //     };
    //
    //     SUB_INFO("Initialising Vulkan Instance");
    //     m_instance = vk::raii::Instance(m_context, instance_info);
    // }
    //
    // auto VulkanContext::setup_debug_messenger(const VulkanInstanceContext& ctx) -> void {
    //     if (!ctx.enable_validation_layers) {
    //         return;
    //     }
    //
    //     vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
    //                                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    //     vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
    //                                                        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
    //                                                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
    //                                                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    //     vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
    //         .messageSeverity = severityFlags,
    //         .messageType     = messageTypeFlags,
    //         .pfnUserCallback = &debug_callback
    //     };
    //     m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    // }
    //
    // auto VulkanContext::create_surface(const thresh::Window& window) -> void {
    //     SUB_INFO("Creating Vulkan Surface");
    //     VkSurfaceKHR surface;
    //     if (SDL_Vulkan_CreateSurface(window.getWindow(), *m_instance, nullptr, &surface) != false) {
    //         m_surface = vk::raii::SurfaceKHR(m_instance, surface);
    //     } else {
    //         throw std::runtime_error("Failed to create Vulkan Surface");
    //     }
    // }

    auto VulkanContext::pick_suitable_device() -> void {
        // auto devices = m_vk_instance.instance().enumeratePhysicalDevices();
        //
        // auto const device_iterator = std::ranges::find_if(devices, [&](const auto& device) {
        //     return is_device_suitable(device);
        // });
        // if (device_iterator == devices.end()) {
        //     throw std::runtime_error("No suitable GPU with Vulkan 1.4 Support found");
        // }
        // m_physicalDevice = *device_iterator;
        // SUB_TRACE("Using GPU: {}", m_physicalDevice.getProperties2().properties.deviceName.data());
    }

    auto VulkanContext::create_logical_device() -> void {
        // const auto queue_family_properties = m_physicalDevice.getQueueFamilyProperties();
        //
        // for (uint32_t qfpIndex = 0; qfpIndex < queue_family_properties.size(); qfpIndex++) {
        //     if ((queue_family_properties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
        //         m_physicalDevice.getSurfaceSupportKHR(qfpIndex, m_vk_instance.surface())) {
        //         // found a queue family that supports both graphics and present
        //         m_queue_family_index = qfpIndex;
        //         break;
        //     }
        // }
        // if (m_queue_family_index == ~0u) {
        //     throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        // }
        //
        // vk::StructureChain<
        //             vk::PhysicalDeviceFeatures2,
        //             vk::PhysicalDeviceVulkan11Features,
        //             vk::PhysicalDeviceVulkan13Features,
        //             vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        //         features{
        //             {
        //                 .features = {
        //                     .samplerAnisotropy = true,
        //                 }
        //             },
        //             {
        //                 .shaderDrawParameters = true,
        //             },
        //             {
        //                 .synchronization2 = true,
        //                 .dynamicRendering = true
        //             },
        //             {
        //                 .extendedDynamicState = true
        //             }
        //         };
        //
        // float                     queue_priority = .5f;
        // vk::DeviceQueueCreateInfo queue_info{
        //     .queueFamilyIndex = m_queue_family_index,
        //     .queueCount       = 1,
        //     .pQueuePriorities = &queue_priority
        // };
        //
        // vk::DeviceCreateInfo device_info{
        //     .pNext                   = &features.get<vk::PhysicalDeviceFeatures2>(),
        //     .queueCreateInfoCount    = 1,
        //     .pQueueCreateInfos       = &queue_info,
        //     .enabledExtensionCount   = static_cast<uint32_t>(m_required_device_extensions.size()),
        //     .ppEnabledExtensionNames = m_required_device_extensions.data()
        // };
        //
        // m_device         = vk::raii::Device(m_physicalDevice, device_info);
        // m_graphics_queue = vk::raii::Queue(m_device, m_queue_family_index, 0);
    }

    auto VulkanContext::create_swapchain(const thresh::Window& window) -> void {
        const auto surface = m_vk_instance.surface();

        vk::SurfaceCapabilitiesKHR surface_capabilities = m_vk_device.physical().getSurfaceCapabilitiesKHR(surface);
        m_swapchain_extent                              = choose_swap_extents(surface_capabilities, window);
        uint32_t min_swap_image_count                   = choose_min_swap_image_count(surface_capabilities);

        std::vector<vk::SurfaceFormatKHR> formats = m_vk_device.physical().getSurfaceFormatsKHR(surface);
        m_swapchain_surface_format                = choose_swap_surface_format(formats);

        vk::PresentModeKHR present_mode =
                choose_swapchain_present_mode(m_vk_device.physical().getSurfacePresentModesKHR(surface));
        vk::SwapchainCreateInfoKHR swapchain_info{
            .surface          = surface,
            .minImageCount    = min_swap_image_count,
            .imageFormat      = m_swapchain_surface_format.format,
            .imageColorSpace  = m_swapchain_surface_format.colorSpace,
            .imageExtent      = m_swapchain_extent,
            .imageArrayLayers = 1, // Only ever higher if doing Stereoscopic
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform     = surface_capabilities.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = present_mode,
            .clipped          = vk::True,
            .oldSwapchain     = nullptr
        };

        m_swapchain        = vk::raii::SwapchainKHR(m_vk_device.logical(), swapchain_info);
        m_swapchain_images = m_swapchain.getImages();
    }

    auto VulkanContext::create_image_views() -> void {

        m_swapchain_image_views.reserve(m_swapchain_images.size());

        for (const auto& image : m_swapchain_images) {
            m_swapchain_image_views.emplace_back(create_image_view(image, m_swapchain_surface_format.format, vk::ImageAspectFlagBits::eColor));
        }
    }

    auto VulkanContext::create_descriptor_set_layouts() -> void {
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
        m_descriptor_set_layout = vk::raii::DescriptorSetLayout(m_vk_device.logical(), layout_info);
    }

    auto VulkanContext::create_graphics_pipelines() -> void {
        assert(substratum::VFS::is_initialized());

        auto                                    triangle_shader_module = load_shader("/shader/triangle.spv");
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

        vk::PipelineLayoutCreateInfo pipeline_layout_info{
            .setLayoutCount         = 1,
            .pSetLayouts            = &*m_descriptor_set_layout,
            .pushConstantRangeCount = 0
        };

        m_pipeline_layout = vk::raii::PipelineLayout(m_vk_device.logical(), pipeline_layout_info);

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
                .pColorAttachmentFormats = &m_swapchain_surface_format.format,
                .depthAttachmentFormat = find_depth_format()
            }
        };

        m_graphics_pipeline = m_vk_device.logical().createGraphicsPipeline(nullptr,
                                                              pipeline_create_info_chain.get<
                                                                  vk::GraphicsPipelineCreateInfo>());
    }

    auto VulkanContext::create_command_pool() -> void {
        // vk::CommandPoolCreateInfo pool_info{
        //     .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        //     .queueFamilyIndex = m_queue_family_index
        // };
        // m_command_pool = vk::raii::CommandPool(m_device, pool_info);
    }

    auto VulkanContext::create_depth_resources() -> void {

        vk::Format format = find_depth_format();
        auto [depth_image, depth_image_memory] = create_image(m_swapchain_extent.width, m_swapchain_extent.height, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);

        m_depth_image = std::move(depth_image);
        m_depth_image_memory = std::move(depth_image_memory);
        m_depth_image_view = create_image_view(m_depth_image, format, vk::ImageAspectFlagBits::eDepth);
    }

    auto VulkanContext::create_command_buffers() -> void {
        vk::CommandBufferAllocateInfo alloc_info{
            .commandPool        = m_vk_device.command_pool(),
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };

        m_command_buffers = vk::raii::CommandBuffers(m_vk_device.logical(), alloc_info);
    }

    auto VulkanContext::create_texture_image() -> void {

        int width, height, nrChannels;

        auto texture_data = substratum::VFS::read_file("textures/brick.png");

        stbi_uc* data = stbi_load_from_memory(texture_data.data(), texture_data.size(), &width, &height, &nrChannels, STBI_rgb_alpha);

        vk::DeviceSize image_size = width * height * STBI_rgb_alpha;

        if (!data || width <= 0 || height <= 0) {
            SUB_FATAL("Failed to load texture image!");
        }

        SUB_TRACE("Texture Data: Size:{}, Width:{}, Height:{}, Channels:{}", texture_data.size(), width, height, nrChannels);

        auto [buffer, buffer_memory] = create_buffer(
            image_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );

        void* data_staging = buffer_memory.mapMemory(0, image_size);
        memcpy(data_staging, data, image_size);
        buffer_memory.unmapMemory();

        // Free STB memory
        stbi_image_free(data);

        auto [texture, texture_memory] = create_image(width, height, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

        transition_image_layout(texture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor);
        copy_buffer_to_image(buffer, texture, width, height);
        transition_image_layout(texture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageAspectFlagBits::eColor);

        m_image = std::move(texture);
        m_image_memory = std::move(texture_memory);
    }

    auto VulkanContext::create_texture_image_view() -> void {

        m_image_view = create_image_view(m_image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
    }

    auto VulkanContext::create_texture_sampler() -> void {

        vk::PhysicalDeviceProperties props = m_vk_device.physical().getProperties();
        vk::SamplerCreateInfo sampler_info{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eMirroredRepeat,
            .addressModeV = vk::SamplerAddressMode::eMirroredRepeat,
            .addressModeW = vk::SamplerAddressMode::eMirroredRepeat,
            .anisotropyEnable = vk::True,
            .maxAnisotropy = props.limits.maxSamplerAnisotropy,
            .compareEnable = vk::False,
            .compareOp = vk::CompareOp::eAlways,

        };

        m_texture_sampler = vk::raii::Sampler(m_vk_device.logical(), sampler_info);
    }

    auto VulkanContext::create_vertex_buffer() -> void {

        vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

        auto [staging_buffer, staging_buffer_memory] =
            create_buffer(buffer_size,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                );

        void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
        memcpy(data_staging, vertices.data(), buffer_size);
        staging_buffer_memory.unmapMemory();

        std::tie(m_vertex_buffer, m_vertex_buffer_memory) =
             create_buffer(buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

        copy_buffer(staging_buffer, m_vertex_buffer, buffer_size);
    }

    auto VulkanContext::create_index_buffer() -> void {

        vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();
        auto [staging_buffer, staging_buffer_memory] =
            create_buffer(buffer_size,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                );
        void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
        memcpy(data_staging, indices.data(), buffer_size);
        staging_buffer_memory.unmapMemory();
        std::tie(m_index_buffer, m_index_buffer_memory) =
             create_buffer(buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copy_buffer(staging_buffer, m_index_buffer, buffer_size);
    }

    auto VulkanContext::create_uniform_buffers() -> void {

        m_uniform_buffers.clear();
        m_uniform_buffer_memory.clear();
        m_uniform_buffers_mapped.clear();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

            auto [buffer, memory] =
                create_buffer(buffer_size,
                    vk::BufferUsageFlagBits::eUniformBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                    );

            m_uniform_buffers.emplace_back(std::move(buffer));
            m_uniform_buffer_memory.emplace_back(std::move(memory));
            m_uniform_buffers_mapped.emplace_back(m_uniform_buffer_memory[i].mapMemory(0, buffer_size));
        }
    }

    auto VulkanContext::create_descriptor_pool() -> void {

        std::array pool_size {

            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT },
            vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT },
        };

        vk::DescriptorPoolCreateInfo pool_create_info {
            .flags          = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets        = MAX_FRAMES_IN_FLIGHT,
            .poolSizeCount  = pool_size.size(),
            .pPoolSizes     = pool_size.data()
        };

        m_descriptor_pool = vk::raii::DescriptorPool(m_vk_device.logical(), pool_create_info);
    }

    auto VulkanContext::create_descriptor_sets() -> void {

        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_descriptor_set_layout);
        vk::DescriptorSetAllocateInfo alloc_info{
            .descriptorPool = m_descriptor_pool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data()
        };
        m_descriptor_sets.clear();
        m_descriptor_sets = vk::raii::DescriptorSets(m_vk_device.logical(), alloc_info);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo buffer_info{
                .buffer = m_uniform_buffers[i],
                .offset = 0,
                .range  = sizeof(UniformBufferObject)
            };
            vk::DescriptorImageInfo image_info {
                .sampler = m_texture_sampler,
                .imageView = m_image_view,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };
            std::array descriptor_writes {

                vk::WriteDescriptorSet {
                    .dstSet = m_descriptor_sets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo = &buffer_info
                },
                vk::WriteDescriptorSet {
                .dstSet = m_descriptor_sets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &image_info
                }
            };
            m_vk_device.logical().updateDescriptorSets(descriptor_writes, {});
        }
    }

    auto VulkanContext::create_sync_objects() -> void {

        assert(m_present_complete_semaphores.empty() && m_render_complete_semaphores.empty() && m_inflight_fences.empty());

        for (size_t i = 0; i < m_swapchain_images.size(); i++) {
            m_render_complete_semaphores.emplace_back(m_vk_device.logical(), vk::SemaphoreCreateInfo{});
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_present_complete_semaphores.emplace_back(m_vk_device.logical(), vk::SemaphoreCreateInfo{});
            m_inflight_fences.emplace_back(m_vk_device.logical(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        }
    }

    auto VulkanContext::record_command_buffer(const uint32_t image_index) -> void {
        const vk::CommandBufferBeginInfo begin_info{};

        auto& command_buffer = m_command_buffers[m_frame_index];

        command_buffer.begin(begin_info);

        transition_image_layout(
            m_swapchain_images[image_index],
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::ImageAspectFlagBits::eColor
            );

        transition_image_layout(
            m_depth_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::ImageAspectFlagBits::eDepth
        );

        constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.f, 0.f, 0.f, 1.f);
        constexpr vk::ClearValue depth_clear_value = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderingAttachmentInfo attachment_info {
            .imageView = m_swapchain_image_views[image_index],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clear_color
        };

        vk::RenderingAttachmentInfo depth_attachment_info {
            .imageView = m_depth_image_view,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .clearValue = depth_clear_value
        };

        vk::RenderingInfo rendering_info {
            .renderArea = {.offset = {0, 0}, .extent = m_swapchain_extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachment_info,
            .pDepthAttachment = &depth_attachment_info
        };

        command_buffer.beginRendering(rendering_info);

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);
        command_buffer.setViewport(0,
            vk::Viewport{0, 0, static_cast<float>(m_swapchain_extent.width)
                , static_cast<float>(m_swapchain_extent.height), 0, 1});
        command_buffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, m_swapchain_extent});

        command_buffer.bindVertexBuffers(0, {*m_vertex_buffer}, {0});
        command_buffer.bindIndexBuffer(*m_index_buffer, 0, vk::IndexType::eUint32);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, *m_descriptor_sets[m_frame_index], nullptr);
        command_buffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        command_buffer.endRendering();

        transition_image_layout(
            m_swapchain_images[image_index],
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
            {},                                                     // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
            vk::ImageAspectFlagBits::eColor
        );
        command_buffer.end();
    }

    auto VulkanContext::transition_image_layout(vk::Image         image, vk::ImageLayout  old_layout,
                                                vk::ImageLayout         new_layout, vk::AccessFlags2 src_access_mask,
                                                vk::AccessFlags2        dst_access_mask,
                                                vk::PipelineStageFlags2 src_stage_mask,
                                                vk::PipelineStageFlags2 dst_stage_mask, vk::ImageAspectFlags aspect_flags) -> void {
        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask        = src_stage_mask,
            .srcAccessMask       = src_access_mask,
            .dstStageMask        = dst_stage_mask,
            .dstAccessMask       = dst_access_mask,
            .oldLayout           = old_layout,
            .newLayout           = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image,
            .subresourceRange    = {
                .aspectMask     = aspect_flags,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };
        vk::DependencyInfo dependency_info = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };
        m_command_buffers[m_frame_index].pipelineBarrier2(dependency_info);
    }

    auto VulkanContext::choose_swap_extents(const vk::SurfaceCapabilitiesKHR& surface_capabilities,
                                            const thresh::Window&             window) -> vk::Extent2D {
        if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return surface_capabilities.currentExtent;
        }

        int width, height;
        window.get_frame_buffer_size(width, height);
        return {
            std::clamp<uint32_t>(width, surface_capabilities.minImageExtent.width,
                                 surface_capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, surface_capabilities.minImageExtent.height,
                                 surface_capabilities.maxImageExtent.height)
        };
    }

    auto VulkanContext::choose_swap_surface_format(
        const std::vector<vk::SurfaceFormatKHR>& formats) -> vk::SurfaceFormatKHR {
        const auto format_interator = std::ranges::find_if(formats, [](const auto& format) {
            return format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace ==
                    vk::ColorSpaceKHR::eSrgbNonlinear;
        });

        return format_interator != formats.end() ? *format_interator : formats[0];
    }

    auto VulkanContext::choose_min_swap_image_count(
        const vk::SurfaceCapabilitiesKHR& surface_capabilities) -> uint32_t {
        auto min_image_count = std::max(3u, surface_capabilities.minImageCount);
        if ((0 > surface_capabilities.maxImageCount) && (min_image_count > surface_capabilities.maxImageCount)) {
            min_image_count = surface_capabilities.maxImageCount;
        }
        return min_image_count;
    }

    auto VulkanContext::choose_swapchain_present_mode(
        const std::vector<vk::PresentModeKHR>& present_modes) -> vk::PresentModeKHR {
        assert(std::ranges::any_of(present_modes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo
                   ; }));
        return std::ranges::any_of(present_modes,
                                   [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; })
                   ? vk::PresentModeKHR::eMailbox
                   : vk::PresentModeKHR::eFifo;
    }

    auto VulkanContext::get_required_extensions(const VulkanInstanceContext& ctx) -> std::vector<const char*> {
        uint32_t sdl_extensions_count = 0;
        auto     sdl_extensions       = SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);

        std::vector extensions_required(sdl_extensions, sdl_extensions + sdl_extensions_count);

        if (ctx.enable_validation_layers) {
            extensions_required.push_back(vk::EXTDebugUtilsExtensionName);
        }

        if (ctx.required_instance_extensions.size() > 0) {
            extensions_required.insert(extensions_required.end(), ctx.required_instance_extensions.begin(),
                                       ctx.required_instance_extensions.end());
        }

        return extensions_required;
    }

    auto VulkanContext::is_device_suitable(const vk::PhysicalDevice& device) -> bool  {
        // const auto device_properties = device.getProperties();
        //
        // auto support_VK1_4 = device_properties.apiVersion >= vk::ApiVersion14;
        //
        // auto queue_families   = device.getQueueFamilyProperties();
        // auto support_graphics = std::ranges::any_of(queue_families, [](const auto& family) {
        //     return !!(family.queueFlags & vk::QueueFlagBits::eGraphics);
        // });
        //
        // // Check if all required physicalDevice extensions are available
        // auto availableDeviceExtensions     = device.enumerateDeviceExtensionProperties();
        // bool supportsAllRequiredExtensions =
        //         std::ranges::all_of(m_required_device_extensions,
        //                             [&availableDeviceExtensions](auto const& requiredDeviceExtension) {
        //                                 return std::ranges::any_of(availableDeviceExtensions,
        //                                                            [requiredDeviceExtension](
        //                                                        auto const& availableDeviceExtension) {
        //                                                                return strcmp(availableDeviceExtension.
        //                                                                    extensionName,
        //                                                                    requiredDeviceExtension) == 0;
        //                                                            });
        //                             });
        //
        // // Check if the physicalDevice supports the required features (dynamic rendering and extended dynamic state)
        // auto features =
        //         device
        //         .getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
        //                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        // bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        //         features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
        //
        // return support_VK1_4 && support_graphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
        return false;
    }

    auto VulkanContext::load_shader(const std::string& shader_path) -> vk::raii::ShaderModule {
        const auto shader_code = substratum::VFS::read_file(shader_path);

        vk::ShaderModuleCreateInfo shader_module_info{
            .codeSize = shader_code.size() * sizeof(uint8_t),
            .pCode    = reinterpret_cast<const uint32_t*>(shader_code.data())
        };

        return {m_vk_device.logical(), shader_module_info};
    }

    auto VulkanContext::find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) -> uint32_t {

        vk::PhysicalDeviceMemoryProperties memory_properties = m_vk_device.physical().getMemoryProperties();
        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        SUB_FATAL("Failed to find suitable memory type!");
        std::unreachable();
    }

    auto VulkanContext::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties) -> std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> {

        const vk::BufferCreateInfo buffer_info{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
          };

        auto buffer = vk::raii::Buffer(m_vk_device.logical(), buffer_info);

        auto mem_reqs = buffer.getMemoryRequirements();

        vk::MemoryAllocateInfo alloc_info{
            .allocationSize = mem_reqs.size,
            .memoryTypeIndex = find_memory_type(mem_reqs.memoryTypeBits, properties)
        };

        auto buffer_memory = vk::raii::DeviceMemory(m_vk_device.logical(), alloc_info);

        buffer.bindMemory(*buffer_memory, 0);

        return {std::move(buffer), std::move(buffer_memory)};
    }

    auto VulkanContext::begin_single_time_commands() -> vk::raii::CommandBuffer {

        vk::CommandBufferAllocateInfo alloc_info{
            .commandPool = m_vk_device.command_pool(),
            .level       = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        vk::raii::CommandBuffer command_buffer = std::move(m_vk_device.logical().allocateCommandBuffers(alloc_info).front());

        command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        return command_buffer;
    }

    auto VulkanContext::end_single_time_commands(const vk::raii::CommandBuffer& command_buffer) -> void {

        command_buffer.end();

        m_vk_device.graphics_queue().submit(vk::SubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers    = &*command_buffer},
        nullptr);

        m_vk_device.graphics_queue().waitIdle();
    }

    auto VulkanContext::copy_buffer(const vk::raii::Buffer& src_buffer, vk::raii::Buffer& dst_buffer,
                                    vk::DeviceSize size) -> void {

        const auto command_buffer = begin_single_time_commands();

        command_buffer.copyBuffer(*src_buffer, *dst_buffer, vk::BufferCopy(0, 0, size));

        end_single_time_commands(command_buffer);
    }

    auto VulkanContext::copy_buffer_to_image(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width,
        uint32_t height) -> void {

        const auto command_buffer = begin_single_time_commands();

        vk::BufferImageCopy region {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = vk::ImageSubresourceLayers{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1}
        };
        command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

        end_single_time_commands(command_buffer);
    }

    auto VulkanContext::create_image(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling_mode,
                                     vk::ImageUsageFlags usage_flags,
                                     vk::MemoryPropertyFlags memory_props) -> std::pair<vk::raii::Image, vk::raii::DeviceMemory> {

        vk::raii::Image texture_image_temp({});
        vk::raii::DeviceMemory texture_image_memory_temp({});

        vk::ImageCreateInfo image_info{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = {width, height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = tiling_mode,
            .usage = usage_flags,
            .sharingMode = vk::SharingMode::eExclusive,
        };
        texture_image_temp = vk::raii::Image(m_vk_device.logical(), image_info);
        texture_image_memory_temp = vk::raii::DeviceMemory(m_vk_device.logical(), vk::MemoryAllocateInfo{
            .allocationSize = texture_image_temp.getMemoryRequirements().size,
            .memoryTypeIndex = find_memory_type(texture_image_temp.getMemoryRequirements().memoryTypeBits, memory_props)
        });
        texture_image_temp.bindMemory(*texture_image_memory_temp, 0);

        return {std::move(texture_image_temp), std::move(texture_image_memory_temp)};
    }

    auto VulkanContext::transition_image_layout(const vk::raii::Image& image, const vk::ImageLayout old_layout,
        const vk::ImageLayout new_layout, vk::ImageAspectFlags aspect_flags) -> void {

        const auto command_buffer = begin_single_time_commands();

        vk::ImageMemoryBarrier barrier{
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .image = image,
            .subresourceRange = {
                .aspectMask = aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vk::PipelineStageFlags source_stage;
        vk::PipelineStageFlags destination_stage;

        if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            source_stage      = vk::PipelineStageFlagBits::eTopOfPipe;
            destination_stage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            source_stage      = vk::PipelineStageFlagBits::eTransfer;
            destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            SUB_FATAL("unsupported layout transition!");
        }

        command_buffer.pipelineBarrier(source_stage, destination_stage, {}, {}, nullptr, barrier);

        end_single_time_commands(command_buffer);
    }

    auto VulkanContext::create_image_view(const vk::Image& image, vk::Format format, vk::ImageAspectFlags flags) -> vk::raii::ImageView {

        vk::ImageViewCreateInfo view_info{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = {
            .aspectMask = flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        return vk::raii::ImageView(m_vk_device.logical(), view_info);
    }

    auto VulkanContext::find_supported_format(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
        vk::FormatFeatureFlags features) -> vk::Format {

        for (const auto& candidate : candidates) {
            auto format_properties = m_vk_device.physical().getFormatProperties(candidate);

            if (tiling == vk::ImageTiling::eLinear && (format_properties.linearTilingFeatures & features) == features) {
                return candidate;
            }
            if (tiling == vk::ImageTiling::eOptimal && (format_properties.optimalTilingFeatures & features) == features) {
                return candidate;
            }
        }
        SUB_FATAL("Failed to find supported format!");
        std::unreachable();
    }

    auto VulkanContext::find_depth_format() -> vk::Format {
        return find_supported_format(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    auto VulkanContext::has_stencil_component(vk::Format format) -> bool {

        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    auto VulkanContext::cleanup_swapchain() -> void {

        m_swapchain_image_views.clear();
        m_swapchain = nullptr;
    }
} // flux
