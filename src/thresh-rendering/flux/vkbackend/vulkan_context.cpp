//
// Created by Admin on 23/04/2026.
//

#include "vulkan_context.hpp"

#include <iostream>
#include <set>

#include "SDL3/SDL_vulkan.h"
#include "substratum/log.hpp"
#include "substratum/filesystem/vfs.hpp"
#include "vulkan/vulkan.h"

namespace flux {
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT             messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                         pUserData) {
        std::cerr << "Vulkan Validation Type: " << vk::to_string(messageSeverity) << " " << vk::to_string(messageType)
                << "\n";
        std::cerr << "Validation Message: " << pCallbackData->pMessage << std::endl;
        return vk::False;
    }

    VulkanContext::VulkanContext(const VulkanInstanceContext& ctx, const thresh::Window& window) {
        create_instance(ctx);
        setup_debug_messenger(ctx);
        create_surface(window);
        pick_suitable_device();
        create_logical_device();
        create_swapchain(window);
        create_image_views();
        create_graphics_pipelines();
        create_command_pool();
        create_command_buffer();
    }

    VulkanContext::~VulkanContext() {
    }

    auto VulkanContext::create_instance(const VulkanInstanceContext& ctx) -> void {
        const vk::ApplicationInfo app_info{
            .pApplicationName   = ctx.application_name.c_str(),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = ctx.engine_name.c_str(),
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_4
        };

        // Get required vulkan layers
        std::vector<const char*> layers_required;
        if (ctx.enable_validation_layers) {
            layers_required.assign(ctx.enabled_validation_layers.begin(),
                                   ctx.enabled_validation_layers.end());
        }

        // Check required layers are supported
        auto vk_layer_properties = m_context.enumerateInstanceLayerProperties();
        if (std::ranges::any_of(ctx.enabled_validation_layers, [&vk_layer_properties](const auto& layer) -> bool {
            return std::ranges::none_of(vk_layer_properties, [layer](const auto& prop) {
                return strcmp(prop.layerName, layer) == 0;
            });
        })) {
            throw std::runtime_error("Required validation layers not supported");
        }

        auto extensions_required   = get_required_extensions(ctx);
        auto extensions_properties = m_context.enumerateInstanceExtensionProperties();

        const auto unsupported_extensions_iterator =
            std::ranges::find_if(extensions_required,
                [&extensions_properties](const auto& extension) {
                      return std::ranges::none_of(
                          extensions_properties,[extension](const auto& prop) {
                                      return strcmp(prop.extensionName, extension)== 0;
                                  });
                  });

        for (const auto& layer : layers_required) {
            SUB_TRACE("Required layer: {}", layer);
        }
        for (const auto& extension : extensions_required) {
            SUB_TRACE("Required extension: {}", extension);
        }

        if (unsupported_extensions_iterator != extensions_required.end()) {
            throw std::runtime_error("Required extension not supported: " +
                                     std::string(*unsupported_extensions_iterator));
        }

        const vk::InstanceCreateInfo instance_info{
            .pApplicationInfo        = &app_info,
            .enabledLayerCount       = static_cast<uint32_t>(layers_required.size()),
            .ppEnabledLayerNames     = layers_required.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(extensions_required.size()),
            .ppEnabledExtensionNames = extensions_required.data()
        };

        SUB_INFO("Initialising Vulkan Instance");
        m_instance = vk::raii::Instance(m_context, instance_info);
    }

    auto VulkanContext::setup_debug_messenger(const VulkanInstanceContext& ctx) -> void {
        if (!ctx.enable_validation_layers) {
            return;
        }

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType     = messageTypeFlags,
            .pfnUserCallback = &debug_callback
        };
        m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    auto VulkanContext::create_surface(const thresh::Window& window) -> void {
        SUB_INFO("Creating Vulkan Surface");
        VkSurfaceKHR surface;
        if (SDL_Vulkan_CreateSurface(window.getWindow(), *m_instance, nullptr, &surface) != false) {
            m_surface = vk::raii::SurfaceKHR(m_instance, surface);
        } else {
            throw std::runtime_error("Failed to create Vulkan Surface");
        }
    }

    auto VulkanContext::pick_suitable_device() -> void {
        auto devices = m_instance.enumeratePhysicalDevices();

        auto const device_iterator = std::ranges::find_if(devices, [&](const auto& device) {
            return is_device_suitable(device);
        });
        if (device_iterator == devices.end()) {
            throw std::runtime_error("No suitable GPU with Vulkan 1.4 Support found");
        }
        m_physicalDevice = *device_iterator;
        SUB_TRACE("Using GPU: {}", m_physicalDevice.getProperties2().properties.deviceName.data());
    }

    auto VulkanContext::create_logical_device() -> void {
        const auto queue_family_properties = m_physicalDevice.getQueueFamilyProperties();

        for (uint32_t qfpIndex = 0; qfpIndex < queue_family_properties.size(); qfpIndex++) {
            if ((queue_family_properties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_physicalDevice.getSurfaceSupportKHR(qfpIndex, *m_surface)) {
                // found a queue family that supports both graphics and present
                m_queue_family_index = qfpIndex;
                break;
            }
        }
        if (m_queue_family_index == ~0u) {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }

        vk::StructureChain<
                    vk::PhysicalDeviceFeatures2,
                    vk::PhysicalDeviceVulkan11Features,
                    vk::PhysicalDeviceVulkan13Features,
                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
                features{
                    {},
                    {
                        .shaderDrawParameters = true,
                    },
                    {
                        .dynamicRendering = true,
                    },
                    {
                        .extendedDynamicState = true
                    }
                };

        float                     queue_priority = .5f;
        vk::DeviceQueueCreateInfo queue_info{
            .queueFamilyIndex = m_queue_family_index,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority
        };

        vk::DeviceCreateInfo device_info{
            .pNext                   = &features.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount    = 1,
            .pQueueCreateInfos       = &queue_info,
            .enabledExtensionCount   = static_cast<uint32_t>(m_required_device_extensions.size()),
            .ppEnabledExtensionNames = m_required_device_extensions.data()
        };

        m_device         = vk::raii::Device(m_physicalDevice, device_info);
        m_graphics_queue = vk::raii::Queue(m_device, m_queue_family_index, 0);
    }

    auto VulkanContext::create_swapchain(const thresh::Window& window) -> void {
        vk::SurfaceCapabilitiesKHR surface_capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
        m_swapchain_extent                              = choose_swap_extents(surface_capabilities, window);
        uint32_t min_swap_image_count                   = choose_min_swap_image_count(surface_capabilities);

        std::vector<vk::SurfaceFormatKHR> formats = m_physicalDevice.getSurfaceFormatsKHR(*m_surface);
        m_swapchain_surface_format                = choose_swap_surface_format(formats);

        vk::PresentModeKHR present_mode =
                choose_swapchain_present_mode(m_physicalDevice.getSurfacePresentModesKHR(*m_surface));
        vk::SwapchainCreateInfoKHR swapchain_info{
            .surface          = *m_surface,
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

        m_swapchain        = vk::raii::SwapchainKHR(m_device, swapchain_info);
        m_swapchain_images = m_swapchain.getImages();
    }

    auto VulkanContext::create_image_views() -> void {
        assert(m_swapchain_image_views.empty());

        vk::ImageViewCreateInfo view_info{
            .viewType   = vk::ImageViewType::e2D,
            .format     = m_swapchain_surface_format.format,
            .components = {
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity
            },
            .subresourceRange = {
                vk::ImageAspectFlagBits::eColor,
                0, 1,
                0, 1
            }
        };

        for (const auto& image : m_swapchain_images) {
            view_info.image = image;
            m_swapchain_image_views.emplace_back(m_device, view_info);
        }
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
        vk::PipelineVertexInputStateCreateInfo   vertex_input_info{};
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{
            .topology = vk::PrimitiveTopology::eTriangleList,
        };
        vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

        vk::PipelineRasterizationStateCreateInfo rasterization_info{
            .depthClampEnable        = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode             = vk::PolygonMode::eFill,
            .cullMode                = vk::CullModeFlagBits::eBack,
            .frontFace               = vk::FrontFace::eClockwise,
            .depthBiasEnable         = vk::False,
            .lineWidth               = 1.0f,
        };

        vk::PipelineMultisampleStateCreateInfo multisampling_info{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable  = vk::False,
        };

        // Depth Stencil is nullptr for now

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
            .setLayoutCount         = 0,
            .pushConstantRangeCount = 0
        };

        m_pipeline_layout = vk::raii::PipelineLayout(m_device, pipeline_layout_info);

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
                .pDepthStencilState  = nullptr,
                .pColorBlendState    = &color_blending_info,
                .pDynamicState       = &dynamic_state_info,
                .layout              = m_pipeline_layout,
                .renderPass          = nullptr
            },
            {
                .colorAttachmentCount    = 1,
                .pColorAttachmentFormats = &m_swapchain_surface_format.format
            }
        };

        m_graphics_pipeline = m_device.createGraphicsPipeline(nullptr,
                                                              pipeline_create_info_chain.get<
                                                                  vk::GraphicsPipelineCreateInfo>());
    }

    auto VulkanContext::create_command_pool() -> void {
        vk::CommandPoolCreateInfo pool_info{
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = m_queue_family_index
        };
        m_command_pool = vk::raii::CommandPool(m_device, pool_info);
    }

    auto VulkanContext::create_command_buffer() -> void {
        vk::CommandBufferAllocateInfo alloc_info{
            .commandPool        = m_command_pool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        m_command_buffer = std::move(m_device.allocateCommandBuffers(alloc_info)[0]);
    }

    auto VulkanContext::record_command_buffer(const uint32_t image_index) {
        const vk::CommandBufferBeginInfo begin_info{};
        m_command_buffer.begin(begin_info);

        transition_image_layout(
            image_index,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
            );

        constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.f, 0.f, 0.f, 1.f);
        vk::RenderingAttachmentInfo attachment_info {
            .imageView = m_swapchain_image_views[image_index],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clear_color
        };

        vk::RenderingInfo rendering_info {
            .renderArea = {.offset = {0, 0}, .extent = m_swapchain_extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachment_info
        };

        m_command_buffer.beginRendering(rendering_info);

        m_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);
        m_command_buffer.setViewport(0,
            vk::Viewport{0, 0, static_cast<float>(m_swapchain_extent.width)
                , static_cast<float>(m_swapchain_extent.height), 0, 1});
        m_command_buffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, m_swapchain_extent});

        m_command_buffer.draw(3, 1, 0, 0);

        m_command_buffer.endRendering();

        transition_image_layout(
            image_index,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
            {},                                                     // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe               // dstStage
        );
        m_command_buffer.end();
    }

    auto VulkanContext::transition_image_layout(uint32_t                imageIndex, vk::ImageLayout  old_layout,
                                                vk::ImageLayout         new_layout, vk::AccessFlags2 src_access_mask,
                                                vk::AccessFlags2        dst_access_mask,
                                                vk::PipelineStageFlags2 src_stage_mask,
                                                vk::PipelineStageFlags2 dst_stage_mask) -> void {
        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask        = src_stage_mask,
            .srcAccessMask       = src_access_mask,
            .dstStageMask        = dst_stage_mask,
            .dstAccessMask       = dst_access_mask,
            .oldLayout           = old_layout,
            .newLayout           = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = m_swapchain_images[imageIndex],
            .subresourceRange    = {
                .aspectMask     = vk::ImageAspectFlagBits::eColor,
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
        m_command_buffer.pipelineBarrier2(dependency_info);
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

    auto VulkanContext::is_device_suitable(const vk::PhysicalDevice& device) -> bool {
        const auto device_properties = device.getProperties();

        auto support_VK1_4 = device_properties.apiVersion >= vk::ApiVersion14;

        auto queue_families   = device.getQueueFamilyProperties();
        auto support_graphics = std::ranges::any_of(queue_families, [](const auto& family) {
            return !!(family.queueFlags & vk::QueueFlagBits::eGraphics);
        });

        // Check if all required physicalDevice extensions are available
        auto availableDeviceExtensions     = device.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions =
                std::ranges::all_of(m_required_device_extensions,
                                    [&availableDeviceExtensions](auto const& requiredDeviceExtension) {
                                        return std::ranges::any_of(availableDeviceExtensions,
                                                                   [requiredDeviceExtension](
                                                               auto const& availableDeviceExtension) {
                                                                       return strcmp(availableDeviceExtension.
                                                                           extensionName,
                                                                           requiredDeviceExtension) == 0;
                                                                   });
                                    });

        // Check if the physicalDevice supports the required features (dynamic rendering and extended dynamic state)
        auto features =
                device
                .getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
                              vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        return support_VK1_4 && support_graphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }

    auto VulkanContext::load_shader(const std::string& shader_path) -> vk::raii::ShaderModule {
        const auto shader_code = substratum::VFS::read_file(shader_path);

        vk::ShaderModuleCreateInfo shader_module_info{
            .codeSize = shader_code.size() * sizeof(uint8_t),
            .pCode    = reinterpret_cast<const uint32_t*>(shader_code.data())
        };

        return {m_device, shader_module_info};
    }
} // flux
