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
#include "texture_bake.hpp"
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

    if (cmd == "cook-defaults") {
        if (argc < 3) {
            std::println(stderr, "Usage: ferret-cli cook-defaults <textures_dir>");
            return 1;
        }
        ferret::cook_default_textures(argv[2]);
        std::println(stdout, "Cooked default textures to {}", argv[2]);
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
        for (const auto& n : model->nodes) {
            // grows as later steps fill the model
            std::println("  node '{}' parent={} mesh={}  T=({:.2f},{:.2f},{:.2f})",
                         n.name, n.parent_index, n.mesh, n.translation[0], n.translation[1], n.translation[2]);
        }
        std::println("meshes:");

        for (std::size_t i = 0; i < model->meshes.size(); ++i) {
            const auto& mesh = model->meshes[i];
            std::println("  mesh[{}] layout={} index_type={} vertices={} indices={} submeshes={}",
                         i,
                         static_cast<int>(mesh.layout),
                         static_cast<int>(mesh.index_type),
                         mesh.vertex_count,
                         mesh.index_count,
                         mesh.submeshes.size());

            std::println("    aabb min=({:.2f},{:.2f},{:.2f}) max=({:.2f},{:.2f},{:.2f})",
                         mesh.aabb_min[0], mesh.aabb_min[1], mesh.aabb_min[2],
                         mesh.aabb_max[0], mesh.aabb_max[1], mesh.aabb_max[2]);

            for (std::size_t s = 0; s < mesh.submeshes.size(); ++s) {
                const auto& sub = mesh.submeshes[s];
                std::println("    submesh[{}] index_offset={} index_count={} material={}",
                             s, sub.index_offset, sub.index_count, sub.material_index);
            }
        }
        return 0;
    }

    std::println(stderr, "Unknown command '{}'", cmd);
    return 1;
}
