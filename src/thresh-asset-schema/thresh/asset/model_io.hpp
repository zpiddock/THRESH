#pragma once
#include <cstddef>
#include <expected>
#include <span>

#include <zstd.h>
#include <glaze/glaze.hpp>

#include "model_asset.hpp"
#include "xxhash.h"

namespace thresh::asset {

    enum class DecodeError {
        BAD_FILE,
        BAD_MAGIC,
        BAD_VERSION,
        SHORT_PAYLOAD,
        DECOMPRESSION_FAILED,
        PAYLOAD_SIZE_MISMATCH,
        GLAZE_ERROR
    };

    inline auto decode_thresh_model(const std::span<const std::uint8_t> data) -> std::expected<ModelAsset, DecodeError> {
        if (data.size() < sizeof(ModelFileHeader)) {
            return std::unexpected(DecodeError::BAD_FILE);
        }

        ModelFileHeader header{};
        std::memcpy(&header, data.data(), sizeof(ModelFileHeader));
        if (header.magic != TMODEL_MAGIC) {
            return std::unexpected(DecodeError::BAD_MAGIC);
        }
        if (header.version != TMODEL_VERSION) {
            return std::unexpected(DecodeError::BAD_VERSION);
        }
        if (data.size() < sizeof(ModelFileHeader) + header.compressed_size) {
            return std::unexpected(DecodeError::SHORT_PAYLOAD);
        }

        const std::span compressed_payload(data.data() + sizeof(ModelFileHeader), header.compressed_size);
        std::vector<std::uint8_t> decompressed_payload(header.payload_size);

        if (header.flags & 1u) { // is zstd compressed
            const std::size_t bytes_decoded =
                ZSTD_decompress(
                    decompressed_payload.data(),
                    decompressed_payload.size(),
                    compressed_payload.data(),
                    compressed_payload.size()
                    );
            if (ZSTD_isError(bytes_decoded) || bytes_decoded != header.payload_size) {
                return std::unexpected(DecodeError::DECOMPRESSION_FAILED);
            }
        } else {
            if (compressed_payload.size() != header.payload_size) {
                return std::unexpected(DecodeError::PAYLOAD_SIZE_MISMATCH);
            }
            decompressed_payload.assign(compressed_payload.begin(), compressed_payload.end());
        }

        ModelAsset asset;
        if (auto error = glz::read_beve(asset, decompressed_payload)) {
            std::println("Glaze error: {}", glz::format_error(error, decompressed_payload));
            return std::unexpected(DecodeError::GLAZE_ERROR);
        }

        return asset;
    }
}
