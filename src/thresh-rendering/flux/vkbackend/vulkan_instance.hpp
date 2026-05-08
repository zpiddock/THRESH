//
// Created by Admin on 06/05/2026.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "vk_structs.hpp"
#include "horizon/window.hpp"

namespace flux {
    class ThreshVkInstance {

        public:
            ThreshVkInstance(const VulkanInstanceContext& ctx, const thresh::Window& window);

            auto instance() -> const vk::raii::Instance& {
                return m_instance;
            }

            auto surface() -> vk::SurfaceKHR {
                return m_surface;
            }


        private:
            // Vulkan Init Functions
            auto create_instance(const VulkanInstanceContext& ctx) -> void;

            auto setup_debug_messenger(const VulkanInstanceContext& ctx) -> void;

            auto create_surface(const thresh::Window& window) -> void;

            // Helper functions
            auto get_required_extensions(const VulkanInstanceContext& ctx) -> std::vector<const char*>;

            // Private members
            vk::raii::Context                             m_context;
            vk::raii::Instance                            m_instance                   = nullptr;
            vk::raii::DebugUtilsMessengerEXT              m_debugMessenger             = nullptr;
            vk::raii::SurfaceKHR                          m_surface                    = nullptr;
    };
} // flux
