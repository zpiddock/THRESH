//
// Created by Admin on 28/06/2026.
//
#include <filesystem>
#include <print>

#include "cook.hpp"

auto main(int argc, char** argv) -> int {
    if (argc < 3) {
        std::println(stderr, "Usage: ferret-cli <source_model> <out.tasset> [--textures <dir>] [--no-compress] [--codec rgba8/bc7]");
        return 1;
    }

    ferret::CookOptions options{};
    const std::filesystem::path src = argv[1], out = argv[2];

    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--textures") {
            options.texture_out_dir = argv[++i];
        } else if (arg == "--no-compress") {
            options.should_compress = false;
        } else if (arg == "--codec" && i + 1 < argc) {
            options.codec = (std::string_view{argv[++i]} == "bc7" ? ferret::TextureCodec::UASTC_BC7 : ferret::TextureCodec::RGBA8);
        }
    }

    const auto result = false; // TODO: Do cooking....
    if (!result) {
        std::println(stderr, "Failed to cook model!");
        return 2;
    }
    std::println(stdout, "Cooked model!");
    return 0;
}
