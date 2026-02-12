#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

#ifdef _WIN32
#ifdef FLUX_EXPORTS
#define FLUX_API __declspec(dllexport)
#else
#define FLUX_API __declspec(dllimport)
#endif
#else
#define FLUX_API
#endif

namespace flux {
    class Device;

    /**
     * Generate mipmaps using vkCmdBlitImage
     * @param device Vulkan device
     * @param cmd Command buffer to record blit commands
     * @param image Image to generate mipmaps for
     * @param format Image format (must support blit operations)
     * @param width Image width
     * @param height Image height
     * @param mip_levels Number of mip levels
     */
    FLUX_API auto generate_mipmaps(
        Device& device,
        VkCommandBuffer cmd,
        VkImage image,
        VkFormat format,
        uint32_t width,
        uint32_t height,
        uint32_t mip_levels
    ) -> void;

    /**
     * Calculate optimal mip levels for texture dimensions
     * Formula: floor(log2(max(width, height))) + 1
     */
    [[nodiscard]] FLUX_API auto calculate_mip_levels(uint32_t width, uint32_t height) -> uint32_t;
} // namespace batleth
