#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Texture {
public:
    Texture();
    ~Texture();

    bool generateAtlas();
    void bind(uint32_t slot = 0) const;
    uint32_t getID() const { return m_id; }

    // Atlas tile read/write for Texture Designer
    // Each tile is 16x16 pixels, RGBA (1024 bytes)
    void updateTile(int tileX, int tileY, const uint8_t* rgba16x16);
    void readTile(int tileX, int tileY, uint8_t* rgba16x16) const;

    static constexpr int kAtlasSize = 256;
    static constexpr int kTileSize = 16;
    static constexpr int kTilesPerRow = kAtlasSize / kTileSize; // 16

private:
    uint32_t m_id = 0;
    std::vector<uint8_t> m_atlasData; // Persistent copy of atlas pixels (256*256*4)
};
