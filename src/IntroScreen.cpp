#include "IntroScreen.hpp"
#include "imgui.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// 5x7 bitmap font - each character is 7 rows of 5-bit-wide data (MSB = leftmost pixel)
const uint8_t IntroScreen::kFont[128][7] = {
    /* 0  */ {0,0,0,0,0,0,0},
    /* 1  */ {0,0,0,0,0,0,0},
    /* 2  */ {0,0,0,0,0,0,0},
    /* 3  */ {0,0,0,0,0,0,0},
    /* 4  */ {0,0,0,0,0,0,0},
    /* 5  */ {0,0,0,0,0,0,0},
    /* 6  */ {0,0,0,0,0,0,0},
    /* 7  */ {0,0,0,0,0,0,0},
    /* 8  */ {0,0,0,0,0,0,0},
    /* 9  */ {0,0,0,0,0,0,0},
    /* 10 */ {0,0,0,0,0,0,0},
    /* 11 */ {0,0,0,0,0,0,0},
    /* 12 */ {0,0,0,0,0,0,0},
    /* 13 */ {0,0,0,0,0,0,0},
    /* 14 */ {0,0,0,0,0,0,0},
    /* 15 */ {0,0,0,0,0,0,0},
    /* 16 */ {0,0,0,0,0,0,0},
    /* 17 */ {0,0,0,0,0,0,0},
    /* 18 */ {0,0,0,0,0,0,0},
    /* 19 */ {0,0,0,0,0,0,0},
    /* 20 */ {0,0,0,0,0,0,0},
    /* 21 */ {0,0,0,0,0,0,0},
    /* 22 */ {0,0,0,0,0,0,0},
    /* 23 */ {0,0,0,0,0,0,0},
    /* 24 */ {0,0,0,0,0,0,0},
    /* 25 */ {0,0,0,0,0,0,0},
    /* 26 */ {0,0,0,0,0,0,0},
    /* 27 */ {0,0,0,0,0,0,0},
    /* 28 */ {0,0,0,0,0,0,0},
    /* 29 */ {0,0,0,0,0,0,0},
    /* 30 */ {0,0,0,0,0,0,0},
    /* 31 */ {0,0,0,0,0,0,0},
    /* 32 ' ' */ {0,0,0,0,0,0,0},
    /* 33 */ {0,0,0,0,0,0,0},
    /* 34 */ {0,0,0,0,0,0,0},
    /* 35 */ {0,0,0,0,0,0,0},
    /* 36 */ {0,0,0,0,0,0,0},
    /* 37 */ {0,0,0,0,0,0,0},
    /* 38 */ {0,0,0,0,0,0,0},
    /* 39 */ {0,0,0,0,0,0,0},
    /* 40 */ {0,0,0,0,0,0,0},
    /* 41 */ {0,0,0,0,0,0,0},
    /* 42 */ {0,0,0,0,0,0,0},
    /* 43 */ {0,0,0,0,0,0,0},
    /* 44 */ {0,0,0,0,0,0,0},
    /* 45 '-' */ {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},
    /* 46 '.' */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04},
    /* 47 */ {0,0,0,0,0,0,0},
    /* 48 '0' */ {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    /* 49 '1' */ {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    /* 50 '2' */ {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F},
    /* 51 '3' */ {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},
    /* 52 '4' */ {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    /* 53 '5' */ {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    /* 54 '6' */ {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    /* 55 '7' */ {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    /* 56 '8' */ {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    /* 57 '9' */ {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E},
    /* 58 */ {0,0,0,0,0,0,0},
    /* 59 */ {0,0,0,0,0,0,0},
    /* 60 */ {0,0,0,0,0,0,0},
    /* 61 */ {0,0,0,0,0,0,0},
    /* 62 */ {0,0,0,0,0,0,0},
    /* 63 */ {0,0,0,0,0,0,0},
    /* 64 */ {0,0,0,0,0,0,0},
    /* 65 'A' */ {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    /* 66 'B' */ {0x1E, 0x11, 0x1E, 0x11, 0x11, 0x11, 0x1E},
    /* 67 'C' */ {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    /* 68 'D' */ {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},
    /* 69 'E' */ {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    /* 70 'F' */ {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    /* 71 'G' */ {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E},
    /* 72 'H' */ {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    /* 73 'I' */ {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    /* 74 'J' */ {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    /* 75 'K' */ {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    /* 76 'L' */ {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    /* 77 'M' */ {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    /* 78 'N' */ {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    /* 79 'O' */ {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    /* 80 'P' */ {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    /* 81 'Q' */ {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    /* 82 'R' */ {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    /* 83 'S' */ {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E},
    /* 84 'T' */ {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    /* 85 'U' */ {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    /* 86 'V' */ {0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04},
    /* 87 'W' */ {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11},
    /* 88 'X' */ {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    /* 89 'Y' */ {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    /* 90 'Z' */ {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
    /* 91 */ {0,0,0,0,0,0,0}, /* 92 */ {0,0,0,0,0,0,0},
    /* 93 */ {0,0,0,0,0,0,0}, /* 94 */ {0,0,0,0,0,0,0},
    /* 95 */ {0,0,0,0,0,0,0}, /* 96 */ {0,0,0,0,0,0,0},
    /* 97 */ {0,0,0,0,0,0,0}, /* 98 */ {0,0,0,0,0,0,0},
    /* 99 */ {0,0,0,0,0,0,0}, /*100 */ {0,0,0,0,0,0,0},
    /*101 */ {0,0,0,0,0,0,0}, /*102 */ {0,0,0,0,0,0,0},
    /*103 */ {0,0,0,0,0,0,0}, /*104 */ {0,0,0,0,0,0,0},
    /*105 */ {0,0,0,0,0,0,0}, /*106 */ {0,0,0,0,0,0,0},
    /*107 */ {0,0,0,0,0,0,0}, /*108 */ {0,0,0,0,0,0,0},
    /*109 */ {0,0,0,0,0,0,0}, /*110 */ {0,0,0,0,0,0,0},
    /*111 */ {0,0,0,0,0,0,0}, /*112 */ {0,0,0,0,0,0,0},
    /*113 */ {0,0,0,0,0,0,0}, /*114 */ {0,0,0,0,0,0,0},
    /*115 */ {0,0,0,0,0,0,0}, /*116 */ {0,0,0,0,0,0,0},
    /*117 */ {0,0,0,0,0,0,0}, /*118 */ {0,0,0,0,0,0,0},
    /*119 */ {0,0,0,0,0,0,0}, /*120 */ {0,0,0,0,0,0,0},
    /*121 */ {0,0,0,0,0,0,0}, /*122 */ {0,0,0,0,0,0,0},
    /*123 */ {0,0,0,0,0,0,0}, /*124 */ {0,0,0,0,0,0,0},
    /*125 */ {0,0,0,0,0,0,0}, /*126 */ {0,0,0,0,0,0,0},
    /*127 */ {0,0,0,0,0,0,0},
};

// ============================================================

void IntroScreen::initStars() {
    m_stars.clear();
    m_stars.resize(400); // doubled from 200 for denser starfield
    for (auto& s : m_stars) {
        s.x = (float)(rand() % 10000) * 0.0001f; // normalized 0-1
        s.y = (float)(rand() % 10000) * 0.0001f;
        s.brightness = 0.3f + (rand() % 70) * 0.01f;
        s.twinkleSpeed = 1.0f + (rand() % 40) * 0.1f;
        s.twinkleOffset = (rand() % 1000) * 0.001f * 6.28f;
        s.size = (rand() % 100 < 15) ? 2.0f : 1.0f; // 15% chance of bigger star
    }
}

void IntroScreen::initNebulaClouds() {
    m_nebulaClouds.clear();

    // Nebula color palette: purples, blues, teals
    struct { uint8_t r, g, b; } nebulaColors[] = {
        {80, 40, 130},   // deep purple
        {45, 55, 140},   // blue
        {35, 100, 120},  // teal
        {100, 30, 110},  // magenta-purple
        {50, 70, 150},   // slate blue
        {30, 110, 105},  // dark teal
        {70, 50, 140},   // violet
        {55, 80, 130},   // steel blue
    };

    for (int i = 0; i < 14; i++) {
        NebulaCloud nc;
        nc.x = (rand() % 10000) * 0.0001f;
        nc.y = (rand() % 10000) * 0.0001f;
        nc.radius = 0.10f + (rand() % 150) * 0.001f; // 0.10 to 0.25 of screen
        auto& c = nebulaColors[i % 8];
        nc.color = IM_COL32(c.r, c.g, c.b, 255);
        nc.baseAlpha = 12.0f + (rand() % 22); // 12-33 alpha (very subtle)
        nc.driftSpeed = 0.08f + (rand() % 25) * 0.01f;
        nc.driftPhase = (rand() % 1000) * 0.001f * 6.28f;
        m_nebulaClouds.push_back(nc);
    }
}

void IntroScreen::initLogo() {
    m_logoVoxels.clear();

    // 3D isometric voxel cube - BIGGER scale for more visual impact
    float isoScale = 10.0f; // increased from 6.0f
    float isoAngle = 30.0f * 3.14159f / 180.0f;
    float cosA = cosf(isoAngle);
    float sinA = sinf(isoAngle);

    // Build a recognizable voxel structure: cube frame + inner cross
    auto addVoxel = [&](int gx, int gy, int gz, uint32_t color) {
        LogoVoxel v;
        // Isometric projection
        float px = (gx - gz) * cosA * isoScale;
        float py = -gy * isoScale + (gx + gz) * sinA * isoScale;
        v.targetX = px;
        v.targetY = py;
        // Random start position (scattered)
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float dist = 300.0f + (rand() % 400);
        v.startX = cosf(angle) * dist;
        v.startY = sinf(angle) * dist;
        v.currentX = v.startX;
        v.currentY = v.startY;
        v.color = color;
        v.alpha = 0.0f;
        // Stagger by distance from center
        float d = sqrtf((float)(gx * gx + gy * gy + gz * gz));
        v.delay = d * 0.06f;
        m_logoVoxels.push_back(v);
    };

    // Colors for different cube layers
    uint32_t grassGreen = IM_COL32(76, 175, 80, 255);
    uint32_t dirtBrown = IM_COL32(139, 105, 55, 255);
    uint32_t stonePurp = IM_COL32(120, 110, 140, 255);
    uint32_t skyBlue = IM_COL32(100, 180, 240, 255);

    // Build a 5x5x5 voxel "terrain chunk" shape
    int size = 5;
    for (int gx = 0; gx < size; gx++) {
        for (int gz = 0; gz < size; gz++) {
            // Top layer (grass)
            addVoxel(gx, size - 1, gz, grassGreen);
            // Middle layers (dirt)
            for (int gy = 1; gy < size - 1; gy++) {
                // Only visible edges: front face, right face, or top
                bool frontFace = (gz == 0);
                bool rightFace = (gx == size - 1);
                if (frontFace || rightFace) {
                    addVoxel(gx, gy, gz, dirtBrown);
                }
            }
            // Bottom layer (stone, only visible edges)
            if (gx == size - 1 || gz == 0) {
                addVoxel(gx, 0, gz, stonePurp);
            }
        }
    }
    // A small "water" section on top
    addVoxel(0, size, 0, skyBlue);
    addVoxel(1, size, 0, skyBlue);
    addVoxel(0, size, 1, skyBlue);
}

void IntroScreen::spawnAmbientParticle(float screenW, float screenH) {
    AmbientParticle p;
    p.x = (float)(rand() % (int)screenW);
    p.y = screenH + 10.0f;
    p.vx = ((rand() % 100) - 50) * 0.3f;
    p.vy = -(30.0f + (rand() % 60));
    p.alpha = 0.0f;
    p.size = 1.0f + (rand() % 30) * 0.1f;
    p.maxLife = 3.0f + (rand() % 30) * 0.1f;
    p.life = 0.0f;
    // Warm subtle colors
    int which = rand() % 4;
    if (which == 0)      p.color = IM_COL32(100, 180, 255, 255);  // blue
    else if (which == 1) p.color = IM_COL32(80, 200, 120, 255);   // green
    else if (which == 2) p.color = IM_COL32(200, 160, 80, 255);   // amber
    else                 p.color = IM_COL32(180, 130, 220, 255);   // purple
    m_particles.push_back(p);
}

void IntroScreen::spawnShootingStar(float screenW, float screenH) {
    ShootingStar ss;
    // Start from top or left edge, streak diagonally
    if (rand() % 2 == 0) {
        // From top area
        ss.x = (float)(rand() % (int)(screenW * 0.8f));
        ss.y = -5.0f;
    } else {
        // From left side
        ss.x = -5.0f;
        ss.y = (float)(rand() % (int)(screenH * 0.5f));
    }
    float angle = (20.0f + (rand() % 50)) * 3.14159f / 180.0f; // 20-70 degrees downward-right
    float speed = 400.0f + (rand() % 500);
    ss.vx = cosf(angle) * speed;
    ss.vy = sinf(angle) * speed;
    ss.life = 0.0f;
    ss.maxLife = 0.4f + (rand() % 80) * 0.01f; // 0.4 to 1.2 seconds
    ss.brightness = 0.7f + (rand() % 30) * 0.01f;
    m_shootingStars.push_back(ss);
}

void IntroScreen::init() {
    m_cubes.clear();
    m_elapsed = 0.0f;
    m_totalTime = 0.0f;
    m_phase = FADE_IN;
    m_skipped = false;
    m_dissolveSparksSpawned = false;
    m_shootingStarTimer = 0.0f;

    m_shootingStars.clear();
    m_trailParticles.clear();
    m_dissolveSparks.clear();
    m_particles.clear();

    srand(42);
    initStars();
    initNebulaClouds();
    initLogo();

    // Build text cubes (same bitmap font approach but with improvements)
    const char* line1 = "VOXEL-SIM";
    const char* line2 = "ARCHITECT";
    const float cubeSize = 8.0f;
    const float charSpacing = 1.0f;

    // Richer palette
    uint32_t palette[] = {
        IM_COL32(76, 200, 100, 255),   // bright green
        IM_COL32(60, 180, 90, 255),    // green
        IM_COL32(160, 120, 50, 255),   // brown
        IM_COL32(120, 100, 60, 255),   // dark brown
        IM_COL32(100, 170, 240, 255),  // sky blue
        IM_COL32(140, 140, 160, 255),  // stone grey
        IM_COL32(80, 200, 140, 255),   // sea green
        IM_COL32(200, 180, 100, 255),  // sand
    };
    int paletteSize = 8;
    int colorIdx = 0;

    auto addLine = [&](const char* text, float baseY) {
        int len = (int)strlen(text);
        float totalWidth = len * (5 + charSpacing) - charSpacing;
        float startX = -totalWidth * 0.5f;

        for (int ci = 0; ci < len; ci++) {
            char ch = text[ci];
            int idx = (int)(unsigned char)ch;
            if (idx < 0 || idx > 127) continue;

            float charX = startX + ci * (5 + charSpacing);
            for (int row = 0; row < 7; row++) {
                uint8_t rowBits = kFont[idx][row];
                for (int col = 0; col < 5; col++) {
                    if (rowBits & (0x10 >> col)) {
                        VoxelCube cube;
                        cube.targetX = (charX + col) * cubeSize;
                        cube.targetY = (baseY + row) * cubeSize;
                        // Spiral start positions
                        float angle = (colorIdx * 137.5f) * 3.14159f / 180.0f; // golden angle
                        float dist = 400.0f + (rand() % 600);
                        cube.startX = cosf(angle) * dist;
                        cube.startY = sinf(angle) * dist;
                        cube.currentX = cube.startX;
                        cube.currentY = cube.startY;
                        cube.color = palette[colorIdx % paletteSize];
                        cube.rotation = (float)(rand() % 360);
                        cube.scale = 0.0f;
                        colorIdx++;
                        float dx = cube.targetX;
                        float dy = cube.targetY;
                        cube.delay = sqrtf(dx * dx + dy * dy) * 0.0008f;
                        cube.alpha = 0.0f;
                        m_cubes.push_back(cube);
                    }
                }
            }
        }
    };

    addLine(line1, -4.5f);
    addLine(line2, 4.5f);
}

void IntroScreen::skip() {
    m_skipped = true;
    m_phase = DONE;
}

bool IntroScreen::update(float dt) {
    if (m_skipped) return true;

    m_elapsed += dt;
    m_totalTime += dt;

    // Check for skip input (any key or mouse click)
    ImGuiIO& io = ImGui::GetIO();
    bool inputPressed = false;
    if (m_totalTime > 0.3f) { // small grace period
        for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
            if (ImGui::IsKeyPressed((ImGuiKey)key, false)) { inputPressed = true; break; }
        }
        if (io.MouseClicked[0] || io.MouseClicked[1]) { inputPressed = true; }
    }

    if (inputPressed) {
        if (m_phase == FADE_IN || m_phase == LOGO_ASSEMBLE || m_phase == TITLE_IN) {
            // Skip to holding
            m_phase = HOLDING;
            m_elapsed = 0.0f;
            for (auto& c : m_cubes) {
                c.currentX = c.targetX;
                c.currentY = c.targetY;
                c.alpha = 1.0f;
                c.scale = 1.0f;
                c.rotation = 0.0f;
            }
            for (auto& v : m_logoVoxels) {
                v.currentX = v.targetX;
                v.currentY = v.targetY;
                v.alpha = 1.0f;
            }
        } else if (m_phase == HOLDING) {
            // Start dissolving
            m_phase = DISSOLVING;
            m_elapsed = 0.0f;
        } else if (m_phase == DISSOLVING) {
            // Skip dissolve
            skip();
            return true;
        }
    }

    float screenW = io.DisplaySize.x;
    float screenH = io.DisplaySize.y;

    // --- Shooting star spawning and update ---
    if (m_phase != DONE) {
        m_shootingStarTimer += dt;
        // Spawn a new shooting star every 1.0 to 2.5 seconds
        float spawnInterval = 1.0f + (rand() % 150) * 0.01f;
        if (m_shootingStarTimer >= spawnInterval && m_shootingStars.size() < 5) {
            spawnShootingStar(screenW, screenH);
            m_shootingStarTimer = 0.0f;
        }

        for (auto& ss : m_shootingStars) {
            ss.life += dt;
            ss.x += ss.vx * dt;
            ss.y += ss.vy * dt;
        }
        // Remove dead shooting stars
        m_shootingStars.erase(
            std::remove_if(m_shootingStars.begin(), m_shootingStars.end(),
                [](const ShootingStar& s) { return s.life >= s.maxLife; }),
            m_shootingStars.end());
    }

    // Update ambient particles
    if (m_phase != DONE) {
        // Spawn new particles occasionally
        if (m_totalTime > 0.5f && (rand() % 100) < 8) {
            spawnAmbientParticle(screenW, screenH);
        }
        for (auto& p : m_particles) {
            p.life += dt;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            float lifeRatio = p.life / p.maxLife;
            if (lifeRatio < 0.2f) p.alpha = lifeRatio / 0.2f;
            else if (lifeRatio > 0.8f) p.alpha = (1.0f - lifeRatio) / 0.2f;
            else p.alpha = 1.0f;
        }
        // Remove dead particles
        m_particles.erase(
            std::remove_if(m_particles.begin(), m_particles.end(),
                [](const AmbientParticle& p) { return p.life >= p.maxLife; }),
            m_particles.end());
    }

    // --- Trail particle update (always, they can linger between phases) ---
    for (auto& tp : m_trailParticles) {
        tp.life += dt;
        float lifeRatio = tp.life / tp.maxLife;
        tp.alpha = 1.0f - lifeRatio; // linear fade out
    }
    m_trailParticles.erase(
        std::remove_if(m_trailParticles.begin(), m_trailParticles.end(),
            [](const TrailParticle& tp) { return tp.life >= tp.maxLife; }),
        m_trailParticles.end());

    // --- Dissolve spark update ---
    for (auto& ds : m_dissolveSparks) {
        ds.life += dt;
        ds.x += ds.vx * dt;
        ds.y += ds.vy * dt;
        // Gravity pull
        ds.vy += 80.0f * dt;
        float lifeRatio = ds.life / ds.maxLife;
        ds.alpha = 1.0f - lifeRatio * lifeRatio; // fade with ease-in
    }
    m_dissolveSparks.erase(
        std::remove_if(m_dissolveSparks.begin(), m_dissolveSparks.end(),
            [](const DissolveSpark& s) { return s.life >= s.maxLife; }),
        m_dissolveSparks.end());

    switch (m_phase) {
    case FADE_IN:
        if (m_elapsed >= kFadeInTime) {
            m_phase = LOGO_ASSEMBLE;
            m_elapsed = 0.0f;
        }
        break;

    case LOGO_ASSEMBLE: {
        for (auto& v : m_logoVoxels) {
            float localT = (m_elapsed - v.delay) / (kLogoTime * 0.7f);
            localT = std::clamp(localT, 0.0f, 1.0f);
            // Ease-out back (slight overshoot)
            float t = 1.0f - (1.0f - localT) * (1.0f - localT);
            float overshoot = 1.0f + sinf(localT * 3.14159f) * 0.1f;
            v.currentX = v.startX + (v.targetX - v.startX) * t * overshoot;
            v.currentY = v.startY + (v.targetY - v.startY) * t * overshoot;
            v.alpha = std::clamp(localT * 2.0f, 0.0f, 1.0f);
        }
        if (m_elapsed >= kLogoTime) {
            m_phase = TITLE_IN;
            m_elapsed = 0.0f;
            // Snap logo voxels to final position
            for (auto& v : m_logoVoxels) {
                v.currentX = v.targetX;
                v.currentY = v.targetY;
                v.alpha = 1.0f;
            }
        }
        break;
    }

    case TITLE_IN: {
        const float textOffsetY = 20.0f;
        for (auto& c : m_cubes) {
            float localT = (m_elapsed - c.delay) / kTitleTime;
            localT = std::clamp(localT, 0.0f, 1.0f);
            // Ease-out elastic (slight bounce)
            float t;
            if (localT < 1.0f) {
                float p = 0.3f;
                t = powf(2.0f, -10.0f * localT) * sinf((localT - p / 4.0f) * (2.0f * 3.14159f) / p) + 1.0f;
                t = std::clamp(t, 0.0f, 1.2f);
            } else {
                t = 1.0f;
            }
            c.currentX = c.startX + (c.targetX - c.startX) * t;
            c.currentY = c.startY + (c.targetY - c.startY) * t;
            c.alpha = std::clamp(localT * 3.0f, 0.0f, 1.0f);
            c.scale = std::clamp(localT * 1.5f, 0.0f, 1.0f);
            c.rotation *= (1.0f - localT); // slow down rotation as it settles

            // Spawn trail particles behind flying cubes
            if (localT > 0.03f && localT < 0.80f && (rand() % 100) < 10) {
                TrailParticle tp;
                tp.x = c.currentX;
                tp.y = c.currentY + textOffsetY;
                tp.alpha = 0.7f;
                tp.size = 1.0f + (rand() % 15) * 0.1f;
                tp.color = c.color;
                tp.life = 0.0f;
                tp.maxLife = 0.25f + (rand() % 25) * 0.01f;
                m_trailParticles.push_back(tp);
            }
        }
        if (m_elapsed >= kTitleTime + 0.4f) {
            m_phase = HOLDING;
            m_elapsed = 0.0f;
            for (auto& c : m_cubes) {
                c.currentX = c.targetX;
                c.currentY = c.targetY;
                c.alpha = 1.0f;
                c.scale = 1.0f;
                c.rotation = 0.0f;
            }
        }
        break;
    }

    case HOLDING:
        // Wait for user input (handled above)
        break;

    case DISSOLVING: {
        // Spawn dissolve sparks once at the start
        if (!m_dissolveSparksSpawned) {
            m_dissolveSparksSpawned = true;
            const float textOffsetY = 20.0f;
            const float logoOffsetY = -110.0f;
            // Sparks from text cubes
            for (auto& c : m_cubes) {
                int numSparks = 2 + rand() % 2; // 2-3 per cube
                for (int i = 0; i < numSparks; i++) {
                    DissolveSpark s;
                    s.x = c.targetX;
                    s.y = c.targetY + textOffsetY;
                    float angle = (rand() % 360) * 3.14159f / 180.0f;
                    float speed = 60.0f + (rand() % 180);
                    s.vx = cosf(angle) * speed;
                    s.vy = sinf(angle) * speed;
                    s.alpha = 1.0f;
                    s.size = 1.0f + (rand() % 20) * 0.1f;
                    s.color = c.color;
                    s.life = 0.0f;
                    s.maxLife = 0.3f + (rand() % 50) * 0.01f;
                    m_dissolveSparks.push_back(s);
                }
            }
            // Sparks from logo voxels
            for (auto& v : m_logoVoxels) {
                int numSparks = 1 + rand() % 2;
                for (int i = 0; i < numSparks; i++) {
                    DissolveSpark s;
                    s.x = v.targetX;
                    s.y = v.targetY + logoOffsetY;
                    float angle = (rand() % 360) * 3.14159f / 180.0f;
                    float speed = 50.0f + (rand() % 140);
                    s.vx = cosf(angle) * speed;
                    s.vy = sinf(angle) * speed;
                    s.alpha = 1.0f;
                    s.size = 1.0f + (rand() % 15) * 0.1f;
                    s.color = v.color;
                    s.life = 0.0f;
                    s.maxLife = 0.25f + (rand() % 45) * 0.01f;
                    m_dissolveSparks.push_back(s);
                }
            }
        }

        float t = m_elapsed / kDissolveTime;
        t = std::min(1.0f, t);
        float eased = t * t; // ease-in for accelerating dispersal

        for (auto& c : m_cubes) {
            float dx = c.targetX;
            float dy = c.targetY;
            float dist = sqrtf(dx * dx + dy * dy) + 1.0f;
            float speed = 3.0f * eased * eased;
            c.currentX = c.targetX + (dx / dist) * speed * 600.0f;
            c.currentY = c.targetY + (dy / dist) * speed * 600.0f;
            c.alpha = 1.0f - eased;
            c.scale = 1.0f - eased * 0.5f;
        }
        for (auto& v : m_logoVoxels) {
            float dx = v.targetX;
            float dy = v.targetY - 20.0f;
            float dist = sqrtf(dx * dx + dy * dy) + 1.0f;
            v.currentX = v.targetX + (dx / dist) * eased * 400.0f;
            v.currentY = v.targetY + (dy / dist) * eased * 400.0f;
            v.alpha = 1.0f - eased;
        }

        if (t >= 1.0f) {
            m_phase = DONE;
        }
        break;
    }

    case DONE:
        return true;
    }
    return false;
}

void IntroScreen::render() {
    ImGuiIO& io = ImGui::GetIO();
    float screenW = io.DisplaySize.x;
    float screenH = io.DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::Begin("##Intro", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // --- Background gradient ---
    float fadeAlpha = 1.0f;
    if (m_phase == FADE_IN) fadeAlpha = std::clamp(m_elapsed / kFadeInTime, 0.0f, 1.0f);
    if (m_phase == DISSOLVING) fadeAlpha = std::clamp(1.0f - m_elapsed / kDissolveTime, 0.0f, 1.0f);

    // Deep space gradient (dark blue to black)
    {
        int topR = (int)(10 * fadeAlpha), topG = (int)(15 * fadeAlpha), topB = (int)(40 * fadeAlpha);
        int botR = (int)(5 * fadeAlpha), botG = (int)(5 * fadeAlpha), botB = (int)(15 * fadeAlpha);
        ImU32 colTop = IM_COL32(topR, topG, topB, 255);
        ImU32 colBot = IM_COL32(botR, botG, botB, 255);
        dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(screenW, screenH), colTop, colTop, colBot, colBot);
    }

    // --- Nebula clouds (behind stars) ---
    for (auto& nc : m_nebulaClouds) {
        float drift = sinf(m_totalTime * nc.driftSpeed + nc.driftPhase) * 6.0f;
        float nx = nc.x * screenW + drift;
        float ny = nc.y * screenH + drift * 0.5f;
        float minDim = (screenW < screenH) ? screenW : screenH;
        float nr = nc.radius * minDim;
        uint8_t r = (nc.color >> IM_COL32_R_SHIFT) & 0xFF;
        uint8_t g = (nc.color >> IM_COL32_G_SHIFT) & 0xFF;
        uint8_t b = (nc.color >> IM_COL32_B_SHIFT) & 0xFF;
        uint8_t a = (uint8_t)(nc.baseAlpha * fadeAlpha);
        if (a > 0) {
            dl->AddCircleFilled(ImVec2(nx, ny), nr, IM_COL32(r, g, b, a), 36);
            // Second, larger, even more transparent layer for soft falloff
            dl->AddCircleFilled(ImVec2(nx, ny), nr * 1.4f, IM_COL32(r, g, b, (uint8_t)(a / 3)), 36);
        }
    }

    // --- Stars ---
    for (auto& s : m_stars) {
        float twinkle = 0.5f + 0.5f * sinf(m_totalTime * s.twinkleSpeed + s.twinkleOffset);
        float brightness = s.brightness * twinkle * fadeAlpha;
        int b = (int)(brightness * 255.0f);
        if (b < 5) continue;
        float sx = s.x * screenW;
        float sy = s.y * screenH;
        ImU32 col = IM_COL32(200 + (int)(55 * twinkle), 200 + (int)(55 * twinkle), 255, b);
        if (s.size > 1.5f) {
            dl->AddCircleFilled(ImVec2(sx, sy), s.size, col);
        } else {
            dl->AddRectFilled(ImVec2(sx, sy), ImVec2(sx + s.size, sy + s.size), col);
        }
    }

    // --- Shooting stars ---
    for (auto& ss : m_shootingStars) {
        float lifeRatio = ss.life / ss.maxLife;
        float headAlpha = ss.brightness * (1.0f - lifeRatio * lifeRatio) * fadeAlpha;
        if (headAlpha < 0.02f) continue;

        float speed = sqrtf(ss.vx * ss.vx + ss.vy * ss.vy);
        if (speed < 0.1f) continue;
        float ndx = -ss.vx / speed; // tail direction (opposite of velocity)
        float ndy = -ss.vy / speed;

        float tailLen = 20.0f + speed * 0.06f;
        tailLen *= (1.0f - lifeRatio * 0.3f);

        // Draw tail as multiple segments with decreasing alpha
        const int segs = 5;
        for (int i = 0; i < segs; i++) {
            float t0 = (float)i / segs;
            float t1 = (float)(i + 1) / segs;
            ImVec2 p0(ss.x + ndx * tailLen * t0, ss.y + ndy * tailLen * t0);
            ImVec2 p1(ss.x + ndx * tailLen * t1, ss.y + ndy * tailLen * t1);
            float segAlpha = headAlpha * (1.0f - (t0 + t1) * 0.5f);
            uint8_t a = (uint8_t)(segAlpha * 255.0f);
            float thickness = 1.5f * (1.0f - (t0 + t1) * 0.25f);
            dl->AddLine(p0, p1, IM_COL32(255, 255, 255, a), thickness);
        }

        // Bright head point
        uint8_t ha = (uint8_t)(headAlpha * 255.0f);
        dl->AddCircleFilled(ImVec2(ss.x, ss.y), 2.0f, IM_COL32(255, 255, 240, ha));
    }

    // --- Ambient particles ---
    for (auto& p : m_particles) {
        if (p.alpha <= 0.01f) continue;
        uint8_t r = (p.color >> IM_COL32_R_SHIFT) & 0xFF;
        uint8_t g = (p.color >> IM_COL32_G_SHIFT) & 0xFF;
        uint8_t b = (p.color >> IM_COL32_B_SHIFT) & 0xFF;
        uint8_t a = (uint8_t)(p.alpha * 120.0f * fadeAlpha);
        dl->AddCircleFilled(ImVec2(p.x, p.y), p.size, IM_COL32(r, g, b, a));
    }

    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    // --- Pulsing glow behind title text (during HOLDING phase) ---
    if (m_phase == HOLDING) {
        float pulse = 0.5f + 0.5f * sinf(m_totalTime * 2.5f);
        uint8_t glowAlpha = (uint8_t)(pulse * 22.0f);
        float textOffsetY = 20.0f;
        // Approximate bounding box of title text area
        float glowLeft = centerX - 230.0f;
        float glowRight = centerX + 230.0f;
        float glowTop = centerY + textOffsetY - 45.0f;
        float glowBottom = centerY + textOffsetY + 100.0f;
        dl->AddRectFilled(
            ImVec2(glowLeft, glowTop), ImVec2(glowRight, glowBottom),
            IM_COL32(70, 90, 180, glowAlpha), 10.0f);
        // Outer glow layer (even more subtle)
        dl->AddRectFilled(
            ImVec2(glowLeft - 20.0f, glowTop - 15.0f),
            ImVec2(glowRight + 20.0f, glowBottom + 15.0f),
            IM_COL32(60, 80, 160, (uint8_t)(glowAlpha / 3)), 16.0f);
    }

    // --- Trail particles ---
    for (auto& tp : m_trailParticles) {
        if (tp.alpha <= 0.01f) continue;
        uint8_t r = (tp.color >> IM_COL32_R_SHIFT) & 0xFF;
        uint8_t g = (tp.color >> IM_COL32_G_SHIFT) & 0xFF;
        uint8_t b = (tp.color >> IM_COL32_B_SHIFT) & 0xFF;
        uint8_t a = (uint8_t)(tp.alpha * 140.0f * fadeAlpha);
        float x = centerX + tp.x;
        float y = centerY + tp.y;
        dl->AddCircleFilled(ImVec2(x, y), tp.size, IM_COL32(r, g, b, a));
    }

    // --- Logo voxel cube (centered above text) ---
    if (m_phase >= LOGO_ASSEMBLE) {
        float logoOffsetY = -110.0f; // above center, adjusted for bigger logo
        float voxSize = 10.0f;       // increased from 6.0f to match isoScale

        // Gentle floating/bobbing during HOLDING phase
        if (m_phase == HOLDING) {
            logoOffsetY += sinf(m_totalTime * 1.5f) * 5.0f;
        }

        for (auto& v : m_logoVoxels) {
            if (v.alpha <= 0.01f) continue;
            float x = centerX + v.currentX;
            float y = centerY + v.currentY + logoOffsetY;

            uint8_t r = (v.color >> IM_COL32_R_SHIFT) & 0xFF;
            uint8_t g = (v.color >> IM_COL32_G_SHIFT) & 0xFF;
            uint8_t b = (v.color >> IM_COL32_B_SHIFT) & 0xFF;
            uint8_t a = (uint8_t)(v.alpha * 255.0f);

            // Main face
            dl->AddRectFilled(ImVec2(x, y), ImVec2(x + voxSize, y + voxSize), IM_COL32(r, g, b, a));
            // Subtle highlight on top-left
            dl->AddRectFilled(ImVec2(x, y), ImVec2(x + voxSize, y + 1),
                IM_COL32(std::min(255, r + 60), std::min(255, g + 60), std::min(255, b + 60), a / 2));
            // Subtle shadow on bottom-right
            dl->AddRectFilled(ImVec2(x, y + voxSize - 1), ImVec2(x + voxSize, y + voxSize),
                IM_COL32(r / 2, g / 2, b / 2, a / 2));
        }
    }

    // --- Title text (voxel cubes) ---
    if (m_phase >= TITLE_IN) {
        float cubeSize = 8.0f;
        float textOffsetY = 20.0f; // below center

        for (auto& c : m_cubes) {
            if (c.alpha <= 0.01f) continue;
            float x = centerX + c.currentX;
            float y = centerY + c.currentY + textOffsetY;
            float s = cubeSize * c.scale;
            if (s < 0.5f) continue;

            uint8_t r = (c.color >> IM_COL32_R_SHIFT) & 0xFF;
            uint8_t g = (c.color >> IM_COL32_G_SHIFT) & 0xFF;
            uint8_t b = (c.color >> IM_COL32_B_SHIFT) & 0xFF;
            uint8_t a = (uint8_t)(c.alpha * 255.0f);

            // Glow behind cube (subtle)
            if (c.alpha > 0.5f) {
                uint8_t ga = (uint8_t)((c.alpha - 0.5f) * 2.0f * 30.0f);
                dl->AddRectFilled(ImVec2(x - 2, y - 2), ImVec2(x + s + 2, y + s + 2),
                    IM_COL32(r, g, b, ga));
            }

            float cx = x + (cubeSize - s) * 0.5f;
            float cy = y + (cubeSize - s) * 0.5f;
            dl->AddRectFilled(ImVec2(cx, cy), ImVec2(cx + s - 1, cy + s - 1), IM_COL32(r, g, b, a));
        }
    }

    // --- Dissolve sparks ---
    if (m_phase == DISSOLVING) {
        for (auto& ds : m_dissolveSparks) {
            if (ds.alpha <= 0.01f) continue;
            uint8_t r = (ds.color >> IM_COL32_R_SHIFT) & 0xFF;
            uint8_t g = (ds.color >> IM_COL32_G_SHIFT) & 0xFF;
            uint8_t b = (ds.color >> IM_COL32_B_SHIFT) & 0xFF;
            // Sparks are brighter than their source color
            r = (uint8_t)std::min(255, r + 80);
            g = (uint8_t)std::min(255, g + 80);
            b = (uint8_t)std::min(255, b + 80);
            uint8_t a = (uint8_t)(ds.alpha * 220.0f);
            float x = centerX + ds.x;
            float y = centerY + ds.y;
            dl->AddCircleFilled(ImVec2(x, y), ds.size, IM_COL32(r, g, b, a));
        }
    }

    // --- Subtitle and version info ---
    if (m_phase == HOLDING || m_phase == DISSOLVING) {
        float subAlpha = 1.0f;
        if (m_phase == HOLDING && m_elapsed < 0.4f) subAlpha = m_elapsed / 0.4f;
        if (m_phase == DISSOLVING) subAlpha = std::max(0.0f, 1.0f - m_elapsed / (kDissolveTime * 0.6f));

        // Subtitle
        {
            const char* subtitle = "Voxel Simulation Architect";
            ImVec2 textSize = ImGui::CalcTextSize(subtitle);
            float tx = centerX - textSize.x * 0.5f;
            float ty = centerY + 160.0f;
            // Shadow
            dl->AddText(ImVec2(tx + 1, ty + 1), IM_COL32(0, 0, 0, (uint8_t)(subAlpha * 120)), subtitle);
            dl->AddText(ImVec2(tx, ty), IM_COL32(180, 190, 220, (uint8_t)(subAlpha * 220)), subtitle);
        }

        // Version text - more prominent with shadow and underline
        {
            const char* version = "Beta v0.2";
            ImFont* font = ImGui::GetFont();
            float versionFontSize = 16.0f;
            float defaultFontSize = ImGui::GetFontSize();
            float fontScale = versionFontSize / defaultFontSize;

            ImVec2 vSize = ImGui::CalcTextSize(version);
            vSize.x *= fontScale;
            vSize.y *= fontScale;
            float vx = centerX - vSize.x * 0.5f;
            float vy = centerY + 185.0f;

            // Shadow
            dl->AddText(font, versionFontSize, ImVec2(vx + 1, vy + 1),
                IM_COL32(0, 0, 0, (uint8_t)(subAlpha * 90)), version);
            // Main text (brighter than before)
            dl->AddText(font, versionFontSize, ImVec2(vx, vy),
                IM_COL32(150, 160, 210, (uint8_t)(subAlpha * 200)), version);
            // Subtle underline
            float ulY = vy + vSize.y + 3.0f;
            float ulHalfW = vSize.x * 0.45f;
            dl->AddLine(
                ImVec2(centerX - ulHalfW, ulY),
                ImVec2(centerX + ulHalfW, ulY),
                IM_COL32(130, 140, 190, (uint8_t)(subAlpha * 80)), 1.0f);
        }

        // "Press any key" blinking text
        if (m_phase == HOLDING) {
            float blink = 0.5f + 0.5f * sinf(m_totalTime * 3.0f);
            const char* skipText = "Press any key to continue";
            ImVec2 skipSize = ImGui::CalcTextSize(skipText);
            float skx = centerX - skipSize.x * 0.5f;
            float sky = screenH - 60.0f;
            dl->AddText(ImVec2(skx, sky),
                IM_COL32(150, 160, 200, (uint8_t)(blink * subAlpha * 180)), skipText);
        }

        // Decorative line under title
        {
            float lineW = 200.0f * subAlpha;
            float lineY = centerY + 148.0f;
            // Gradient line (fading at edges)
            for (int i = 0; i < 3; ++i) {
                float spread = (float)i * 0.3f;
                uint8_t la = (uint8_t)(subAlpha * (80 - i * 20));
                dl->AddLine(
                    ImVec2(centerX - lineW * (1.0f + spread), lineY + (float)i),
                    ImVec2(centerX + lineW * (1.0f + spread), lineY + (float)i),
                    IM_COL32(100, 140, 200, la), 1.0f);
            }
            // Small diamond accent at center
            float dSize = 4.0f;
            dl->AddQuadFilled(
                ImVec2(centerX, lineY - dSize),
                ImVec2(centerX + dSize, lineY),
                ImVec2(centerX, lineY + dSize),
                ImVec2(centerX - dSize, lineY),
                IM_COL32(140, 170, 230, (uint8_t)(subAlpha * 120)));
        }
    }

    // --- Scanline overlay (CRT retro-futuristic effect) ---
    {
        uint8_t scanAlpha = (uint8_t)(8.0f * fadeAlpha);
        if (scanAlpha > 0) {
            float step = 3.0f;
            for (float y = 0.0f; y < screenH; y += step) {
                dl->AddLine(ImVec2(0, y), ImVec2(screenW, y),
                    IM_COL32(0, 0, 0, scanAlpha), 1.0f);
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
