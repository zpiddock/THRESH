#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <cstdint>
#include <vector>

namespace thresh {

    /**
     * A VFS mount point: maps a real directory or archive to a virtual path.
     */
    struct VfsMountEntry {
        std::string real_path;          ///< Filesystem path to directory or archive
        std::string mount_point = "/";  ///< Virtual directory to mount at
        bool append = true;             ///< Append to search path (true) or prepend (false)
    };

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

        /**
         * Virtual filesystem mount points.
         * If empty, the engine will mount the current working directory at "/".
         * For dev: mount source asset folders.
         * For release: mount .zip / .pak archives.
         */
        std::vector<VfsMountEntry> vfs_mounts = {};
    };
} // namespace thresh
