//
// Created by Admin on 28/06/2026.
//

#pragma once
#include <string>

#include "texture_codec.hpp"

namespace ferret {

    struct CookOptions {
        std::string texture_out_dir;
        TextureCodec codec = UASTC_BC7;
        bool should_compress = true;
    };
} // ferret
