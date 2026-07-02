//
// Created by Admin on 02/07/2026.
//

#pragma once
#include <type_traits>

#include <glaze/glaze.hpp>

#include "helix/math.hpp"

static_assert(std::is_trivially_copyable_v<helix::float4>);
static_assert(sizeof(helix::float4) == 4 * sizeof(float));

template<>
struct glz::meta<helix::float2> {
    static constexpr auto value = [](auto& self) {
        return std::span<float, 2>{&self.x, 2};
    };
};

template<>
struct glz::meta<helix::float3> {
    static constexpr auto value = [](auto& self) {
        return std::span<float, 3>{&self.x, 3};
    };
};

template<>
struct glz::meta<helix::float4> {

    static constexpr auto value = [](auto& self) {
        return std::span<float, 4>{&self.x, 4};
    };
};

template<>
struct glz::meta<helix::float3x3> {
    static constexpr auto value = [](auto& self) {
        return std::span<float, 9>{&helix::value_ptr(self), 9};
    };
};

template<>
struct glz::meta<helix::float4x4> {
    static constexpr auto value = [](auto& self) {
        return std::span<float, 16>{&helix::value_ptr(self), 16};
    };
};