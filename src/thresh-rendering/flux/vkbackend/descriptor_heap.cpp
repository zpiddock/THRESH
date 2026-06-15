//
// Created by Admin on 15/06/2026.
//

#include "descriptor_heap.hpp"

#include "substratum/log.hpp"

namespace flux {
    DescriptorHeap::DescriptorHeap(ThreshVkDevice& device, Kind kind, uint32_t capacity) :
    m_device(device),
    m_kind(kind),
    m_stride(
        kind == Kind::RESOURCE ?
        THRESH_RESOURCE_HEAP_STRIDE :
        THRESH_SAMPLER_HEAP_STRIDE) {

        const auto& props = device.heap_properties();

        // VU: reservedRangeSize >= minResourceHeapReservedRange (resp. sampler). Park it at
        // offset 0 and round up to whole slots so allocatable indices start on a stride
        // boundary - keeps byte_offset() multiplication-only.
        const vk::DeviceSize min_reserved = (kind == Kind::RESOURCE)
                ? props.minResourceHeapReservedRange
                : props.minSamplerHeapReservedRange;
        m_reserved_size = (min_reserved + m_stride - 1) / m_stride * m_stride;
        m_first_index   = static_cast<uint32_t>(m_reserved_size / m_stride);

        m_storage = Buffer(device, {
            .size  = m_reserved_size + vk::DeviceSize{capacity} * m_stride,
            .usage = vk::BufferUsageFlagBits::eDescriptorHeapEXT
                   | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            // Descriptors are written by the HOST into the live heap, so it must be mapped.
            // DEVICE_LOCAL|HOST_VISIBLE = ReBAR memory
            // Fallback if find_memory_type misses: drop DEVICE_LOCAL (correct, slower fetches).
            .memory = vk::MemoryPropertyFlagBits::eDeviceLocal
                    | vk::MemoryPropertyFlagBits::eHostVisible
                    | vk::MemoryPropertyFlagBits::eHostCoherent,
            .persistent_map = true,
            .debug_name     = kind == Kind::RESOURCE ? "resource_heap" : "sampler_heap",
        });

        // VU: heapRange.address must be a multiple of resourceHeapAlignment/samplerHeapAlignment.
        // Buffer base addresses are typically 256-aligned. if this ever fires, the fix is to
        // over-allocate and align the range start inside the buffer.
        const auto alignment = (kind == Kind::RESOURCE) ? props.resourceHeapAlignment
                                                        : props.samplerHeapAlignment;
        if (m_storage.device_address() % alignment != 0) {
            SUB_FATAL("Heap buffer address not {}-aligned — implement in-buffer range alignment", alignment);
        }

        // Free list over [m_first_index, m_first_index + capacity). Pushed high-to-low so the
        // first allocate() (pop_back) hands out m_first_index - allocation climbs from the front.
        m_free.reserve(capacity);
        for (uint32_t i = capacity; i-- > 0; ) m_free.push_back(m_first_index + i);
    }

    auto DescriptorHeap::allocate() -> HeapSlot {

        if (m_free.empty()) {
            SUB_FATAL("{} Descriptor Heap Exhausted", m_kind == Kind::RESOURCE ? "Resource" : "Sampler");
        }
        const uint32_t index = m_free.back();
        m_free.pop_back();
        return HeapSlot{index};
    }

    auto DescriptorHeap::release(HeapSlot slot) -> void {

        // Immediate recycle is safe ONLY when the GPU can't still reference the slot. true at
        // shutdown and on the idle swapchain-recreate path, which is all the engine does today.
        // To release a slot mid-frame, defer for now (tag with the frame index, return to the
        // free list MAX_FRAMES_IN_FLIGHT frames later)
        m_free.push_back(std::to_underlying(slot));
    }

    auto DescriptorHeap::write_sampled_image(HeapSlot slot, const vk::ImageViewCreateInfo& view,
        vk::ImageLayout layout) -> void {

        assert(m_kind == Kind::RESOURCE && std::to_underlying(slot) >= m_first_index);

        const vk::ImageDescriptorInfoEXT image_info {
            .pView = &view,
            .layout = layout
        };
        const vk::ResourceDescriptorInfoEXT resource_info {
            .type = vk::DescriptorType::eSampledImage,
            .data = { &image_info },
        };
        const vk::HostAddressRangeEXT dst {
            .address = m_storage.mapped().data() + byte_offset(slot),
            .size = m_stride
        };
        m_device.logical().writeResourceDescriptorsEXT(resource_info, dst);
    }

    auto DescriptorHeap::write_sampler(HeapSlot slot, const vk::SamplerCreateInfo& sampler) -> void {

        assert(m_kind == Kind::SAMPLER && std::to_underlying(slot) >= m_first_index);
        const vk::HostAddressRangeEXT dst{
            .address = m_storage.mapped().data() + byte_offset(slot),
            .size    = m_stride,
        };
        m_device.logical().writeSamplerDescriptorsEXT(sampler, dst);
    }

    auto DescriptorHeap::bind_info() const -> vk::BindHeapInfoEXT {

        return {
            .heapRange = {.address = m_storage.device_address(), .size = m_storage.size()},
            .reservedRangeOffset = 0,
            .reservedRangeSize = m_reserved_size
        };
    }
} // flux