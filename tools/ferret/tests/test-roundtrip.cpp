//
// Created by Admin on 28/06/2026.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <thresh/asset/model_asset.hpp>
#include <thresh/asset/model_io.hpp>

#include "cook.hpp"
using namespace thresh::asset;

TEST_CASE("header rejects bad magic and version") {
    auto  bytes = *ferret::encode_thresh_model(ModelAsset{}, /*compress*/ false);
    auto* h     = reinterpret_cast<ModelFileHeader*>(bytes.data());
    SUBCASE("magic")   { h->magic   = {'X','X','X','X'}; CHECK_FALSE(decode_thresh_model(bytes).has_value()); }
    SUBCASE("version") { h->version = 999;               CHECK_FALSE(decode_thresh_model(bytes).has_value()); }
    SUBCASE("truncated") { bytes.resize(8);              CHECK_FALSE(decode_thresh_model(bytes).has_value()); }
}

TEST_CASE("empty ModelAsset round-trips (both framings)") {
    for (bool compress : {false, true}) {
        ModelAsset in{ .src_uri = "unit-test", .meshes = {}, .materials = {}, .nodes = {} };
        auto encoded = ferret::encode_thresh_model(in, compress);
        REQUIRE(encoded.has_value());
        auto out = decode_thresh_model(*encoded);
        REQUIRE(out.has_value());
        CHECK(out->src_uri == "unit-test");
        CHECK(out->meshes.empty());
        CHECK(out->nodes.empty());
    }
}

TEST_CASE("compressible payload round-trips (compressed_size < payload_size)") {
    // A large run of zero-bytes shrinks hard under zstd, so compressed_size < payload_size.
    // This is the case the SHORT_PAYLOAD bound got wrong: the on-disk file is only
    // sizeof(header)+compressed_size, but the check used to demand +payload_size.
    MeshEntry mesh{};
    mesh.vertex_count = 1024;
    mesh.vertices.assign(mesh.vertex_count * sizeof(MeshVertex), std::uint8_t{0});
    ModelAsset in{ .src_uri = "compressible", .meshes = { mesh }, .materials = {}, .nodes = {} };

    auto encoded = ferret::encode_thresh_model(in, /*compress*/ true);
    REQUIRE(encoded.has_value());

    ModelFileHeader h{};
    std::memcpy(&h, encoded->data(), sizeof h);
    REQUIRE((h.flags & 1u) != 0u);              // actually compressed
    CHECK(h.compressed_size < h.payload_size);  // actually shrank (else it wouldn't catch the bug)
    CHECK(encoded->size() == sizeof(ModelFileHeader) + h.compressed_size);

    auto out = decode_thresh_model(*encoded);
    REQUIRE(out.has_value());
    REQUIRE(out->meshes.size() == 1);
    CHECK(out->meshes[0].vertex_count == 1024);
    CHECK(out->meshes[0].vertices == in.meshes[0].vertices);
}

TEST_CASE("model_content_hash is stable for identical input") {
    auto a = *ferret::encode_thresh_model(ModelAsset{ .src_uri = "x", .meshes = {}, .materials = {}, .nodes = {} }, false);
    auto b = *ferret::encode_thresh_model(ModelAsset{ .src_uri = "x", .meshes = {}, .materials = {}, .nodes = {} }, false);
    ModelFileHeader ha{}, hb{};
    std::memcpy(&ha, a.data(), sizeof ha);  std::memcpy(&hb, b.data(), sizeof hb);
    CHECK(ha.model_content_hash == hb.model_content_hash);
    CHECK(ha.model_content_hash != 0);
}