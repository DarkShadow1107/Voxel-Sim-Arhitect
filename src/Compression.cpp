#include "Compression.hpp"

std::vector<RLEPair> Compression::compress(const std::vector<uint8_t>& data) {
    std::vector<RLEPair> compressed;
    if (data.empty()) return compressed;

    uint8_t currentType = data[0];
    uint32_t currentCount = 1;

    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i] == currentType) {
            currentCount++;
        } else {
            compressed.push_back({currentType, currentCount});
            currentType = data[i];
            currentCount = 1;
        }
    }
    compressed.push_back({currentType, currentCount});

    return compressed;
}

std::vector<uint8_t> Compression::decompress(const std::vector<RLEPair>& compressedData) {
    std::vector<uint8_t> decompressed;
    for (const auto& pair : compressedData) {
        for (uint32_t i = 0; i < pair.count; ++i) {
            decompressed.push_back(pair.type);
        }
    }
    return decompressed;
}
