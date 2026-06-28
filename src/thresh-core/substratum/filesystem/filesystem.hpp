//
// Created by Admin on 28/06/2026.
//

#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

namespace substratum {

    auto read_file_bytes(const std::filesystem::path& path) -> std::vector<uint8_t>;
} // substratum
