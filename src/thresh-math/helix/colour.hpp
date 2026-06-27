#pragma once

#include <cstdint>
#include <string_view>

#include "math.hpp"

namespace helix {

    struct Colour {
        float r{0.0f};
        float g{0.0f};
        float b{0.0f};
        float a{1.0f};

        constexpr Colour() = default;
        constexpr Colour(const float r_, const float g_, const float b_, const float a_ = 1.0f)
            : r{r_}, g{g_}, b{b_}, a{a_} {}

        // GLM/alias interop -----------------------------------------------------
        explicit constexpr Colour(const float4& v) : r{v.x}, g{v.y}, b{v.z}, a{v.w} {}

        explicit constexpr Colour(const float3& v, const float a_ = 1.0f)
            : r{v.x}, g{v.y}, b{v.z}, a{a_} {}

        explicit constexpr Colour(const float2& v) : r {v.x}, g{v.y}, b{1.0f} {}

        explicit constexpr Colour(const float v) : r{v}, g{v}, b{v}, a{v} {}

        explicit constexpr operator float4() const { return {r, g, b, a}; }
        [[nodiscard]] constexpr float3 rgb() const { return {r, g, b}; }

        // Raw pointer for Vulkan/ImGui uploads ----------------------------------
        [[nodiscard]] const float* data() const { return &r; }
        float* data() { return &r; }
    };

    // Hex helpers ---------------------------------------------------------------
    // 0xRRGGBB  (alpha defaults to 1.0)
    constexpr Colour from_rgb(const std::uint32_t hex) {
        return {
            ((hex >> 16) & 0xFF) / 255.0f,
            ((hex >>  8) & 0xFF) / 255.0f,
            ( hex        & 0xFF) / 255.0f,
            1.0f
        };
    }

    // 0xRRGGBBAA
    constexpr Colour from_rgba(const std::uint32_t hex) {
        return {
            ((hex >> 24) & 0xFF) / 255.0f,
            ((hex >> 16) & 0xFF) / 255.0f,
            ((hex >>  8) & 0xFF) / 255.0f,
            ( hex        & 0xFF) / 255.0f
        };
    }

    // "#RRGGBB" / "RRGGBB" / "#RRGGBBAA" string parse (constexpr-friendly)
    constexpr Colour from_hex(std::string_view s) {
        if (!s.empty() && s.front() == '#') s.remove_prefix(1);

        auto nibble = [](char c) -> std::uint32_t {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };

        std::uint32_t value = 0;
        for (const char c : s) value = (value << 4) | nibble(c);

        return s.size() == 8 ? from_rgba(value) : from_rgb(value);
    }

} // namespace flux::math