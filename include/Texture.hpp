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

private:
    uint32_t m_id = 0;
};
