//
// Created by Admin on 28/06/2026.
//

#include "filesystem.hpp"

#include <cstdint>
#include <fstream>

namespace substratum {
    auto read_file_bytes(const std::filesystem::path& path) -> std::vector<uint8_t> {
        auto file = std::ifstream(path, std::ios::binary);
        if (!file.is_open()) {
            return {};
        }

        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        if (size <= 0) {
            return {};
        }

        std::vector<uint8_t> bytes(size);
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

        if (!file) {
            return {};
        }

        return bytes;
    }
} // substratum
