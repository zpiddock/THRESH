#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "horizon/window.hpp"

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
    /**
 * RAII wrapper for VkSurfaceKHR.
 * Manages the lifetime of a Vulkan surface tied to a window.
 */
    class FLUX_API Surface {
    public:
        /**
     * Create a surface for the given window.
     * @param instance Vulkan instance (must outlive this Surface)
     * @param window Horizon window (provides platform abstraction)
     */
        Surface(VkInstance instance, horizon::Window &window);

        ~Surface();

        Surface(const Surface &) = delete;

        Surface &operator=(const Surface &) = delete;

        Surface(Surface &&) noexcept;

        Surface &operator=(Surface &&) noexcept;

        auto get_handle() const -> VkSurfaceKHR { return m_surface; }

        /**
     * Query surface capabilities for a physical device.
     * Used during swapchain creation.
     */
        auto get_capabilities(VkPhysicalDevice device) const -> VkSurfaceCapabilitiesKHR;

        /**
     * Query supported surface formats.
     */
        auto get_formats(VkPhysicalDevice device) const -> std::vector<VkSurfaceFormatKHR>;

        /**
     * Query supported present modes.
     */
        auto get_present_modes(VkPhysicalDevice device) const -> std::vector<VkPresentModeKHR>;

    private:
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    };
} // namespace flux