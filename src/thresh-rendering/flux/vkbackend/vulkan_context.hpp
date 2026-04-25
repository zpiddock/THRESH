//
// Created by Admin on 23/04/2026.
//

#pragma once
#include <string>
#include "vulkan/vulkan_raii.hpp"

namespace flux {

    struct VulkanInstanceContext {
        std::string application_name;
        std::string engine_name = "THRΞSH";
        std::string engine_version = "0.0.1";
        std::string application_version = "0.0.1";
        bool enable_validation_layers = true;
        std::vector<const char *> enabled_validation_layers = {
            "VK_LAYER_KHRONOS_validation"
        };
        std::vector<const char*> required_instance_extensions = {};
    };

    class VulkanContext {

        public:
            VulkanContext(const VulkanInstanceContext& ctx);
            ~VulkanContext();

        private:
            auto create_instance(const VulkanInstanceContext& ctx) -> void;
            auto setup_debug_messenger(const VulkanInstanceContext& ctx) -> void;
            auto pick_suitable_device() -> void;
            auto create_logical_device() -> void;

            auto get_required_extensions(const VulkanInstanceContext& ctx) -> std::vector<const char *>;
            auto is_device_suitable(const vk::PhysicalDevice& device) -> bool;

            std::vector<const char*> m_required_device_extensions = {
                vk::KHRSwapchainExtensionName
            };

            vk::raii::Context m_context;
            vk::raii::Instance m_instance = nullptr;
            vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;
            vk::raii::PhysicalDevice m_physicalDevice = nullptr;
            vk::raii::Device m_device = nullptr;
    };
} // flux
