//
// Created by Admin on 28/06/2026.
//
#include <filesystem>
#include <cstdio>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "CLI/CLI.hpp"

#include "cook.hpp"
#include "texture_bake.hpp"
#include "substratum/filesystem/filesystem.hpp"
#include "thresh/asset/model_io.hpp"


auto register_cook_app(CLI::App& base_app) -> void {
    struct CookArgs {
        std::filesystem::path input;
        std::filesystem::path output;
        std::filesystem::path textures_dir;
        bool compress = true;
        bool is_bc7 = false;
    };

    auto args = std::make_shared<CookArgs>();

    auto cook_app = base_app.add_subcommand(
        "cook",
        "Cooks a singular asset in to a thresh .tasset"
        );
    cook_app->add_option("input", args->input, "Input asset path")->required()->check(CLI::ExistingFile);
    cook_app->add_option("output", args->output, "Output thresh asset path")->required();
    cook_app->add_option("--textures", args->textures_dir, "Directory to output textures to")->check(CLI::ExistingDirectory);
    cook_app->add_flag("--nocompress", args->compress, "Do not compress the thresh asset (defaults to true)")->default_val(true);
    cook_app->add_flag("--BC7", args->is_bc7, "Use BC7 compression for textures (defaults to RGBA8)")->default_val(false);

    cook_app->callback([args]() {
        ferret::CookOptions options{};
        options.texture_out_dir = args->textures_dir.string();
        options.should_compress = args->compress;
        options.codec = args->is_bc7 ? ferret::TextureCodec::UASTC_BC7 : ferret::TextureCodec::RGBA8;

        const auto result = ferret::cook_model(args->input, args->output, options);
        if (!result.has_value()) {
            std::println(stderr, "Failed to cook model!, error: {}", result.error());
        }
        std::println(stdout, "Cooked model to {}!", result->output_path.string());
    });
}

auto register_cook_defaults_command(CLI::App& base_app) -> void {

    struct DefaultsArgs {
        std::filesystem::path textures_dir;
    };
    auto args = std::make_shared<DefaultsArgs>();

    auto* defaults_app = base_app.add_subcommand(
        "cook-defaults",
        "Cooks the built-in default textures into a directory");
    defaults_app->add_option("textures", args->textures_dir, "Directory to output default textures to")
        ->required()->check(CLI::ExistingDirectory);

    defaults_app->callback([args]() {                       // by value — keeps args alive through parse
        ferret::cook_default_textures(args->textures_dir.string());
        std::println(stdout, "Cooked default textures to {}", args->textures_dir.string());
    });
}

auto register_dump_command(CLI::App& base_app) -> void {

     struct DumpArgs {
        std::filesystem::path input;
    };
    auto args = std::make_shared<DumpArgs>();

    auto* dump_app = base_app.add_subcommand(
        "dump",
        "Dumps the contents of a thresh .tasset");
    dump_app->add_option("input", args->input, "Input thresh asset path")
        ->required()->check(CLI::ExistingFile);

    dump_app->callback([args]() {
        const auto bytes = substratum::read_file_bytes(args->input);

        const auto model = thresh::asset::decode_thresh_model(bytes);
        if (!model.has_value()) {
            std::println(stderr, "Failed to decode model");
            return;                                          // callback returns void; no exit codes here
        }

        std::println("source_uri: {}", model->src_uri);
        std::println("nodes: {}  meshes: {}  materials: {}",
                     model->nodes.size(), model->meshes.size(), model->materials.size());
        for (const auto& n : model->nodes) {
            std::println("  node '{}' parent={} mesh={}  T=({:.2f},{:.2f},{:.2f})",
                         n.name, n.parent_index, n.mesh, n.translation[0], n.translation[1], n.translation[2]);
        }
        std::println("meshes:");
        for (std::size_t i = 0; i < model->meshes.size(); ++i) {
            const auto& mesh = model->meshes[i];
            std::println("  mesh[{}] layout={} index_type={} vertices={} indices={} submeshes={}",
                         i, static_cast<int>(mesh.layout), static_cast<int>(mesh.index_type),
                         mesh.vertex_count, mesh.index_count, mesh.submeshes.size());
            std::println("    aabb min=({:.2f},{:.2f},{:.2f}) max=({:.2f},{:.2f},{:.2f})",
                         mesh.aabb_min[0], mesh.aabb_min[1], mesh.aabb_min[2],
                         mesh.aabb_max[0], mesh.aabb_max[1], mesh.aabb_max[2]);
            for (std::size_t s = 0; s < mesh.submeshes.size(); ++s) {
                const auto& sub = mesh.submeshes[s];
                std::println("    submesh[{}] index_offset={} index_count={} material={}",
                             s, sub.index_offset, sub.index_count, sub.material_index);
            }
        }
    });
}

auto main(int argc, char** argv) -> int {

    CLI::App app{"Ferret CLI"};
    app.require_subcommand(1);

    register_cook_app(app);
    register_cook_defaults_command(app);
    register_dump_command(app);

    CLI11_PARSE(app, argc, argv);
    return 0;
}