//
// Created by Admin on 28/06/2026.
//

#pragma once
#include <cstdint>
#include <span>

namespace ferret {

    auto hash_bytes(std::span<const std::uint8_t> bytes) -> std::uint64_t;
} // ferret
