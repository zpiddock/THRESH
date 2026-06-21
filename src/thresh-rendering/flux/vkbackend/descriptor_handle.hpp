//
// Created by Admin on 15/06/2026.
//

#pragma once
#include <cstdint>

namespace flux::gpu {
    struct alignas(8) DescriptorHandle {
        std::uint32_t index;
        std::uint32_t _zero; // Slang's uint->handle constructor zero-extends; keep zero.
        static auto make(const uint32_t shader_index) -> DescriptorHandle {
            return {shader_index, 0};
        }
    };
    static_assert(sizeof(DescriptorHandle) == 8);
}
