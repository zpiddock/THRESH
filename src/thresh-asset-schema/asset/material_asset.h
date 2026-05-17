//
// Created by shad0w on 17/05/2026.
//

#pragma once
#include <array>
#include <string>

namespace thresh::asset {
    struct MaterialAsset {

        std::string          albedo_path;
        std::array<float, 3> albedo_tint     = {1.0f, 1.0f, 1.0f};
        std::string          normal_path;
        std::string          metallic_path;
        std::string          roughness_path;
        std::string          emission_path;
    };
}
