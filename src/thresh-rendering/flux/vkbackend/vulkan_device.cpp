//
// Created by Admin on 06/05/2026.
//

#include "vulkan_device.hpp"

#include "substratum/log.hpp"

namespace flux {
    ThreshVkDevice::ThreshVkDevice(const vk::raii::Instance& instance, const vk::SurfaceKHR& surface) {

        pick_suitable_device(instance);
        create_logical_device(surface);
        create_command_pool();
    }

    auto ThreshVkDevice::pick_suitable_device(const vk::raii::Instance& instance) -> void {
        auto devices = instance.enumeratePhysicalDevices();

        auto const device_iterator = std::ranges::find_if(devices, [&](const auto& device) {
            return is_device_suitable(device);
        });
        if (device_iterator == devices.end()) {
            throw std::runtime_error("No suitable GPU with Vulkan 1.4 Support found");
        }
        m_physicalDevice = *device_iterator;
        SUB_TRACE("Using GPU: {}", m_physicalDevice.getProperties2().properties.deviceName.data());
    }

    auto ThreshVkDevice::create_logical_device(const vk::SurfaceKHR& surface) -> void {
        const auto queue_family_properties = m_physicalDevice.getQueueFamilyProperties();

        for (uint32_t qfpIndex = 0; qfpIndex < queue_family_properties.size(); qfpIndex++) {
            if ((queue_family_properties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_physicalDevice.getSurfaceSupportKHR(qfpIndex, surface)) {
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
                    {
                        .features = {
                            .samplerAnisotropy = true,
                        }
                    },
                    {
                        .shaderDrawParameters = true,
                    },
                    {
                        .synchronization2 = true,
                        .dynamicRendering = true
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

    auto ThreshVkDevice::is_device_suitable(const vk::PhysicalDevice& device) -> bool {
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

    auto ThreshVkDevice::create_command_pool() -> void {
        const vk::CommandPoolCreateInfo pool_info{
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = m_queue_family_index
        };
        m_command_pool = vk::raii::CommandPool(m_device, pool_info);
    }
} // flux