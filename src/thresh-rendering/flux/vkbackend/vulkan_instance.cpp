//
// Created by Admin on 06/05/2026.
//

#include "vulkan_instance.hpp"

#include <vector>

#include <SDL3/SDL_vulkan.h>

#include "vk_structs.hpp"
#include "substratum/log.hpp"

namespace flux {
    ThreshVkInstance::ThreshVkInstance(const VulkanInstanceContext& ctx, const thresh::Window& window) {

        create_instance(ctx);
        setup_debug_messenger(ctx);
        create_surface(window);
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT             messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                         pUserData) {
        const auto type_str = vk::to_string(messageType);
        const auto* msg = pCallbackData->pMessage;
        switch (messageSeverity) {
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
                SUB_TRACE("[Vulkan {}] {}", type_str, msg);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
                SUB_INFO("[Vulkan {}] {}", type_str, msg);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
                SUB_WARN("[Vulkan {}] {}", type_str, msg);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            default:
                SUB_ERROR("[Vulkan {}] {}", type_str, msg);
                break;
        }
        return vk::False;
    }

    auto ThreshVkInstance::create_instance(const VulkanInstanceContext& ctx) -> void {
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

        vk::InstanceCreateInfo instance_info{
            .pApplicationInfo        = &app_info,
            .enabledLayerCount       = static_cast<uint32_t>(layers_required.size()),
            .ppEnabledLayerNames     = layers_required.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(extensions_required.size()),
            .ppEnabledExtensionNames = extensions_required.data()
        };

        std::vector<vk::ValidationFeatureEnableEXT> sync_enables = {
            vk::ValidationFeatureEnableEXT::eSynchronizationValidation
        };
        vk::ValidationFeaturesEXT validation_features{};
        validation_features.setEnabledValidationFeatures(sync_enables);

        if (ctx.enable_validation_layers && ctx.enable_sync_validation) {
            instance_info.pNext = &validation_features;
        }

        SUB_INFO("Initialising Vulkan Instance");
        m_instance = vk::raii::Instance(m_context, instance_info);
    }

    auto ThreshVkInstance::setup_debug_messenger(const VulkanInstanceContext& ctx) -> void {
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

    auto ThreshVkInstance::create_surface(const thresh::Window& window) -> void {
        SUB_INFO("Creating Vulkan Surface");
        VkSurfaceKHR surface;
        if (SDL_Vulkan_CreateSurface(window.getWindow(), *m_instance, nullptr, &surface) != false) {
            m_surface = vk::raii::SurfaceKHR(m_instance, surface);
        } else {
            throw std::runtime_error("Failed to create Vulkan Surface");
        }
    }

    auto ThreshVkInstance::get_required_extensions(const VulkanInstanceContext& ctx) -> std::vector<const char*> {
        uint32_t       sdl_extensions_count = 0;
        const auto     sdl_extensions       = SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);

        std::vector extensions_required(sdl_extensions, sdl_extensions + sdl_extensions_count);

        if (ctx.enable_validation_layers) {
            extensions_required.push_back(vk::EXTDebugUtilsExtensionName);
        }

        if (!ctx.required_instance_extensions.empty()) {
            extensions_required.insert(extensions_required.end(), ctx.required_instance_extensions.begin(),
                                       ctx.required_instance_extensions.end());
        }

        return extensions_required;
    }
} // flux