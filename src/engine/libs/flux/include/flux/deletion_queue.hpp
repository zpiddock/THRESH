#pragma once

#include <vulkan/vulkan.h>
#include <functional>
#include <vector>
#include <deque>
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

/**
 * Deferred deletion queue for Vulkan resources.
 *
 * When a resource is "in flight" (being used by the GPU), we can't destroy it
 * immediately. The DeletionQueue holds resources until enough frames have passed
 * that we're certain the GPU is done with them.
 *
 * Usage:
 *   queue.push(pipeline, current_frame);  // Schedule for deletion
 *   queue.flush(current_frame);           // Call once per frame, deletes old resources
 */
class FLUX_API DeletionQueue {
public:
    explicit DeletionQueue(VkDevice device, uint32_t frames_in_flight = 2);
    ~DeletionQueue();

    // Non-copyable
    DeletionQueue(const DeletionQueue&) = delete;
    DeletionQueue& operator=(const DeletionQueue&) = delete;

    /**
     * Schedule a pipeline layout for deletion.
     */
    void push_pipeline_layout(VkPipelineLayout layout, uint64_t current_frame);

    /**
     * Schedule a shader object (VkShaderEXT) for deletion.
     * Requires the extension destroy function pointer since VkShaderEXT
     * is destroyed via an extension function, not a core Vulkan function.
     */
    void push_shader_object(VkShaderEXT shader, PFN_vkDestroyShaderEXT destroy_fn, uint64_t current_frame);

    /**
     * Schedule a descriptor set layout for deletion.
     */
    void push_descriptor_set_layout(VkDescriptorSetLayout layout, uint64_t current_frame);

    /**
     * Schedule a generic deletion function.
     */
    void push(std::function<void()> deleter, uint64_t current_frame);

    /**
     * Flush deletions that are safe (their frame has passed).
     * Call this once per frame with the current frame number.
     */
    void flush(uint64_t current_frame);

    /**
     * Flush ALL pending deletions immediately.
     * Only call this during shutdown when GPU is idle.
     */
    void flush_all();

    /**
     * Get number of pending deletions.
     */
    [[nodiscard]] auto pending_count() const -> size_t { return m_pending.size(); }

private:
    struct PendingDeletion {
        std::function<void()> deleter;
        uint64_t delete_after_frame;  // Safe to delete when current_frame > this
    };

    VkDevice m_device;
    uint32_t m_frames_in_flight;
    std::deque<PendingDeletion> m_pending;
};

} // namespace flux
