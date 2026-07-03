//
// Created by Admin on 02/07/2026.
//

#pragma once
#include <span>
#include <type_traits>

#include <glaze/glaze.hpp>

#include "helix/math.hpp"

// The bridge relies on helix/glm types being trivially-copyable, tightly-packed floats.
// These guards also catch GLM_FORCE_ALIGNED / SIMD configs, which would insert padding.
static_assert(std::is_trivially_copyable_v<helix::float2>   && sizeof(helix::float2)   ==  2 * sizeof(float));
static_assert(std::is_trivially_copyable_v<helix::float3>   && sizeof(helix::float3)   ==  3 * sizeof(float));
static_assert(std::is_trivially_copyable_v<helix::float4>   && sizeof(helix::float4)   ==  4 * sizeof(float));
static_assert(std::is_trivially_copyable_v<helix::quat>     && sizeof(helix::quat)     ==  4 * sizeof(float));
static_assert(std::is_trivially_copyable_v<helix::float3x3> && sizeof(helix::float3x3) ==  9 * sizeof(float));
static_assert(std::is_trivially_copyable_v<helix::float4x4> && sizeof(helix::float4x4) == 16 * sizeof(float));

namespace thresh::asset::detail {
    // Fixed-extent float view over a packed math type; const-correct for both glaze paths
    // (write sees const self, read needs a mutable view).
    template <std::size_t N>
    inline constexpr auto float_span = [](auto& self) {
        using F = std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>,
                                     const float, float>;
        return std::span<F, N>{reinterpret_cast<F*>(&self), N};
    };
}

template<> struct glz::meta<helix::float2>   { static constexpr auto value = thresh::asset::detail::float_span<2>;  };
template<> struct glz::meta<helix::float3>   { static constexpr auto value = thresh::asset::detail::float_span<3>;  };
template<> struct glz::meta<helix::float4>   { static constexpr auto value = thresh::asset::detail::float_span<4>;  };
template<> struct glz::meta<helix::quat>     { static constexpr auto value = thresh::asset::detail::float_span<4>;  }; // xyzw memory order
template<> struct glz::meta<helix::float3x3> { static constexpr auto value = thresh::asset::detail::float_span<9>;  };
template<> struct glz::meta<helix::float4x4> { static constexpr auto value = thresh::asset::detail::float_span<16>; };
