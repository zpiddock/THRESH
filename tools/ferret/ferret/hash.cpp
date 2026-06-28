//
// Created by Admin on 28/06/2026.
//

#include "hash.hpp"

#define XXH_INLINE_ALL
#include "xxhash.h"

namespace ferret {
    auto hash_bytes(std::span<const std::uint8_t> bytes) -> std::uint64_t {

        return XXH3_64bits(bytes.data(), bytes.size());
    }
} // ferret