#include <catch2/catch_test_macros.hpp>
#include "Compression.hpp"
#include <vector>
#include <numeric>
#include <algorithm>

// Helper: verify compress(data) round-trips exactly
static void requireRoundTrip(const std::vector<uint8_t>& data) {
    auto compressed   = Compression::compress(data);
    auto decompressed = Compression::decompress(compressed);
    REQUIRE(decompressed.size() == data.size());
    REQUIRE(decompressed == data);
}

// =============================================================================
//  BASIC CORRECTNESS
// =============================================================================

TEST_CASE("Compression: Empty data compresses and decompresses to empty", "[compression][basic]") {
    std::vector<uint8_t> data;
    auto compressed = Compression::compress(data);
    REQUIRE(compressed.empty());
    REQUIRE(Compression::decompress(compressed).empty());
}

TEST_CASE("Compression: Single element yields one RLE pair", "[compression][basic]") {
    std::vector<uint8_t> data = {42};
    auto compressed = Compression::compress(data);
    REQUIRE(compressed.size() == 1);
    REQUIRE(compressed[0].type  == 42);
    REQUIRE(compressed[0].count == 1);
    requireRoundTrip(data);
}

TEST_CASE("Compression: Uniform run collapses to one pair", "[compression][basic]") {
    SECTION("5 identical values") {
        std::vector<uint8_t> data(5, 7);
        auto c = Compression::compress(data);
        REQUIRE(c.size() == 1);
        REQUIRE(c[0].type  == 7);
        REQUIRE(c[0].count == 5);
    }
    SECTION("All 256 values uniform") {
        for (int v = 0; v <= 255; ++v) {
            std::vector<uint8_t> data(8, static_cast<uint8_t>(v));
            auto c = Compression::compress(data);
            REQUIRE(c.size() == 1);
            REQUIRE(c[0].type  == static_cast<uint8_t>(v));
            REQUIRE(c[0].count == 8);
        }
    }
}

TEST_CASE("Compression: All distinct values — no merging", "[compression][basic]") {
    std::vector<uint8_t> data = {0, 1, 2, 3, 4, 5};
    auto c = Compression::compress(data);
    REQUIRE(c.size() == 6);
    for (size_t i = 0; i < 6; ++i) {
        REQUIRE(c[i].type  == static_cast<uint8_t>(i));
        REQUIRE(c[i].count == 1);
    }
    requireRoundTrip(data);
}

TEST_CASE("Compression: Mixed run pattern encodes correctly", "[compression][basic]") {
    //   {1,1} {2,3} {1,1} {3,2}  →  4 pairs
    std::vector<uint8_t> data = {1, 1, 2, 2, 2, 1, 3, 3};
    auto c = Compression::compress(data);
    REQUIRE(c.size() == 4);
    REQUIRE(c[0].type == 1); REQUIRE(c[0].count == 2);
    REQUIRE(c[1].type == 2); REQUIRE(c[1].count == 3);
    REQUIRE(c[2].type == 1); REQUIRE(c[2].count == 1);
    REQUIRE(c[3].type == 3); REQUIRE(c[3].count == 2);
    requireRoundTrip(data);
}

// =============================================================================
//  DECOMPRESS-ONLY CASES
// =============================================================================

TEST_CASE("Compression: Decompress pre-built RLE correctly", "[compression][decompress]") {
    SECTION("Three segments") {
        std::vector<RLEPair> rle = { {2, 1000}, {0, 5000}, {5, 100} };
        auto d = Compression::decompress(rle);
        REQUIRE(d.size() == 6100);
        REQUIRE(d[0]    == 2);
        REQUIRE(d[999]  == 2);
        REQUIRE(d[1000] == 0);
        REQUIRE(d[5999] == 0);
        REQUIRE(d[6000] == 5);
        REQUIRE(d[6099] == 5);
    }
    SECTION("Decompress empty RLE") {
        std::vector<RLEPair> rle;
        REQUIRE(Compression::decompress(rle).empty());
    }
    SECTION("Single pair count=1") {
        std::vector<RLEPair> rle = { {99, 1} };
        auto d = Compression::decompress(rle);
        REQUIRE(d.size() == 1);
        REQUIRE(d[0] == 99);
    }
}

// =============================================================================
//  ROUND-TRIP FIDELITY
// =============================================================================

TEST_CASE("Compression: Round-trip — all 256 block types", "[compression][roundtrip]") {
    // One of each byte value
    std::vector<uint8_t> data(256);
    std::iota(data.begin(), data.end(), 0);
    requireRoundTrip(data);
}

TEST_CASE("Compression: Round-trip — repeated full cycle", "[compression][roundtrip]") {
    // Repeat 0..255 multiple times: alternating runs
    std::vector<uint8_t> data;
    for (int rep = 0; rep < 20; ++rep)
        for (int v = 0; v < 256; ++v)
            data.push_back(static_cast<uint8_t>(v));
    REQUIRE(data.size() == 20 * 256);
    requireRoundTrip(data);
}

TEST_CASE("Compression: Round-trip — realistic chunk column (air + stone + dirt)", "[compression][roundtrip]") {
    // Typical voxel column: 0-3 bedrock, 4-40 stone, 41-44 dirt, 45-127 air
    std::vector<uint8_t> col;
    for (int y = 0; y <   4; ++y) col.push_back(7);  // BLOCK_BEDROCK
    for (int y = 0; y <  37; ++y) col.push_back(3);  // BLOCK_STONE
    for (int y = 0; y <   4; ++y) col.push_back(1);  // BLOCK_DIRT
    for (int y = 0; y < 83; ++y)  col.push_back(0);  // BLOCK_AIR
    REQUIRE(col.size() == 128);
    auto c = Compression::compress(col);
    REQUIRE(c.size() == 4);   // exactly 4 distinct runs
    requireRoundTrip(col);
}

TEST_CASE("Compression: Round-trip — random-looking alternating pattern", "[compression][roundtrip]") {
    std::vector<uint8_t> data;
    // Pseudo-random deterministic pattern
    uint32_t rng = 0xDEADBEEF;
    for (int i = 0; i < 2048; ++i) {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        data.push_back(static_cast<uint8_t>(rng & 0xFF));
    }
    requireRoundTrip(data);
}

// =============================================================================
//  COMPRESSION RATIO
// =============================================================================

TEST_CASE("Compression: Uniform data achieves maximum ratio", "[compression][ratio]") {
    // 65536 identical bytes should shrink to exactly 1 RLE pair
    const size_t N = 65536;
    std::vector<uint8_t> data(N, 3);
    auto c = Compression::compress(data);
    REQUIRE(c.size() == 1);
    REQUIRE(c[0].count == N);
    INFO("Compression ratio: 1 pair vs " << N << " bytes");
}

TEST_CASE("Compression: Worst-case (all unique) ratio = 1:1", "[compression][ratio]") {
    // Alternating 0,1,0,1,... — no run > 1 is merged
    const size_t N = 512;
    std::vector<uint8_t> data(N);
    for (size_t i = 0; i < N; ++i) data[i] = static_cast<uint8_t>(i & 1);
    auto c = Compression::compress(data);
    REQUIRE(c.size() == N);  // each element is its own RLE pair
    requireRoundTrip(data);
}

// =============================================================================
//  STRESS
// =============================================================================

TEST_CASE("Compression: Stress — 1 million uniform bytes", "[compression][stress]") {
    const size_t N = 1'000'000;
    std::vector<uint8_t> data(N, 15);
    auto c = Compression::compress(data);
    REQUIRE(c.size()    == 1);
    REQUIRE(c[0].count == N);
    REQUIRE(Compression::decompress(c) == data);
}

TEST_CASE("Compression: Stress — 50 000 worst-case all-different values", "[compression][stress]") {
    const size_t N = 50000;
    std::vector<uint8_t> data(N);
    for (size_t i = 0; i < N; ++i) data[i] = static_cast<uint8_t>(i % 256);
    auto c = Compression::compress(data);
    REQUIRE(c.size() == N);
    REQUIRE(Compression::decompress(c) == data);
}
