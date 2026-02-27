#pragma once

#include <vector>
#include <cstdint>

class IntroScreen {
public:
    void init();
    bool update(float dt); // returns true when intro is done
    void render();
    void skip();           // skip to end immediately

private:
    struct VoxelCube {
        float targetX, targetY;
        float startX, startY;
        float currentX, currentY;
        uint32_t color;
        float alpha = 1.0f;
        float delay = 0.0f;
        float rotation = 0.0f;   // spin while flying in
        float scale = 1.0f;
    };

    struct Star {
        float x, y;
        float brightness;
        float twinkleSpeed;
        float twinkleOffset;
        float size;
    };

    struct ShootingStar {
        float x, y;           // screen coordinates
        float vx, vy;
        float life;
        float maxLife;
        float brightness;
    };

    struct NebulaCloud {
        float x, y;           // normalized 0-1
        float radius;          // normalized fraction of screen
        uint32_t color;
        float baseAlpha;
        float driftSpeed;
        float driftPhase;
    };

    struct TrailParticle {
        float x, y;           // relative to screen center (same space as cubes)
        float alpha;
        float size;
        uint32_t color;
        float life;
        float maxLife;
    };

    struct DissolveSpark {
        float x, y;           // relative to screen center
        float vx, vy;
        float alpha;
        float size;
        uint32_t color;
        float life;
        float maxLife;
    };

    struct AmbientParticle {
        float x, y;
        float vx, vy;
        float alpha;
        float size;
        uint32_t color;
        float life;
        float maxLife;
    };

    // Logo voxel block (isometric 3D cube made of mini voxels)
    struct LogoVoxel {
        float targetX, targetY;
        float currentX, currentY;
        float startX, startY;
        uint32_t color;
        float alpha = 0.0f;
        float delay = 0.0f;
    };

    std::vector<VoxelCube> m_cubes;
    std::vector<Star> m_stars;
    std::vector<ShootingStar> m_shootingStars;
    std::vector<NebulaCloud> m_nebulaClouds;
    std::vector<TrailParticle> m_trailParticles;
    std::vector<DissolveSpark> m_dissolveSparks;
    std::vector<AmbientParticle> m_particles;
    std::vector<LogoVoxel> m_logoVoxels;
    float m_elapsed = 0.0f;
    float m_totalTime = 0.0f;
    bool m_skipped = false;
    bool m_dissolveSparksSpawned = false;
    float m_shootingStarTimer = 0.0f;

    enum Phase { FADE_IN, LOGO_ASSEMBLE, TITLE_IN, HOLDING, DISSOLVING, DONE };
    Phase m_phase = FADE_IN;

    static constexpr float kFadeInTime   = 0.3f;   // was 0.6f — snappier start
    static constexpr float kLogoTime     = 1.0f;   // was 1.8f — logo assembles faster
    static constexpr float kTitleTime    = 0.7f;   // was 1.2f — title animates faster
    static constexpr float kHoldTime     = 2.0f;   // unchanged — time to read
    static constexpr float kDissolveTime = 0.5f;   // was 0.8f — quick exit

    void initStars();
    void initLogo();
    void initNebulaClouds();
    void spawnAmbientParticle(float screenW, float screenH);
    void spawnShootingStar(float screenW, float screenH);

    // 5x7 bitmap font
    static const uint8_t kFont[128][7];
};
