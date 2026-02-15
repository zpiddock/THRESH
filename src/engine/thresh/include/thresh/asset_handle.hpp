#pragma once

#include <cstdint>
#include <functional>

namespace thresh {

    /**
     * Type-safe handle for assets managed by the AssetSystem.
     *
     * Contains an index (slot in the asset array) and a generation counter
     * (incremented each time a slot is reused). This detects dangling handles:
     * if the generation doesn't match, the handle is stale.
     *
     * @tparam T The asset type this handle refers to (e.g. flux::Mesh, flux::Texture)
     */
    template <typename T>
    struct AssetHandle {
        static constexpr std::uint32_t INVALID_INDEX = ~0u;
        static constexpr std::uint32_t INVALID_GENERATION = 0u;

        std::uint32_t index = INVALID_INDEX;
        std::uint32_t generation = INVALID_GENERATION;

        [[nodiscard]] auto is_valid() const -> bool {
            return index != INVALID_INDEX && generation != INVALID_GENERATION;
        }

        auto operator==(const AssetHandle &other) const -> bool = default;
        auto operator!=(const AssetHandle &other) const -> bool = default;
    };

} // namespace thresh

// Hash support for unordered containers
template <typename T>
struct std::hash<thresh::AssetHandle<T>> {
    auto operator()(const thresh::AssetHandle<T> &h) const noexcept -> std::size_t {
        auto h1 = std::hash<std::uint32_t>{}(h.index);
        auto h2 = std::hash<std::uint32_t>{}(h.generation);
        return h1 ^ (h2 << 16);
    }
};
