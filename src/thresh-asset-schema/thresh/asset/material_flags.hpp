#pragma once
#include <cstdint>

namespace thresh::asset {

    inline constexpr std::uint32_t MATERIAL_FLAG_ALPHA_MASK   = 1u << 0;
    inline constexpr std::uint32_t MATERIAL_FLAG_DOUBLE_SIDED = 1u << 1;
    inline constexpr std::uint32_t MATERIAL_FLAG_HAS_NORMAL   = 1u << 2;
    inline constexpr std::uint32_t MATERIAL_FLAG_HAS_EMISSIVE = 1u << 3;

    inline constexpr std::uint64_t WHITE_HASH       = 0x749b54e677a157f6ull;
    inline constexpr std::uint64_t FLAT_NORMAL_HASH = 0x196798cf7c6f4e49ull;
}
