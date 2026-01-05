#include "Texture.hpp"
#include <glad/gl.h>
#include <vector>
#include <cmath>
#include <algorithm>

Texture::Texture() {}
Texture::~Texture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

bool Texture::generateAtlas() {
    const int atlasSize = 256;
    const int blockSize = 16;
    std::vector<uint8_t> data(atlasSize * atlasSize * 4, 255);

    auto setPixel = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        if (x < 0 || x >= atlasSize || y < 0 || y >= atlasSize) return;
        int idx = (x + y * atlasSize) * 4;
        data[idx] = r; data[idx+1] = g; data[idx+2] = b; data[idx+3] = a;
    };

    auto drawBlock = [&](int bx, int by, uint8_t r, uint8_t g, uint8_t b, bool noise = true) {
        for (int y = 0; y < blockSize; ++y) {
            for (int x = 0; x < blockSize; ++x) {
                float n = noise ? (float)(rand() % 100) / 100.0f * 0.2f + 0.9f : 1.0f;
                setPixel(bx * blockSize + x, by * blockSize + y, (uint8_t)(r * n), (uint8_t)(g * n), (uint8_t)(b * n));
            }
        }
    };

    auto clearTile = [&](int bx, int by, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        for (int y = 0; y < blockSize; ++y) {
            for (int x = 0; x < blockSize; ++x) {
                setPixel(bx * blockSize + x, by * blockSize + y, r, g, b, a);
            }
        }
    };

    auto drawLine = [&](int bx, int by, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        // Simple Bresenham
        int dx = std::abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            setPixel(bx * blockSize + x0, by * blockSize + y0, r, g, b, a);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };

    // 0,0: Dirt
    drawBlock(0, 0, 120, 80, 50);
    // 1,0: Grass Top
    drawBlock(1, 0, 80, 160, 60);
    // 2,0: Grass Side
    drawBlock(2, 0, 100, 120, 60);
    // 3,0: Stone
    drawBlock(3, 0, 130, 130, 135);
    // 4,0: Water
    drawBlock(4, 0, 40, 100, 220, false);
    // 5,0: Lava - Minecraft-like swirly texture
    for (int y = 0; y < blockSize; ++y) {
        for (int x = 0; x < blockSize; ++x) {
            float d = std::sqrt(std::pow(x - 8, 2) + std::pow(y - 8, 2));
            if (d < 4.0f || d > 10.0f) {
                setPixel(5 * blockSize + x, 0 * blockSize + y, 255, 60 + rand() % 40, 0); // Red-Orange
            } else {
                setPixel(5 * blockSize + x, 0 * blockSize + y, 255, 160 + rand() % 60, 0); // Yellow-Orange
            }
            if (rand() % 10 == 0) setPixel(5 * blockSize + x, 0 * blockSize + y, 180, 40, 0); // Darker spots
        }
    }
    // 6,0: Wood
    drawBlock(6, 0, 100, 70, 40);
    // 7,0: Leaves
    drawBlock(7, 0, 40, 140, 40);
    // 8,0: Sand
    drawBlock(8, 0, 230, 220, 140);
    // 9,0: Snow - Minecraft-like subtle noise
    for (int y = 0; y < blockSize; ++y) {
        for (int x = 0; x < blockSize; ++x) {
            uint8_t r = 250, g = 250, b = 255;
            if ((x + y * 3) % 7 == 0 || (x * 2 + y) % 11 == 0) {
                r = 240; g = 240; b = 250; // Subtle blue-ish shadow
            }
            if (rand() % 20 == 0) {
                r = 255; g = 255; b = 255; // Bright sparkle
            }
            setPixel(9 * blockSize + x, 0 * blockSize + y, r, g, b);
        }
    }
    // 10,0: Bedrock
    drawBlock(10, 0, 40, 40, 40);
    // 11,0: Red Flower
    drawBlock(11, 0, 255, 50, 50);
    // 12,0: Blue Flower
    drawBlock(12, 0, 50, 100, 255);
    // 13,0: Tall Grass
    drawBlock(13, 0, 60, 150, 60);
    // 14,0: Glass (Transparent)
    for(int y=0; y<blockSize; ++y) for(int x=0; x<blockSize; ++x) {
        bool edge = (x==0 || x==15 || y==0 || y==15);
        setPixel(14*blockSize+x, 0*blockSize+y, 200, 230, 255, edge ? 200 : 50);
    }

    // Ores (Row 1)
    drawBlock(0, 1, 130, 130, 135); // Stone base
    for(int i=0; i<10; ++i) setPixel(0*blockSize+rand()%16, 1*blockSize+rand()%16, 20, 20, 20); // Coal
    
    drawBlock(1, 1, 130, 130, 135);
    for(int i=0; i<10; ++i) setPixel(1*blockSize+rand()%16, 1*blockSize+rand()%16, 200, 150, 100); // Iron

    drawBlock(2, 1, 130, 130, 135);
    for(int i=0; i<10; ++i) setPixel(2*blockSize+rand()%16, 1*blockSize+rand()%16, 255, 220, 0); // Gold

    drawBlock(3, 1, 130, 130, 135);
    for(int i=0; i<10; ++i) setPixel(3*blockSize+rand()%16, 1*blockSize+rand()%16, 0, 255, 255); // Diamond

    // Birch Wood (Row 1, Col 4)
    drawBlock(4, 1, 220, 220, 210, false);
    for(int i=0; i<15; ++i) setPixel(4*blockSize+rand()%16, 1*blockSize+rand()%16, 40, 40, 40); // Dark spots
    // Birch Leaves (Row 1, Col 5)
    drawBlock(5, 1, 100, 180, 80);
    // Cherry Wood (Row 1, Col 6) - Pure White/Pinkish Trunk
    drawBlock(6, 1, 255, 250, 250, false);
    for(int i=0; i<16; ++i) setPixel(6*blockSize+rand()%16, 1*blockSize+rand()%16, 255, 255, 255);
    // Cherry Leaves (Row 1, Col 7)
    drawBlock(7, 1, 255, 150, 200); // Keep the pinkish leaves color

    // Ice (Row 1, Col 12)
    for(int y=0; y<blockSize; ++y) for(int x=0; x<blockSize; ++x) {
        setPixel(12*blockSize+x, 1*blockSize+y, 180, 220, 255, 180);
    }

    // Cobblestone (Row 1, Col 8)
    drawBlock(8, 1, 100, 100, 100);
    for(int i=0; i<20; ++i) setPixel(8*blockSize+rand()%16, 1*blockSize+rand()%16, 70, 70, 70);
    // Mossy Stone (Row 1, Col 9)
    drawBlock(9, 1, 100, 100, 100);
    for(int i=0; i<15; ++i) setPixel(9*blockSize+rand()%16, 1*blockSize+rand()%16, 50, 120, 50);
    // Oak Planks (Row 1, Col 10)
    drawBlock(10, 1, 160, 120, 70, false);
    for(int i=0; i<16; ++i) setPixel(10*blockSize+rand()%16, 1*blockSize+rand()%16, 140, 100, 50);
    // Bricks (Row 1, Col 11)
    drawBlock(11, 1, 150, 70, 50, false);
    for(int i=0; i<16; ++i) {
        int x = rand()%16; int y = rand()%16;
        if (y % 4 == 0 || x % 8 == 0) setPixel(11*blockSize+x, 1*blockSize+y, 200, 200, 200);
    }

    // Crack stages (Row 2, columns 0-9)
    // Transparent background with white-ish cracks.
    for (int stage = 0; stage < 10; ++stage) {
        int bx = stage;
        int by = 2;
        clearTile(bx, by, 255, 255, 255, 0);
        int lines = 2 + stage;
        for (int i = 0; i < lines; ++i) {
            int x0 = rand() % 16;
            int y0 = rand() % 16;
            int x1 = rand() % 16;
            int y1 = rand() % 16;
            uint8_t a = (uint8_t)std::clamp(80 + stage * 18, 0, 240);
            drawLine(bx, by, x0, y0, x1, y1, 240, 240, 240, a);
            // a bit of thickness
            if (x0 + 1 < 16) drawLine(bx, by, x0 + 1, y0, x1 + 1, y1, 240, 240, 240, a);
        }
        // subtle border
        for (int x = 0; x < 16; ++x) {
            setPixel(bx * blockSize + x, by * blockSize + 0, 255, 255, 255, (uint8_t)(30 + stage * 8));
            setPixel(bx * blockSize + x, by * blockSize + 15, 255, 255, 255, (uint8_t)(30 + stage * 8));
        }
        for (int y = 0; y < 16; ++y) {
            setPixel(bx * blockSize + 0, by * blockSize + y, 255, 255, 255, (uint8_t)(30 + stage * 8));
            setPixel(bx * blockSize + 15, by * blockSize + y, 255, 255, 255, (uint8_t)(30 + stage * 8));
        }
    }

    // Sun / Moon / Cloud tiles
    // (Row 2: 13=Moon, 14=Sun, 15=Cloud, 12=Star)
    
    // Refined Sun (Minecraft Style: Pure White/Yellow Square)
    clearTile(14, 2, 255, 255, 255, 0);
    for (int y = 1; y < 15; ++y) {
        for (int x = 1; x < 15; ++x) {
            setPixel(14 * blockSize + x, 2 * blockSize + y, 255, 255, 255, 255);
        }
    }
    // Inner yellow
    for (int y = 3; y < 13; ++y) {
        for (int x = 3; x < 13; ++x) {
            setPixel(14 * blockSize + x, 2 * blockSize + y, 255, 255, 200, 255);
        }
    }

    // Refined Moon (Minecraft Style: Square with craters)
    clearTile(13, 2, 255, 255, 255, 0);
    for (int y = 2; y < 14; ++y) {
        for (int x = 2; x < 14; ++x) {
            setPixel(13 * blockSize + x, 2 * blockSize + y, 220, 220, 220, 255);
        }
    }
    // Craters
    setPixel(13 * blockSize + 4, 2 * blockSize + 4, 180, 180, 180, 255);
    setPixel(13 * blockSize + 10, 2 * blockSize + 5, 180, 180, 180, 255);
    setPixel(13 * blockSize + 6, 2 * blockSize + 10, 180, 180, 180, 255);

    // Star (Row 2, Col 12)
    clearTile(12, 2, 255, 255, 255, 0);
    setPixel(12 * blockSize + 7, 2 * blockSize + 7, 255, 255, 255, 255);

    // Refined Cloud (Minecraft Style: Blocky, Volumetric-ish)
    clearTile(15, 2, 255, 255, 255, 0);
    // Main body (White)
    for (int y = 1; y < 13; ++y) {
        for (int x = 1; x < 15; ++x) {
            setPixel(15 * blockSize + x, 2 * blockSize + y, 255, 255, 255, 255);
        }
    }
    // Bottom shadow (Grey)
    for (int x = 1; x < 15; ++x) {
        setPixel(15 * blockSize + x, 2 * blockSize + 13, 200, 200, 210, 255);
        setPixel(15 * blockSize + x, 2 * blockSize + 14, 180, 180, 190, 255);
    }
    // Side shadow
    for (int y = 1; y < 13; ++y) {
        setPixel(15 * blockSize + 14, 2 * blockSize + y, 220, 220, 230, 255);
        setPixel(15 * blockSize + 15, 2 * blockSize + y, 200, 200, 210, 255);
    }

    // Mob tiles (Row 3, columns 0..8)
    // These are simple palettes used for voxel-style mob bodies.
    // Cow
    drawBlock(0, 3, 210, 210, 210, false);
    for (int i = 0; i < 24; ++i) setPixel(0 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 40, 40, 40);
    // Pig
    drawBlock(1, 3, 235, 150, 160, false);
    for (int i = 0; i < 16; ++i) setPixel(1 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 245, 170, 175);
    // Sheep
    drawBlock(2, 3, 240, 240, 240, false);
    for (int i = 0; i < 18; ++i) setPixel(2 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 220, 220, 220);
    // Chicken
    drawBlock(3, 3, 245, 245, 245, false);
    for (int i = 0; i < 10; ++i) setPixel(3 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 255, 210, 70);
    // Dog
    drawBlock(4, 3, 190, 190, 190, false);
    for (int i = 0; i < 14; ++i) setPixel(4 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 140, 140, 140);
    // Cat
    drawBlock(5, 3, 220, 180, 120, false);
    for (int i = 0; i < 18; ++i) setPixel(5 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 120, 90, 60);
    // Fish
    drawBlock(6, 3, 70, 140, 220, false);
    for (int i = 0; i < 20; ++i) setPixel(6 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 140, 200, 255);
    // Salmon
    drawBlock(7, 3, 240, 120, 90, false);
    for (int i = 0; i < 18; ++i) setPixel(7 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 255, 170, 130);
    // Octopus
    drawBlock(8, 3, 160, 80, 180, false);
    for (int i = 0; i < 18; ++i) setPixel(8 * blockSize + rand() % 16, 3 * blockSize + rand() % 16, 110, 40, 130);

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasSize, atlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

void Texture::bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
