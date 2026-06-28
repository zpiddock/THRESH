//
// Created by Admin on 28/06/2026.
//
#include <filesystem>
#include <cstdio>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "cook.hpp"
#include "substratum/filesystem/filesystem.hpp"
#include "thresh/asset/model_io.hpp"

auto main(int argc, char** argv) -> int {
    if (argc < 2) {
        std::println(stderr, "Usage: ferret-cli <cook|dump> ...");
        return 1;
    }

    const std::string_view cmd = argv[1];

    if (cmd == "cook") {
        if (argc < 4) {
            std::println(stderr, "Usage: ferret-cli cook <source_model> <out.tasset> [--textures <dir>] [--no-compress] [--codec rgba8/bc7]");
            return 1;
        }

        ferret::CookOptions options{};
        const std::filesystem::path src = argv[2], out = argv[3];

        for (int i = 4; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--textures") {
                options.texture_out_dir = argv[++i];
            } else if (arg == "--no-compress") {
                options.should_compress = false;
            } else if (arg == "--codec" && i + 1 < argc) {
                options.codec = (std::string_view{argv[++i]} == "bc7" ? ferret::TextureCodec::UASTC_BC7 : ferret::TextureCodec::RGBA8);
            }
        }

        const auto result = ferret::cook_model(src, out, options);
        if (!result.has_value()) {
            std::println(stderr, "Failed to cook model!, error: {}", result.error());
            return 2;
        }
        std::println(stdout, "Cooked model to {}!", result->output_path.string());
        return 0;
    }

    if (cmd == "dump") {
        if (argc < 3) {
            std::println(stderr, "Usage: ferret-cli dump <input.tasset>");
            return 1;
        }

        auto path = std::filesystem::path(argv[2]);
        auto bytes = substratum::read_file_bytes(path);

        auto model = thresh::asset::decode_thresh_model(bytes);
        if (!model.has_value()) {
            std::println(stderr, "Failed to decode model");
            return 2;
        }
        std::println("source_uri: {}", model->src_uri);
        std::println("nodes: {}  meshes: {}  materials: {}",
                     model->nodes.size(), model->meshes.size(), model->materials.size());
        for (const auto& n : model->nodes)                                      // grows as later steps fill the model
            std::println("  node '{}' parent={} mesh={}  T=({:.2f},{:.2f},{:.2f})",
                         n.name, n.parent_index, n.mesh, n.translation[0], n.translation[1], n.translation[2]);
        return 0;
    }

    std::println(stderr, "Unknown command '{}'", cmd);
    return 1;
}
