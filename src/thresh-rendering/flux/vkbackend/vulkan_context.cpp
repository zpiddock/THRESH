//
// Created by Admin on 23/04/2026.
//

#include "vulkan_context.hpp"

#include <iostream>

#include "SDL3/SDL_vulkan.h"
#include "substratum/log.hpp"
#include "vulkan/vulkan.h"

namespace flux {

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "Vulkan Validation Type: " << vk::to_string(messageSeverity) << " " << vk::to_string(messageType) << "\n";
        std::cerr << "Validation Message: " << pCallbackData->pMessage << std::endl;
        return vk::False;
    }

    VulkanContext::VulkanContext(const VulkanInstanceContext& ctx) {

        create_instance(ctx);
        setup_debug_messenger(ctx);
        pick_suitable_device();
        create_logical_device();
    }

    VulkanContext::~VulkanContext() {

    };

    auto VulkanContext::create_instance(const VulkanInstanceContext& ctx) -> void {

        const vk::ApplicationInfo app_info {
            .pApplicationName = ctx.application_name.c_str(),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = ctx.engine_name.c_str(),
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_4
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

        auto extensions_required = get_required_extensions(ctx);
        auto extensions_properties = m_context.enumerateInstanceExtensionProperties();

        const auto unsupported_extensions_iterator = std::ranges::find_if(
            extensions_required, [&extensions_properties](const auto& extension) {
                return std::ranges::none_of(extensions_properties, [extension](const auto& prop) {
                    return strcmp(prop.extensionName, extension) == 0;
                });
        });

        for (const auto& layer : layers_required) {
            SUB_TRACE("Required layer: {}", layer);
        }
        for (const auto& extension : extensions_required) {
            SUB_TRACE("Required extension: {}", extension);
        }

        if (unsupported_extensions_iterator != extensions_required.end()) {
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupported_extensions_iterator));
        }

        const vk::InstanceCreateInfo instance_info {
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
        vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
                                                                              .messageType     = messageTypeFlags,
                                                                              .pfnUserCallback = &debug_callback};
        m_debugMessenger = m_instance.createDebugUtilsMessengerEXT( debugUtilsMessengerCreateInfoEXT );
    }

    auto VulkanContext::pick_suitable_device() -> void {

        auto devices = m_instance.enumeratePhysicalDevices();

        auto const device_iterator = std::ranges::find_if(devices, [&] (const auto& device) {
            return is_device_suitable(device);
        });
        if (device_iterator == devices.end()) {
            throw std::runtime_error("No suitable GPU with Vulkan 1.4 Support found");
        }
        m_physicalDevice = *device_iterator;
        SUB_TRACE("Using GPU: {}", m_physicalDevice.getProperties2().properties.deviceName.data());
    }

    auto VulkanContext::create_logical_device() -> void {

    }

    auto VulkanContext::get_required_extensions(const VulkanInstanceContext& ctx) -> std::vector<const char*> {

        uint32_t sdl_extensions_count = 0;
        auto sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);

        std::vector extensions_required(sdl_extensions, sdl_extensions + sdl_extensions_count);

        if (ctx.enable_validation_layers)
        {
            extensions_required.push_back(vk::EXTDebugUtilsExtensionName);
        }

        if (ctx.required_instance_extensions.size() > 0) {
            extensions_required.insert(extensions_required.end(), ctx.required_instance_extensions.begin(), ctx.required_instance_extensions.end());
        }

        return extensions_required;
    }

    auto VulkanContext::is_device_suitable(const vk::PhysicalDevice& device) -> bool {

        const auto device_properties = device.getProperties();

        auto support_VK1_4 = device_properties.apiVersion >= vk::ApiVersion14;

        auto queue_families = device.getQueueFamilyProperties();
        auto support_graphics = std::ranges::any_of(queue_families, [](const auto& family) {
            return !!(family.queueFlags & vk::QueueFlagBits::eGraphics);
        });

        // Check if all required physicalDevice extensions are available
        auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions =
          std::ranges::all_of(m_required_device_extensions,
                               [&availableDeviceExtensions]( auto const & requiredDeviceExtension )
                               {
                                 return std::ranges::any_of( availableDeviceExtensions,
                                                             [requiredDeviceExtension]( auto const & availableDeviceExtension )
                                                             { return strcmp( availableDeviceExtension.extensionName, requiredDeviceExtension ) == 0; } );
                               } );

        // Check if the physicalDevice supports the required features (dynamic rendering and extended dynamic state)
        auto features =
          device
            .getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                        features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        return support_VK1_4 && support_graphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }
} // flux