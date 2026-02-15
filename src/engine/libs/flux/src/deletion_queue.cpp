#include "flux/deletion_queue.hpp"
#include <substratum/log.hpp>

namespace flux {

DeletionQueue::DeletionQueue(VkDevice device, uint32_t frames_in_flight)
    : m_device(device)
    , m_frames_in_flight(frames_in_flight)
{
    SUB_DEBUG("DeletionQueue created with {} frames in flight", frames_in_flight);
}

DeletionQueue::~DeletionQueue() {
    flush_all();
}

void DeletionQueue::push_pipeline_layout(VkPipelineLayout layout, uint64_t current_frame) {
    if (layout == VK_NULL_HANDLE) return;

    push([this, layout]() {
        ::vkDestroyPipelineLayout(m_device, layout, nullptr);
        SUB_TRACE("DeletionQueue: Destroyed pipeline layout");
    }, current_frame);
}

void DeletionQueue::push_shader_object(VkShaderEXT shader, PFN_vkDestroyShaderEXT destroy_fn, uint64_t current_frame) {
    if (shader == VK_NULL_HANDLE || destroy_fn == nullptr) return;

    push([this, shader, destroy_fn]() {
        destroy_fn(m_device, shader, nullptr);
        SUB_TRACE("DeletionQueue: Destroyed shader object");
    }, current_frame);
}

void DeletionQueue::push_descriptor_set_layout(VkDescriptorSetLayout layout, uint64_t current_frame) {
    if (layout == VK_NULL_HANDLE) return;

    push([this, layout]() {
        ::vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
        SUB_TRACE("DeletionQueue: Destroyed descriptor set layout");
    }, current_frame);
}

void DeletionQueue::push(std::function<void()> deleter, uint64_t current_frame) {
    m_pending.push_back({
        std::move(deleter),
        current_frame + m_frames_in_flight
    });
}

void DeletionQueue::flush(uint64_t current_frame) {
    while (!m_pending.empty()) {
        auto& front = m_pending.front();
        if (current_frame > front.delete_after_frame) {
            front.deleter();
            m_pending.pop_front();
        } else {
            break;  // Remaining items are newer, stop here
        }
    }
}

void DeletionQueue::flush_all() {
    for (auto& pending : m_pending) {
        pending.deleter();
    }
    m_pending.clear();
    SUB_DEBUG("DeletionQueue: Flushed all pending deletions");
}

} // namespace flux
