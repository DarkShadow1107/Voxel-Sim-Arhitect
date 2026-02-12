#include "Texture.hpp"
#include <glad/gl.h>
#include <vector>
#include <cmath>
#include <cstring>
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

    // Deterministic hash noise (no rand() - consistent textures every launch)
    auto pixelHash = [](int x, int y, int seed) -> float {
        int h = x * 374761393 + y * 668265263 + seed * 1274126177;
        h = (h ^ (h >> 13)) * 1103515245;
        return (float)(h & 0x7FFFFFFF) / (float)0x7FFFFFFF;
    };

    auto drawBlock = [&](int bx, int by, uint8_t r, uint8_t g, uint8_t b, int noiseAmt = 15) {
        for (int y = 0; y < blockSize; ++y) {
            for (int x = 0; x < blockSize; ++x) {
                float n = pixelHash(x, y, bx * 16 + by);
                int nr = (int)r + (int)((n - 0.5f) * noiseAmt);
                int ng = (int)g + (int)((n - 0.5f) * noiseAmt);
                int nb = (int)b + (int)((n - 0.5f) * noiseAmt);
                setPixel(bx * blockSize + x, by * blockSize + y,
                    (uint8_t)std::clamp(nr, 0, 255),
                    (uint8_t)std::clamp(ng, 0, 255),
                    (uint8_t)std::clamp(nb, 0, 255));
            }
        }
    };

    auto clearTile = [&](int bx, int by, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        for (int y = 0; y < blockSize; ++y)
            for (int x = 0; x < blockSize; ++x)
                setPixel(bx * blockSize + x, by * blockSize + y, r, g, b, a);
    };

    auto drawLine = [&](int bx, int by, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            setPixel(bx * blockSize + x0, by * blockSize + y0, r, g, b, a);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };

    // ============ Row 0: Base blocks ============

    // 0,0: Dirt
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 100);
            float streak = pixelHash(x / 4, y, 101) * 0.12f;
            setPixel(0 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp((int)(120 + (n - 0.5f) * 20 + streak * 25), 0, 255),
                (uint8_t)std::clamp((int)(80  + (n - 0.5f) * 15 + streak * 12), 0, 255),
                (uint8_t)std::clamp((int)(50  + (n - 0.5f) * 10), 0, 255));
        }

    // 1,0: Grass Top
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 200);
            float p = pixelHash(x / 3, y / 3, 201);
            setPixel(1 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp((int)(75  + (n - 0.5f) * 12 + p * 8), 0, 255),
                (uint8_t)std::clamp((int)(155 + (n - 0.5f) * 18 + p * 12), 0, 255),
                (uint8_t)std::clamp((int)(55  + (n - 0.5f) * 8), 0, 255));
        }

    // 2,0: Grass Side - dirt body with green top
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 300);
            int grassEdge = (int)(pixelHash(x, 0, 301) * 2.0f) + 2;
            if (y < grassEdge) {
                setPixel(2 * blockSize + x, 0 * blockSize + y,
                    (uint8_t)std::clamp((int)(75  + (n - 0.5f) * 12), 0, 255),
                    (uint8_t)std::clamp((int)(150 + (n - 0.5f) * 18), 0, 255),
                    (uint8_t)std::clamp((int)(50  + (n - 0.5f) * 8), 0, 255));
            } else {
                setPixel(2 * blockSize + x, 0 * blockSize + y,
                    (uint8_t)std::clamp((int)(120 + (n - 0.5f) * 16), 0, 255),
                    (uint8_t)std::clamp((int)(80  + (n - 0.5f) * 12), 0, 255),
                    (uint8_t)std::clamp((int)(50  + (n - 0.5f) * 8), 0, 255));
            }
        }

    // 3,0: Stone
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 400);
            float crack = pixelHash(x / 2, y / 3, 401);
            int base = (int)(128 + (n - 0.5f) * 14);
            if (crack > 0.85f) base -= 18;
            if (crack < 0.1f) base += 6;
            setPixel(3 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp(base, 60, 170),
                (uint8_t)std::clamp(base, 60, 170),
                (uint8_t)std::clamp(base + 3, 60, 175));
        }

    // 4,0: Water
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 500);
            setPixel(4 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp((int)(30 + (n - 0.5f) * 8), 0, 255),
                (uint8_t)std::clamp((int)(80 + (n - 0.5f) * 12), 0, 255),
                (uint8_t)std::clamp((int)(200 + (n - 0.5f) * 15), 0, 255));
        }

    // 5,0: Lava
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 600);
            float flow = pixelHash(x / 3, y / 2, 601);
            if (flow > 0.6f)
                setPixel(5 * blockSize + x, 0 * blockSize + y, 255, (uint8_t)(160 + n * 60), 0);
            else if (flow < 0.2f)
                setPixel(5 * blockSize + x, 0 * blockSize + y, (uint8_t)(180 + n * 30), (uint8_t)(40 + n * 20), 0);
            else
                setPixel(5 * blockSize + x, 0 * blockSize + y, 255, (uint8_t)(80 + n * 40), 0);
        }

    // 6,0: Wood - with vertical grain
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 700);
            bool grain = (x == 3 || x == 7 || x == 11);
            int r = grain ? (int)(80 + n * 12) : (int)(105 + (n - 0.5f) * 12);
            int g = grain ? (int)(55 + n * 8)  : (int)(72  + (n - 0.5f) * 10);
            int b = grain ? (int)(30 + n * 6)  : (int)(42  + (n - 0.5f) * 6);
            setPixel(6 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp(r, 0, 255), (uint8_t)std::clamp(g, 0, 255), (uint8_t)std::clamp(b, 0, 255));
        }

    // 7,0: Leaves
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 800);
            float cl = pixelHash(x / 2, y / 2, 801);
            int r = (int)(35 + cl * 18 + (n - 0.5f) * 8);
            int g = (int)(120 + cl * 28 + (n - 0.5f) * 16);
            int b = (int)(35 + cl * 8 + (n - 0.5f) * 6);
            if (pixelHash(x, y, 802) > 0.92f) { r -= 15; g -= 20; b -= 10; }
            setPixel(7 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp(r, 0, 255), (uint8_t)std::clamp(g, 0, 255), (uint8_t)std::clamp(b, 0, 255));
        }

    // 8,0: Sand
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 900);
            setPixel(8 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp((int)(225 + (n - 0.5f) * 12), 0, 255),
                (uint8_t)std::clamp((int)(215 + (n - 0.5f) * 12), 0, 255),
                (uint8_t)std::clamp((int)(135 + (n - 0.5f) * 16), 0, 255));
        }

    // 9,0: Snow - pure white with very subtle variation
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 1000);
            setPixel(9 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp((int)(255 + (n - 0.5f) * 2), 253, 255),
                (uint8_t)std::clamp((int)(255 + (n - 0.5f) * 2), 253, 255),
                (uint8_t)255);
        }

    // 10,0: Bedrock
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 1100);
            float r = pixelHash(x / 2, y / 2, 1101);
            int base = (int)(40 + (n - 0.5f) * 12 + r * 8);
            setPixel(10 * blockSize + x, 0 * blockSize + y,
                (uint8_t)std::clamp(base, 25, 60),(uint8_t)std::clamp(base, 25, 60),(uint8_t)std::clamp(base, 25, 60));
        }

    // 11,0: Red Flower
    clearTile(11, 0, 0, 0, 0, 0);
    for (int y = 8; y < 16; ++y) { setPixel(11*blockSize+7,y,50,130,40); setPixel(11*blockSize+8,y,50,130,40); }
    for (int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx)
        if(dx*dx+dy*dy<=5) setPixel(11*blockSize+7+dx,5+dy,220,40,40);
    setPixel(11*blockSize+7,5,255,220,50);

    // 12,0: Blue Flower
    clearTile(12, 0, 0, 0, 0, 0);
    for (int y = 8; y < 16; ++y) { setPixel(12*blockSize+7,y,50,130,40); setPixel(12*blockSize+8,y,50,130,40); }
    for (int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx)
        if(dx*dx+dy*dy<=5) setPixel(12*blockSize+7+dx,5+dy,60,90,230);
    setPixel(12*blockSize+7,5,255,220,50);

    // 13,0: Tall Grass
    clearTile(13, 0, 0, 0, 0, 0);
    for (int blade = 0; blade < 5; ++blade) {
        int bx = 2 + blade * 3;
        for (int y = 3; y < 16; ++y) {
            float sway = std::sin(blade * 1.5f + y * 0.3f) * 0.8f;
            int px = bx + (int)sway;
            if (px >= 0 && px < 16) {
                int g = std::min(140 + (16 - y) * 3, 185);
                setPixel(13 * blockSize + px, y, 55, g, 45);
            }
        }
    }

    // 14,0: Glass
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            bool edge = (x==0||x==15||y==0||y==15);
            bool inner = (x==1||x==14||y==1||y==14);
            if (edge) setPixel(14*blockSize+x,y,180,210,240,200);
            else if (inner) setPixel(14*blockSize+x,y,200,225,245,100);
            else setPixel(14*blockSize+x,y,220,235,250,40);
        }

    // ============ Row 1: Ores & special blocks ============

    // Helper: ore on stone base
    auto drawOre = [&](int col, int seed, uint8_t or_, uint8_t og, uint8_t ob) {
        for (int y = 0; y < blockSize; ++y)
            for (int x = 0; x < blockSize; ++x) {
                float n = pixelHash(x, y, seed);
                float ore = pixelHash(x / 2, y / 2, seed + 1);
                if (ore > 0.7f) {
                    setPixel(col * blockSize + x, 1 * blockSize + y, or_, og, ob);
                } else {
                    int base = (int)(128 + (n - 0.5f) * 14);
                    setPixel(col * blockSize + x, 1 * blockSize + y, (uint8_t)base, (uint8_t)base, (uint8_t)(base+3));
                }
            }
    };

    drawOre(0, 1200, 30, 30, 30);       // Coal
    drawOre(1, 1300, 200, 155, 110);     // Iron
    drawOre(2, 1400, 255, 215, 0);       // Gold
    drawOre(3, 1500, 80, 230, 230);      // Diamond

    // 4,1: Birch Wood
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 1600);
            bool dark = (y % 5 == 0 && pixelHash(x, y, 1601) > 0.5f);
            if (dark) setPixel(4*blockSize+x,1*blockSize+y,50,50,45);
            else setPixel(4*blockSize+x,1*blockSize+y,
                (uint8_t)(220+(n-0.5f)*8),(uint8_t)(218+(n-0.5f)*8),(uint8_t)(210+(n-0.5f)*8));
        }

    // 5,1: Birch Leaves
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 1700);
            float cl = pixelHash(x/2,y/2,1701);
            setPixel(5*blockSize+x,1*blockSize+y,
                (uint8_t)std::clamp((int)(90+cl*12+(n-0.5f)*8),0,255),
                (uint8_t)std::clamp((int)(170+cl*12+(n-0.5f)*12),0,255),
                (uint8_t)std::clamp((int)(70+cl*8+(n-0.5f)*6),0,255));
        }

    // 6,1: Cherry Wood
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 1800);
            bool grain = (x==4||x==10);
            setPixel(6*blockSize+x,1*blockSize+y,
                grain?240:(uint8_t)(250+(n-0.5f)*6),
                grain?235:(uint8_t)(245+(n-0.5f)*6),
                grain?240:(uint8_t)(248+(n-0.5f)*4));
        }

    // 7,1: Cherry Leaves
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 1900);
            float cl = pixelHash(x/2,y/2,1901);
            int r = (int)(240+cl*8+(n-0.5f)*8);
            int g = (int)(140+cl*12+(n-0.5f)*10);
            int b = (int)(190+cl*8+(n-0.5f)*8);
            if (pixelHash(x,y,1902)>0.9f) { r=255; g+=15; b+=8; }
            setPixel(7*blockSize+x,1*blockSize+y,
                (uint8_t)std::clamp(r,0,255),(uint8_t)std::clamp(g,0,255),(uint8_t)std::clamp(b,0,255));
        }

    // 8,1: Cobblestone
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 2000);
            float stone = pixelHash(x/3,y/3,2001);
            bool seam = (x%5==0||y%4==0) && pixelHash(x,y,2002)>0.6f;
            int base = (int)(100+stone*28+(n-0.5f)*12);
            if (seam) base -= 22;
            uint8_t v = (uint8_t)std::clamp(base,50,150);
            setPixel(8*blockSize+x,1*blockSize+y,v,v,v);
        }

    // 9,1: Mossy Stone
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 2100);
            float moss = pixelHash(x/2,y/2,2101);
            if (moss > 0.55f) {
                setPixel(9*blockSize+x,1*blockSize+y,(uint8_t)(50+n*12),(uint8_t)(110+n*18),(uint8_t)(45+n*8));
            } else {
                int base = (int)(105+(n-0.5f)*14);
                setPixel(9*blockSize+x,1*blockSize+y,(uint8_t)base,(uint8_t)base,(uint8_t)base);
            }
        }

    // 10,1: Oak Planks
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 2200);
            bool line = (y%4==0);
            if (line) setPixel(10*blockSize+x,1*blockSize+y,(uint8_t)(120+n*8),(uint8_t)(85+n*6),(uint8_t)(45+n*4));
            else setPixel(10*blockSize+x,1*blockSize+y,
                (uint8_t)std::clamp((int)(160+(n-0.5f)*10),0,255),
                (uint8_t)std::clamp((int)(120+(n-0.5f)*8),0,255),
                (uint8_t)std::clamp((int)(70+(n-0.5f)*6),0,255));
        }

    // 11,1: Bricks
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 2300);
            int row = y/4;
            int offset = (row%2)*4;
            bool grout = (y%4==0) || ((x+offset)%8==0);
            if (grout) setPixel(11*blockSize+x,1*blockSize+y,190,185,175);
            else setPixel(11*blockSize+x,1*blockSize+y,
                (uint8_t)std::clamp((int)(170+(n-0.5f)*16),0,255),
                (uint8_t)std::clamp((int)(80+(n-0.5f)*12),0,255),
                (uint8_t)std::clamp((int)(55+(n-0.5f)*8),0,255));
        }

    // 12,1: Ice
    for (int y = 0; y < blockSize; ++y)
        for (int x = 0; x < blockSize; ++x) {
            float n = pixelHash(x, y, 2400);
            float crack = pixelHash(x/3,y/4,2401);
            int r = (int)(175+(n-0.5f)*8), g = (int)(215+(n-0.5f)*8), b = (int)(250+(n-0.5f)*4);
            if (crack > 0.85f) { r-=12; g-=8; b-=4; }
            setPixel(12*blockSize+x,1*blockSize+y,
                (uint8_t)std::clamp(r,0,255),(uint8_t)std::clamp(g,0,255),(uint8_t)std::clamp(b,0,255),180);
        }

    // ============ Row 2: Crack stages + sky ============

    for (int stage = 0; stage < 10; ++stage) {
        clearTile(stage, 2, 255, 255, 255, 0);
        int lines = 2 + stage;
        for (int i = 0; i < lines; ++i) {
            int x0 = (int)(pixelHash(i, stage, 3000)*14)+1;
            int y0 = (int)(pixelHash(i, stage, 3001)*14)+1;
            int x1 = (int)(pixelHash(i, stage, 3002)*14)+1;
            int y1 = (int)(pixelHash(i, stage, 3003)*14)+1;
            uint8_t a = (uint8_t)std::clamp(80+stage*18,0,240);
            drawLine(stage, 2, x0, y0, x1, y1, 240, 240, 240, a);
            if (x0+1<16) drawLine(stage, 2, x0+1, y0, x1+1, y1, 240, 240, 240, a);
        }
        for (int x=0;x<16;++x) {
            setPixel(stage*blockSize+x,2*blockSize+0,255,255,255,(uint8_t)(30+stage*8));
            setPixel(stage*blockSize+x,2*blockSize+15,255,255,255,(uint8_t)(30+stage*8));
        }
        for (int y=0;y<16;++y) {
            setPixel(stage*blockSize+0,2*blockSize+y,255,255,255,(uint8_t)(30+stage*8));
            setPixel(stage*blockSize+15,2*blockSize+y,255,255,255,(uint8_t)(30+stage*8));
        }
    }

    // Star (12,2)
    clearTile(12, 2, 255, 255, 255, 0);
    setPixel(12*blockSize+7,2*blockSize+7,255,255,255,255);

    // Moon (13,2)
    clearTile(13, 2, 255, 255, 255, 0);
    for (int y=2;y<14;++y) for(int x=2;x<14;++x)
        setPixel(13*blockSize+x,2*blockSize+y,220,220,225,255);
    setPixel(13*blockSize+4,2*blockSize+4,190,190,195,255);
    setPixel(13*blockSize+10,2*blockSize+5,190,190,195,255);
    setPixel(13*blockSize+6,2*blockSize+10,190,190,195,255);

    // Sun (14,2)
    clearTile(14, 2, 255, 255, 255, 0);
    for (int y=1;y<15;++y) for(int x=1;x<15;++x)
        setPixel(14*blockSize+x,2*blockSize+y,255,255,255,255);
    for (int y=3;y<13;++y) for(int x=3;x<13;++x)
        setPixel(14*blockSize+x,2*blockSize+y,255,255,200,255);

    // Cloud (15,2)
    clearTile(15, 2, 255, 255, 255, 0);
    for (int y=1;y<13;++y) for(int x=1;x<15;++x)
        setPixel(15*blockSize+x,2*blockSize+y,255,255,255,255);
    for (int x=1;x<15;++x) {
        setPixel(15*blockSize+x,2*blockSize+13,200,200,210,255);
        setPixel(15*blockSize+x,2*blockSize+14,180,180,190,255);
    }
    for (int y=1;y<13;++y) {
        setPixel(15*blockSize+14,2*blockSize+y,220,220,230,255);
        setPixel(15*blockSize+15,2*blockSize+y,200,200,210,255);
    }

    // ============ Row 3: Mob textures ============
    // Cow (patches)
    drawBlock(0, 3, 210, 210, 210, 0);
    for (int y=0;y<16;++y) for(int x=0;x<16;++x)
        if (pixelHash(x/3,y/3,4000)>0.6f) setPixel(x,3*blockSize+y,40,40,40);
    drawBlock(1, 3, 235, 150, 160, 5); // Pig
    drawBlock(2, 3, 240, 240, 240, 5); // Sheep
    drawBlock(3, 3, 245, 245, 245, 5); // Chicken
    drawBlock(4, 3, 180, 150, 120, 8); // Dog
    drawBlock(5, 3, 220, 180, 120, 8); // Cat
    drawBlock(6, 3, 70, 140, 220, 8);  // Fish
    drawBlock(7, 3, 240, 120, 90, 8);  // Salmon
    drawBlock(8, 3, 160, 80, 180, 8);  // Octopus

    // ============ Upload to GPU ============
    // Store persistent copy for Texture Designer read/write
    m_atlasData = data;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasSize, atlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.5f);

    return true;
}

void Texture::bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::updateTile(int tileX, int tileY, const uint8_t* rgba16x16) {
    if (!m_id || m_atlasData.empty()) return;
    if (tileX < 0 || tileX >= kTilesPerRow || tileY < 0 || tileY >= kTilesPerRow) return;

    // Update CPU-side atlas data
    for (int y = 0; y < kTileSize; ++y) {
        int atlasY = tileY * kTileSize + y;
        int atlasX = tileX * kTileSize;
        int dstIdx = (atlasX + atlasY * kAtlasSize) * 4;
        int srcIdx = y * kTileSize * 4;
        std::memcpy(&m_atlasData[dstIdx], &rgba16x16[srcIdx], kTileSize * 4);
    }

    // Upload to GPU via glTexSubImage2D
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
        tileX * kTileSize, tileY * kTileSize,
        kTileSize, kTileSize,
        GL_RGBA, GL_UNSIGNED_BYTE, rgba16x16);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture::readTile(int tileX, int tileY, uint8_t* rgba16x16) const {
    if (m_atlasData.empty()) {
        std::memset(rgba16x16, 0, kTileSize * kTileSize * 4);
        return;
    }
    if (tileX < 0 || tileX >= kTilesPerRow || tileY < 0 || tileY >= kTilesPerRow) {
        std::memset(rgba16x16, 0, kTileSize * kTileSize * 4);
        return;
    }

    for (int y = 0; y < kTileSize; ++y) {
        int atlasY = tileY * kTileSize + y;
        int atlasX = tileX * kTileSize;
        int srcIdx = (atlasX + atlasY * kAtlasSize) * 4;
        int dstIdx = y * kTileSize * 4;
        std::memcpy(&rgba16x16[dstIdx], &m_atlasData[srcIdx], kTileSize * 4);
    }
}
