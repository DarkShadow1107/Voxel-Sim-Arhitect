#pragma once
#include <vector>
#include <cstdint>

struct RLEPair {
    uint8_t type;
    uint32_t count;
};

class Compression {
public:
    static std::vector<RLEPair> compress(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompress(const std::vector<RLEPair>& compressedData);
};
