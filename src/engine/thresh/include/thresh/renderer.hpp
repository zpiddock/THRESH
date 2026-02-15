#pragma once

#include "thresh/renderer/frame_context.hpp"
#include "flux/instance.hpp"
#include "flux/surface.hpp"
#include "flux/device.hpp"
#include "flux/swapchain.hpp"
#include "horizon/window.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

#ifdef _WIN32
#ifdef THRESH_EXPORTS
#define THRESH_API __declspec(dllexport)
#else
#define THRESH_API __declspec(dllimport)
#endif
#else
#define THRESH_API
#endif

namespace thresh {
    struct EngineConfig;

    /**
     * Owns all Vulkan infrastructure and drives the frame cycle.
     * Manages instance, device, swapchain, and per-frame synchronization.
     */
    class THRESH_API Renderer {
    public:
        explicit Renderer(horizon::Window &window, const EngineConfig &config);

        ~Renderer();

        Renderer(const Renderer &) = delete;
        Renderer &operator=(const Renderer &) = delete;

        /**
         * Begin a new frame: wait for fence, acquire swapchain image, begin command buffer.
         * @return Command buffer to record into, or nullopt if swapchain was recreated (skip frame)
         */
        auto begin_frame() -> std::optional<VkCommandBuffer>;

        /**
         * End the current frame: end command buffer, submit, present.
         */
        auto end_frame() -> void;

        /**
         * Request a swapchain resize. Thread-safe (deferred to next begin_frame).
         */
        auto request_resize(std::uint32_t width, std::uint32_t height) -> void;

        /**
         * Wait for all GPU work to complete.
         */
        auto wait_idle() -> void;

        // Accessors required by RenderGraph
        [[nodiscard]] auto get_device_ref() -> flux::Device & { return *m_device; }
        [[nodiscard]] auto get_instance() const -> VkInstance { return m_instance->get_handle(); }
        [[nodiscard]] auto get_physical_device() const -> VkPhysicalDevice { return m_device->get_physical_device(); }

        // Swapchain accessors
        [[nodiscard]] auto get_swapchain() -> flux::Swapchain & { return *m_swapchain; }
        [[nodiscard]] auto get_current_image() const -> VkImage;
        [[nodiscard]] auto get_current_image_view() const -> VkImageView;
        [[nodiscard]] auto get_swapchain_format() const -> VkFormat { return m_swapchain->get_format(); }
        [[nodiscard]] auto get_swapchain_extent() const -> VkExtent2D { return m_swapchain->get_extent(); }

        // Frame state
        [[nodiscard]] auto get_current_frame_index() const -> std::uint32_t { return m_current_frame; }
        [[nodiscard]] auto get_frame_number() const -> std::uint64_t { return m_frame_number; }

    private:
        auto handle_resize() -> void;

        // Vulkan infrastructure (ordered for destruction)
        std::unique_ptr<flux::Instance> m_instance;
        std::unique_ptr<flux::Surface> m_surface;
        std::unique_ptr<flux::Device> m_device;
        std::unique_ptr<flux::Swapchain> m_swapchain;

        // Per-frame resources
        std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_frames;

        // Frame state
        std::uint32_t m_current_frame = 0;
        std::uint32_t m_current_image_index = 0;
        std::uint64_t m_frame_number = 0;

        // Thread-safe resize request
        std::mutex m_resize_mutex;
        bool m_resize_pending = false;
        std::uint32_t m_resize_width = 0;
        std::uint32_t m_resize_height = 0;
    };
} // namespace thresh
