//
// Created by Admin on 15/06/2026.
//

#pragma once
#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

#include "buffer.hpp"

namespace helix {

    class ThreshVkDevice;
    // Strong index type. Value == the index DescriptorHandle<T> consumes in shaders,
    // in units of the cook-time stride. Never a byte offset.
    enum class HeapSlot : std::uint32_t {};
    inline constexpr auto HEAP_INVALID_SLOT = HeapSlot{~0u};

    class DescriptorHeap {

        public:
            enum class Kind {
                RESOURCE,
                SAMPLER,
            };

            DescriptorHeap(ThreshVkDevice& device, Kind kind, uint32_t capacity);

            [[nodiscard]] auto allocate() -> HeapSlot;
            auto release(HeapSlot slot) -> void;

            auto write_sampled_image(HeapSlot slot, const vk::ImageViewCreateInfo& view, vk::ImageLayout layout) -> void;
            auto write_sampler(HeapSlot slot, const vk::SamplerCreateInfo& sampler) -> void;

            // Shaders consume this
            // Identity today - exists so that if the index<->offset
            // relationship ever changes, this is the single line that changes with it.
            [[nodiscard]] static auto shader_index(const HeapSlot slot) -> uint32_t {
                return std::to_underlying(slot);
            }

            [[nodiscard]] auto bind_info() const -> vk::BindHeapInfoEXT;


        private:
            // THE single point where a slot becomes a byte offset. The reserved range
            // occupies whole slots at the front, so this stays multiplication-only;
            // allocate() simply never hands out an index below m_first_index.
            [[nodiscard]] auto byte_offset(HeapSlot slot) const -> vk::DeviceSize {
                return vk::DeviceSize{std::to_underlying(slot)} * m_stride;
            }

            ThreshVkDevice&       m_device;
            Kind                  m_kind;
            Buffer                m_storage;
            vk::DeviceSize        m_stride        = 0;
            vk::DeviceSize        m_reserved_size = 0;   // implementation-reserved, offset 0
            uint32_t              m_first_index   = 0;   // first allocatable slot
            std::vector<uint32_t> m_free;                // — allocator state yours —
    };
} // flux
