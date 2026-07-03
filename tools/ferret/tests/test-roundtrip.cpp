//
// Created by Admin on 28/06/2026.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <thresh/asset/model_asset.hpp>
#include <thresh/asset/model_io.hpp>

#include "cook.hpp"
using namespace thresh::asset;

namespace ferret_tests {   // named: glaze reflection needs the type to have linkage
    struct BridgeProbe {
        helix::float2   a{1.f, 2.f};
        helix::float3   b{3.f, 4.f, 5.f};
        helix::float4   c{6.f, 7.f, 8.f, 9.f};
        helix::float4x4 m{2.f};                       // scaled identity — asymmetric enough
        helix::AABB     box{{-1.f, -2.f, -3.f}, {1.f, 2.f, 3.f}};
    };
}
using ferret_tests::BridgeProbe;

TEST_CASE("helix types round-trip through BEVE bitwise") {
    BridgeProbe in{};
    in.m[3] = {10.f, 11.f, 12.f, 1.f};                // translation column — catches transposition

    std::string beve;
    REQUIRE_FALSE(glz::write_beve(in, beve));

    BridgeProbe out{helix::float2{}, helix::float3{}, helix::float4{}, helix::float4x4{0.f}, {}};
    REQUIRE_FALSE(glz::read_beve(out, beve));

    CHECK(std::memcmp(&in.a, &out.a, sizeof in.a) == 0);
    CHECK(std::memcmp(&in.b, &out.b, sizeof in.b) == 0);
    CHECK(std::memcmp(&in.c, &out.c, sizeof in.c) == 0);
    CHECK(std::memcmp(&in.m, &out.m, sizeof in.m) == 0);
    CHECK(std::memcmp(&in.box, &out.box, sizeof in.box) == 0);
}

TEST_CASE("header rejects bad magic and version") {
    auto  bytes = *ferret::encode_thresh_model(ModelAsset{}, /*compress*/ false);
    auto* h     = reinterpret_cast<ModelFileHeader*>(bytes.data());
    SUBCASE("magic")   { h->magic   = {'X','X','X','X'}; CHECK_FALSE(decode_thresh_model(bytes).has_value()); }
    SUBCASE("version") { h->version = 999;               CHECK_FALSE(decode_thresh_model(bytes).has_value()); }
    SUBCASE("truncated") { bytes.resize(8);              CHECK_FALSE(decode_thresh_model(bytes).has_value()); }
}

TEST_CASE("empty ModelAsset round-trips (both framings)") {
    for (bool compress : {false, true}) {
        ModelAsset in{};
        in.name = "unit-test";
        auto encoded = ferret::encode_thresh_model(in, compress);
        REQUIRE(encoded.has_value());
        auto out = decode_thresh_model(*encoded);
        REQUIRE(out.has_value());
        CHECK(out->name == "unit-test");
        CHECK(out->submeshes.empty());
        CHECK(out->vertices.empty());
        CHECK_FALSE(out->aabb.valid());               // the invalid sentinel survives the trip
    }
}

TEST_CASE("compressible payload round-trips (compressed_size < payload_size)") {
    // A large run of zero-bytes shrinks hard under zstd, so compressed_size < payload_size.
    // This is the case the SHORT_PAYLOAD bound got wrong: the on-disk file is only
    // sizeof(header)+compressed_size, but the check used to demand +payload_size.
    ModelAsset in{};
    in.name = "compressible";
    in.vertices.assign(1024, MeshVertex{});
    in.vertex_count = 1024;

    auto encoded = ferret::encode_thresh_model(in, /*compress*/ true);
    REQUIRE(encoded.has_value());

    ModelFileHeader h{};
    std::memcpy(&h, encoded->data(), sizeof h);
    REQUIRE((h.flags & 1u) != 0u);              // actually compressed
    CHECK(h.compressed_size < h.payload_size);  // actually shrank (else it wouldn't catch the bug)
    CHECK(encoded->size() == sizeof(ModelFileHeader) + h.compressed_size);

    auto out = decode_thresh_model(*encoded);
    REQUIRE(out.has_value());
    CHECK(out->vertex_count == 1024);
    REQUIRE(out->vertices.size() == 1024);
    CHECK(std::memcmp(out->vertices.data(), in.vertices.data(), 1024 * sizeof(MeshVertex)) == 0);
}

TEST_CASE("two submeshes share one index range with distinct transforms") {
    ModelAsset in{};
    in.name = "instanced";
    in.vertex_count = 3;
    in.index_count  = 3;
    in.vertices.assign(3, MeshVertex{});
    in.indices  = { 0, 1, 2 };
    const auto a = helix::float4x4{1.f};                                     // identity
    const auto b = helix::translate(helix::float4x4{1.f}, {5.f, 0.f, 0.f});
    const helix::AABB box{{-1.f, -1.f, -1.f}, {1.f, 1.f, 1.f}};
    in.submeshes.push_back({ .local = a, .aabb = box, .index_offset = 0, .index_count = 3, .material_index = 0 });
    in.submeshes.push_back({ .local = b, .aabb = box, .index_offset = 0, .index_count = 3, .material_index = 0 }); // same range, new placement

    const auto out = decode_thresh_model(*ferret::encode_thresh_model(in, true));
    REQUIRE(out.has_value());
    REQUIRE(out->submeshes.size() == 2);
    CHECK(out->submeshes[0].index_offset == out->submeshes[1].index_offset);
    CHECK(std::memcmp(&out->submeshes[1].local, &b, sizeof b) == 0);         // bitwise — exercises the bridge
}

TEST_CASE("model_content_hash is stable for identical input") {
    ModelAsset probe{};
    probe.name = "x";
    auto a = *ferret::encode_thresh_model(probe, false);
    auto b = *ferret::encode_thresh_model(probe, false);
    ModelFileHeader ha{}, hb{};
    std::memcpy(&ha, a.data(), sizeof ha);  std::memcpy(&hb, b.data(), sizeof hb);
    CHECK(ha.model_content_hash == hb.model_content_hash);
    CHECK(ha.model_content_hash != 0);
}
