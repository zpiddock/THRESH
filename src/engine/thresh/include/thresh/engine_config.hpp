#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <cstdint>

namespace thresh {
    /**
     * Configuration for the THRESH engine.
     * Passed to Engine constructor to control window, rendering, and debug options.
     */
    struct EngineConfig {
        std::string application_name = "THRESH Application";
        std::string window_title = "THRΞSH";
        std::uint32_t window_width = 1280;
        std::uint32_t window_height = 720;
        bool window_resizable = true;
        bool window_maximized = true;
        VkPresentModeKHR preferred_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        bool enable_validation = true;
        bool enable_shader_hot_reload = true;
    };
} // namespace thresh
