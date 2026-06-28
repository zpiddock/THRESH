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

TEST_CASE("model_content_hash is stable for identical input") {
    auto a = *ferret::encode_thresh_model(ModelAsset{ .src_uri = "x", .meshes = {}, .materials = {}, .nodes = {} }, false);
    auto b = *ferret::encode_thresh_model(ModelAsset{ .src_uri = "x", .meshes = {}, .materials = {}, .nodes = {} }, false);
    ModelFileHeader ha{}, hb{};
    std::memcpy(&ha, a.data(), sizeof ha);  std::memcpy(&hb, b.data(), sizeof hb);
    CHECK(ha.model_content_hash == hb.model_content_hash);
    CHECK(ha.model_content_hash != 0);
}