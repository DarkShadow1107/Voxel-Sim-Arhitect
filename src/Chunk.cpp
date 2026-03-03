#include "Chunk.hpp"
#include "FastNoiseLite.h"
#include "StructureGenerator.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

// Initialize the static pool allocator for up to 4096 chunks
PoolAllocator Chunk::s_allocator(kVoxelCount * sizeof(uint8_t), 4096);

Chunk::Chunk() {
    m_voxels = static_cast<uint8_t*>(s_allocator.allocate());
    if (m_voxels) {
        std::memset(m_voxels, 0, kVoxelCount);
    } else {
        std::cerr << "WARNING: Chunk pool exhausted — using heap fallback to prevent crash." << std::endl;
        m_voxels = new uint8_t[kVoxelCount](); // value-init to 0
        m_heapAllocated = true;
    }
}

Chunk::~Chunk() {
    if (m_voxels) {
        if (m_heapAllocated) delete[] m_voxels;
        else                  s_allocator.deallocate(m_voxels);
    }
}

uint8_t Chunk::get(int x, int y, int z) const {
    if (!m_voxels || x < 0 || y < 0 || z < 0 || x >= SizeX || y >= SizeY || z >= SizeZ) {
        return 0;
    }
    return m_voxels[(size_t)idx(x, y, z)];
}

void Chunk::set(int x, int y, int z, uint8_t v) {
    if (!m_voxels || x < 0 || y < 0 || z < 0 || x >= SizeX || y >= SizeY || z >= SizeZ) {
        return;
    }
    m_voxels[(size_t)idx(x, y, z)] = v;
}

BiomeType Chunk::getBiomeAt(FastNoiseLite& biomeNoise, FastNoiseLite& continentalNoise,
                             FastNoiseLite& mountainNoise, float wx, float wz) {
    if (continentalNoise.GetNoise(wx, wz) < -0.30f) return BIOME_OCEAN;

    float bn = biomeNoise.GetNoise(wx, wz);
    float mn = mountainNoise.GetNoise(wx, wz);

    // Mountains: threshold 0.50 keeps ranges dramatic but not ubiquitous.
    // Allowed across a wide biome band to include snowy mountain ranges.
    if (mn > 0.50f && bn > -0.18f && bn < 0.82f) return BIOME_MOUNTAINS;

    if (bn < -0.20f) return BIOME_POLAR;
    if (bn <  0.05f) return BIOME_SNOWY;
    if (bn <  0.35f) return BIOME_PLAINS;
    if (bn <  0.50f) return BIOME_SAVANNA;
    if (bn <  0.65f) return BIOME_DESERT;
    if (bn <  0.85f) return BIOME_JUNGLE;
    return BIOME_ASHWORLD;
}

void Chunk::generateTerrain(FastNoiseLite& noise, int seed, float frequency, int baseHeight, int offsetX, int offsetZ) {
    if (!m_voxels) return;

    // -----------------------------------------------------------------------
    // Noise setup — multiple layers for realistic terrain
    // -----------------------------------------------------------------------

    // 1. Base terrain (medium-scale undulations)
    noise.SetSeed(seed);
    noise.SetFrequency(frequency);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFractalLacunarity(2.0f);
    noise.SetFractalGain(0.5f);

    // 2. Mountain noise — FBm for large rounded peaks
    FastNoiseLite mountainNoise;
    mountainNoise.SetSeed(seed + 1);
    mountainNoise.SetFrequency(frequency * 0.55f);
    mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    mountainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    mountainNoise.SetFractalOctaves(4);
    mountainNoise.SetFractalLacunarity(2.0f);
    mountainNoise.SetFractalGain(0.5f);

    // 3. Ridge noise — sharp mountain ridges and canyons
    FastNoiseLite ridgeNoise;
    ridgeNoise.SetSeed(seed + 3);
    ridgeNoise.SetFrequency(frequency * 0.55f);
    ridgeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    ridgeNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    ridgeNoise.SetFractalOctaves(3);
    ridgeNoise.SetFractalLacunarity(2.0f);
    ridgeNoise.SetFractalGain(0.4f);

    // 4. Detail noise — high-frequency micro-variation
    FastNoiseLite detailNoise;
    detailNoise.SetSeed(seed + 4);
    detailNoise.SetFrequency(frequency * 3.5f);
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // 5. River noise — low-frequency for wide river systems
    FastNoiseLite riverNoise;
    riverNoise.SetSeed(seed + 5);
    riverNoise.SetFrequency(std::max(0.0020f, frequency * 0.09f));
    riverNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // 6. Continental noise — determines ocean vs land at macro scale
    FastNoiseLite continentalNoise;
    continentalNoise.SetSeed(seed + 10);
    continentalNoise.SetFrequency(frequency * 0.07f);
    continentalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    continentalNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    continentalNoise.SetFractalOctaves(3);

    // 7. Biome noise — large-scale temperature/humidity
    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(seed + 20);
    biomeNoise.SetFrequency(frequency * 0.04f);
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    biomeNoise.SetFractalOctaves(2);

    // 8. 3D ore noise — vein-shaped ore placement
    FastNoiseLite oreNoise;
    oreNoise.SetSeed(seed + 50);
    oreNoise.SetFrequency(0.08f);
    oreNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    // 9. Domain-warp X — offsets mountain/ridge sampling for organic shapes
    FastNoiseLite warpNoiseX;
    warpNoiseX.SetSeed(seed + 60);
    warpNoiseX.SetFrequency(frequency * 0.28f);
    warpNoiseX.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    warpNoiseX.SetFractalType(FastNoiseLite::FractalType_FBm);
    warpNoiseX.SetFractalOctaves(3);

    // 10. Domain-warp Z
    FastNoiseLite warpNoiseZ;
    warpNoiseZ.SetSeed(seed + 70);
    warpNoiseZ.SetFrequency(frequency * 0.28f);
    warpNoiseZ.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    warpNoiseZ.SetFractalType(FastNoiseLite::FractalType_FBm);
    warpNoiseZ.SetFractalOctaves(3);

    // 11. Erosion noise — softens mountain faces to create eroded cliff look
    FastNoiseLite erosionNoise;
    erosionNoise.SetSeed(seed + 80);
    erosionNoise.SetFrequency(frequency * 1.8f);
    erosionNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // 12. Mountain Mask — very-low-frequency noise controls WHERE mountains can appear.
    //     At values below 0.35 (mm01) the mask is 0 (flat land); above 0.55 it is 1 (full
    //     mountain eligible). A smoothstep blend between those thresholds prevents hard seams.
    //     Using a lower frequency than mountainNoise means the mask changes slowly over the
    //     world, creating continent-scale mountain ranges rather than scattered peaks.
    FastNoiseLite mountainMaskNoise;
    mountainMaskNoise.SetSeed(seed + 90);
    mountainMaskNoise.SetFrequency(frequency * 0.03f);
    mountainMaskNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    mountainMaskNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    mountainMaskNoise.SetFractalOctaves(2);
    mountainMaskNoise.SetFractalLacunarity(2.0f);
    mountainMaskNoise.SetFractalGain(0.5f);

    // 13. Infernal noise — very-low-frequency continental overlay that forces
    //     BIOME_ASHWORLD in large scattered patches regardless of temperature.
    //     Separate from biomeNoise so ashworld and jungle both have their own
    //     full biome noise range; neither displaces the other.
    FastNoiseLite infernalNoise;
    infernalNoise.SetSeed(seed + 95);
    infernalNoise.SetFrequency(frequency * 0.022f);
    infernalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    infernalNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    infernalNoise.SetFractalOctaves(2);

    std::memset(m_voxels, 0, kVoxelCount);

    // seaLevel scales with worldBaseHeight so ocean surface matches land elevation.
    const int seaLevel = baseHeight + 6;

    // -----------------------------------------------------------------------
    // Biome boundary thresholds (must match getBiomeAt())
    // -----------------------------------------------------------------------
    // bn < -0.20 → POLAR, < 0.05 → SNOWY, < 0.35 → PLAINS (Badlands),
    // < 0.50 → SAVANNA, < 0.65 → DESERT, < 0.85 → JUNGLE, else ASHWORLD
    // Infernal override (in_val > 0.36) → ASHWORLD regardless of temperature
    // MOUNTAINS: detected via mountainNoise > 0.50 within temperate range
    static constexpr float kBiomeThr[] = { -0.20f, 0.05f, 0.35f, 0.50f, 0.65f, 0.85f };
    static constexpr float kBlendR     = 0.08f;  // blend radius at biome borders (~100 world blocks)

    for (int z = 0; z < SizeZ; ++z) {
        for (int x = 0; x < SizeX; ++x) {
            float wx = (float)(x + offsetX);
            float wz = (float)(z + offsetZ);

            // Sample noise layers
            const float n   = noise.GetNoise(wx, wz);            // base  [-1,1]
            // Domain-warp offsets for organic mountain shapes
            const float wx_warp = wx + warpNoiseX.GetNoise(wx, wz) * 20.0f;
            const float wz_warp = wz + warpNoiseZ.GetNoise(wx, wz) * 20.0f;
            const float mn  = mountainNoise.GetNoise(wx_warp, wz_warp); // mountain (warped)
            const float rn  = ridgeNoise.GetNoise(wx_warp, wz_warp);    // ridge (warped)
            const float dn  = detailNoise.GetNoise(wx, wz);      // detail
            const float cn  = continentalNoise.GetNoise(wx, wz); // continental
            const float bn  = biomeNoise.GetNoise(wx, wz);       // biome
            const float en  = erosionNoise.GetNoise(wx, wz);     // erosion [-1,1]
            const float in_val = infernalNoise.GetNoise(wx, wz); // infernal patch

            // Normalise to [0,1]
            const float h01 = (n  + 1.0f) * 0.5f;
            const float m01 = (mn + 1.0f) * 0.5f;
            const float r01 = rn;               // Ridged is already [0,1]
            const float d01 = (dn + 1.0f) * 0.5f;

            // Mountain mask: smoothstep 0→1 between mask values 0.35–0.55.
            // Multiplied into height contributions so mountain magnitude fades to zero
            // outside mountain-eligible zones, leaving flat/hilly terrain there instead.
            const float mmn        = mountainMaskNoise.GetNoise(wx, wz);
            const float mm01       = (mmn + 1.0f) * 0.5f;
            float mountainMask     = std::clamp((mm01 - 0.35f) / 0.20f, 0.0f, 1.0f);
            mountainMask           = mountainMask * mountainMask * (3.0f - 2.0f * mountainMask);

            // Classify biome using the domain-warped mountain noise (mn) so the biome
            // boundary matches exactly what the height formula uses.  Calling getBiomeAt()
            // here would use unwarped mn and produce height/biome mismatches that appear
            // as sudden cliff faces or floating terrain at mountain borders.
            BiomeType biome;
            if      (cn < -0.30f)                                                      biome = BIOME_OCEAN;
            else if (mn > 0.50f && bn > -0.18f && bn < 0.82f && mountainMask > 0.25f) biome = BIOME_MOUNTAINS;
            else if (in_val > 0.36f)                               biome = BIOME_ASHWORLD; // infernal override
            else if (bn < -0.20f)                                  biome = BIOME_POLAR;
            else if (bn <  0.05f)                                  biome = BIOME_SNOWY;
            else if (bn <  0.35f)                                  biome = BIOME_PLAINS;
            else if (bn <  0.50f)                                  biome = BIOME_SAVANNA;
            else if (bn <  0.65f)                                  biome = BIOME_DESERT;
            else if (bn <  0.85f)                                  biome = BIOME_JUNGLE;
            else                                                   biome = BIOME_ASHWORLD;
            const bool isPolar     = (biome == BIOME_POLAR);
            const bool isSnowy     = (biome == BIOME_SNOWY);
            const bool isPlains    = (biome == BIOME_PLAINS);
            const bool isSavanna   = (biome == BIOME_SAVANNA);
            const bool isDesert    = (biome == BIOME_DESERT);
            const bool isJungle    = (biome == BIOME_JUNGLE);
            const bool isAshworld  = (biome == BIOME_ASHWORLD);
            const bool isMountains = (biome == BIOME_MOUNTAINS);
            const bool isOcean     = (biome == BIOME_OCEAN);

            // ------------------------------------------------------------------
            // Per-biome height function (float for smooth blending)
            // ------------------------------------------------------------------
            float fh;
            const float bh = (float)baseHeight;

            if (isOcean) {
                // Continental shelf model: depth increases further from shore.
                // cn approaches -1 in deep ocean, just below -0.30 near coast.
                float depthFactor = std::max(0.0f, -cn - 0.30f) / 0.70f;  // 0=shelf, 1=abyss
                float shelfDepth  = depthFactor * 26.0f;                    // 0-26 block dive

                // Seamount ridges: occasional upwellings using height noise
                float seamount    = std::max(0.0f, h01 - 0.62f) * 30.0f;
                // Trenches: narrow depressions dig deeper in abyssal zones
                float trench      = r01 * 5.0f * depthFactor;

                float baseFloor   = (float)seaLevel - 5.0f - shelfDepth;   // always 5 below sea
                fh = std::max(2.0f, baseFloor + seamount - trench + d01 * 2.5f);
            } else if (isAshworld) {
                // Rolling ash plains with ridge vents; lava rivers carved below
                float ashBase = m01 * 35.0f;
                float ventH   = r01 * 10.0f;
                fh = bh + 10.0f + ashBase + ventH;
                fh = std::min(fh, (float)(SizeY - 20));
                // Lava river carving: narrow channels depress height
                const float avr  = std::abs(riverNoise.GetNoise(wx, wz));
                const float aW   = 0.045f;
                const float aStr = std::clamp((aW - avr) / aW, 0.0f, 1.0f);
                if (aStr > 0.0f) {
                    float lavaBedH = (float)(seaLevel + 2);
                    fh = fh * (1.0f - aStr) + lavaBedH * aStr;
                }
            } else if (isMountains) {
                // ── Enhanced dramatic mountain generation ─────────────────────────────
                // Architecture:
                //   mountainMask  — world-scale gate; prevents scattered random peaks
                //   broadBase     — wide footing for the whole range
                //   ridgedPeak    — r01^3 PRIMARY shape: nearly flat valleys, sharp peaks
                //   peakMass      — mStrength^1.5 dense-core extra height
                //   ridgeEdge     — r01^2 for knife-edge arêtes and sub-ridges
                //   cliffDetail   — high-freq erosion for rugged Dolomite-like faces
                //   valleyFactor  — U-shaped alpine glacial valley carving
                //   lateralNoise  — secondary noise layer for peak asymmetry
                // ─────────────────────────────────────────────────────────────────────
                float e01_m = (en + 1.0f) * 0.5f;

                // Base elevation: all mountains start well above sea level
                float mountainBase = bh + 24.0f;

                // Large-scale undulation — varies the general height of the range
                float foundation = h01 * 22.0f;

                // mStrength ramps from 0 at detection threshold (mn=0.50, m01=0.75)
                constexpr float kMtnThresh01 = 0.75f;
                float mStrength = std::max(0.0f, (m01 - kMtnThresh01) / (1.0f - kMtnThresh01));

                // Broad dome base — gives the range a natural wide footprint
                float broadBase = std::sqrt(mStrength) * 30.0f * mountainMask;

                // PRIMARY shape: ridged noise CUBED — valley floors near zero, peaks soar
                //   r01=0.50 → 0.125×120 = 15  (valley, nearly flat)
                //   r01=0.75 → 0.422×120 = 51  (moderate ridge)
                //   r01=0.90 → 0.729×120 = 87  (prominent peak)
                //   r01=0.97 → 0.913×120 = 110 (dramatic Matterhorn-class spire)
                float ridgedPeak = std::pow(r01, 3.0f) * 120.0f * mountainMask;

                // SECONDARY: peak mass gives extra bulk in the dense range core
                // mStrength^1.5 is less steep than ^2 so moderate ranges still bulk up
                float peakMass = std::pow(mStrength, 1.5f) * 72.0f * mountainMask;

                // Accent ridgelines: r01^2 retains more knife-edge coverage
                float ridgeEdge = std::pow(r01, 2.0f) * 38.0f * mountainMask;

                // Lateral asymmetry — secondary noise shifts peak flanks for realism
                float lateralShift = (dn + 1.0f) * 0.5f;  // 0..1
                float peakAsymm = lateralShift * mStrength * 14.0f * mountainMask;

                float combinedH = mountainBase + foundation + broadBase + ridgedPeak + peakMass + ridgeEdge + peakAsymm;

                // Allow very tall peaks but taper extreme outliers gently
                if (combinedH > bh + 148.0f) {
                    float excess = combinedH - (bh + 148.0f);
                    combinedH = (bh + 148.0f) + excess * 0.08f; // gentle taper, huge peaks still visible
                }

                // Cliff-face erosion — rough texture on steep slopes, smooth on peaks
                float erosionCarve = e01_m * 11.0f * (1.0f - mStrength * 0.55f);
                // Extra micro-detail: small-scale roughness for realistic cliff textures
                float detailCarve = d01 * 4.5f * (1.0f - mStrength * 0.4f);

                // U-shaped glacial valley carving between ridge spines
                float valleyFactor = std::max(0.0f, 0.55f - m01) * 38.0f * mountainMask;

                fh = combinedH - erosionCarve - detailCarve - valleyFactor;
                fh = std::min(fh, (float)(SizeY - 4));
            } else if (isPolar) {
                // Nearly flat, very slight undulation — almost perfectly level ice sheet
                fh = bh + h01 * 3.0f + d01 * 1.0f;
            } else if (isSnowy) {
                // Dramatic snowy highlands: rolling hills, icy ridges, frozen lakebeds
                float snowHill  = h01 * 20.0f;                               // taller hills
                float ridgePush = r01 * 16.0f * std::max(0.0f, m01 - 0.42f); // rolling snow ridges
                float lakeDip   = std::max(0.0f, -h01 + 0.22f) * 8.0f;       // frozen lake depressions
                fh = bh + snowHill + ridgePush + d01 * 3.5f - lakeDip;
            } else if (isPlains) {
                // Rolling temperate plains — gentle hills, wide open meadows
                // Low-amplitude terrain with smooth undulation and occasional rises.
                float plainBase = h01 * 14.0f;           // gentle rolling hills up to 14 blocks
                float gentleRidge = r01 * 4.0f;           // very mild ridgeline variation
                float meadowDip   = std::max(0.0f, 0.35f - h01) * 5.0f; // shallow bowl depressions
                fh = bh + 4.0f + plainBase + gentleRidge + d01 * 3.0f - meadowDip;
                fh = std::min(fh, bh + 30.0f);
            } else if (isSavanna) {
                // Flat plains punctuated by steep mesa formations
                float mesaRaw  = std::max(0.0f, m01 - 0.70f) / 0.30f; // 0..1
                float mesaH    = mesaRaw * mesaRaw * 28.0f;            // squared for sharp edge
                // Hard cap the mesa top so it's table-flat (sharper than before)
                if (mesaH > 18.0f) mesaH = 18.0f + (mesaH - 18.0f) * 0.10f;
                fh = bh + h01 * 5.0f + mesaH + d01 * 1.5f;
            } else if (isDesert) {
                // Wind-sculpted dunes — tall crests, sharp interdunal troughs
                float e01_d     = (en + 1.0f) * 0.5f;
                float duneBase  = h01 * 18.0f;                          // taller base
                float duneCrest = r01 * 18.0f * std::max(0.3f, m01);   // prominent crests
                float trough    = e01_d * 4.0f * (1.0f - h01);         // interdunal hollows
                fh = bh + duneBase + duneCrest + d01 * 2.5f - trough;
            } else if (isJungle) {
                // Dense, dramatic jungle highlands — steep ridges and deep misty valleys
                float e01_j    = (en + 1.0f) * 0.5f;
                float jungleH  = h01 * 32.0f;                           // tall jungle hills
                float ridgeStr = r01 * 38.0f * std::max(0.0f, m01 - 0.25f); // dramatic ridges
                float valleyCarve = e01_j * 5.0f * (1.0f - h01);
                fh = bh + jungleH + ridgeStr + d01 * 4.5f - valleyCarve;
            } else {
                // Generic land (fallback)
                fh = bh + h01 * 14.0f + d01 * 2.0f;
            }

            // ------------------------------------------------------------------
            // Biome transition blending — smooth-step toward a neutral height
            // when we're close to any biome boundary
            // ------------------------------------------------------------------
            if (!isOcean) {
                float neutralH = bh + h01 * 10.0f; // neutral blended height
                for (float thr : kBiomeThr) {
                    float dist = std::abs(bn - thr);
                    if (dist < kBlendR) {
                        float t = dist / kBlendR;                      // 0 near boundary, 1 away
                        t = t * t * (3.0f - 2.0f * t);                // smoothstep
                        fh = fh * t + neutralH * (1.0f - t);
                        break;
                    }
                }

                // Continental coastal push-down: ease land to seaLevel near ocean edge
                if (cn < 0.08f && cn > -0.35f) {
                    float coastT = std::clamp((cn + 0.35f) / 0.43f, 0.0f, 1.0f); // 0=ocean,1=land
                    coastT = coastT * coastT;  // ease in
                    float coastH = (float)seaLevel + 1.0f;
                    fh = fh * coastT + coastH * (1.0f - coastT);
                }

                // Mountain boundary blend: prevent cliff seam at mn = 0.50 threshold.
                // Columns just inside the mountain zone blend their height back toward
                // neutral so the transition to the adjacent flat/hilly biome is gradual.
                float mBorderDist = mn - 0.50f;
                if (mBorderDist >= 0.0f && mBorderDist < 0.08f) {
                    float t = mBorderDist / 0.08f;
                    t = t * t * (3.0f - 2.0f * t); // smoothstep: 0 at border, 1 inside
                    float nonMtnH = bh + h01 * 10.0f;
                    fh = fh * t + nonMtnH * (1.0f - t);
                }
            }

            // ------------------------------------------------------------------
            // River carving — blend height toward river bed
            // (Ashworld uses its own lava channel system; excluded here)
            // Badlands: dry biome — no rivers
            // ------------------------------------------------------------------
            if (!isDesert && !isPolar && !isOcean && !isAshworld && !isPlains) {
                const float rv   = std::abs(riverNoise.GetNoise(wx, wz));
                const float rW   = 0.075f;  // wider channels than 0.060
                const float rStr = std::clamp((rW - rv) / rW, 0.0f, 1.0f);
                if (rStr > 0.0f) {
                    const float riverBedH = (float)(seaLevel - 3);
                    fh = fh * (1.0f - rStr) + riverBedH * rStr;
                }
            }

            int h = std::clamp((int)fh, 1, SizeY - 1);

            // ------------------------------------------------------------------
            // Fill voxel column
            // ------------------------------------------------------------------
            for (int y = 0; y < SizeY; ++y) {
                if (y == 0) {
                    set(x, y, z, BLOCK_BEDROCK);
                    continue;
                }

                if (y > h) {
                    // Air above surface — ocean/polar fill handled below
                    if (y < seaLevel) {
                        if (isPolar) set(x, y, z, (y >= seaLevel - 1) ? BLOCK_ICE : BLOCK_WATER);
                        else         set(x, y, z, BLOCK_WATER);
                    }
                    continue;
                }

                // y <= h (solid)
                uint8_t type = BLOCK_STONE;

                if (y == h) {
                    // Surface block
                    if (isOcean) {
                        // Depth-based ocean floor material:
                        // abyssal zone → stone, mid-ocean → gravel, shelf → sand
                        if      (h <= seaLevel - 18) type = BLOCK_STONE;
                        else if (h <= seaLevel - 10) type = BLOCK_GRAVEL;
                        else                         type = BLOCK_SAND;
                    } else if (isPolar) {
                        type = (y <= seaLevel) ? BLOCK_ICE : BLOCK_SNOW;
                    } else if (isSnowy) {
                        if      (h > bh + 58 || h > seaLevel + 2) type = BLOCK_SNOW;
                        else if (h >= seaLevel - 1)                type = BLOCK_ICE;
                        else                                        type = BLOCK_SAND;
                    } else if (isDesert) {
                        type = BLOCK_SAND;
                    } else if (isPlains) {
                        // Rolling plains: grass surface, sandy near water
                        type = (y <= seaLevel + 1) ? BLOCK_SAND : BLOCK_GRASS;
                    } else if (isSavanna) {
                        // Mesa top: stone/sandstone strata; low ground: grass/sand
                        float mesaRaw = std::max(0.0f, m01 - 0.70f) / 0.30f;
                        if (mesaRaw > 0.5f) type = (((y / 3) % 2 == 0)) ? BLOCK_SAND : BLOCK_STONE;
                        else                type = (y <= seaLevel + 1) ? BLOCK_SAND : BLOCK_GRASS;
                    } else if (isJungle) {
                        type = (y <= seaLevel + 1) ? BLOCK_SAND : BLOCK_GRASS;
                    } else if (isAshworld) {
                        // Dark ashen surface: basalt outcrops or ash coating
                        type = (m01 > 0.78f) ? BLOCK_BASALT : BLOCK_ASH;
                    } else if (isMountains) {
                        // Enhanced altitude-banded surface blocks for dramatic alpine look:
                        // bh+120: permanent deep snowfield (near summit)
                        // bh+95:  wind-scoured bare stone
                        // bh+78:  gravel/scree (freeze-thaw fractured rock)
                        // bh+62:  mossy stone (sheltered cliff ledges)
                        // bh+46:  alpine grass meadow (tree-line)
                        // bh+32:  earthy foothills
                        // below:  dirt/grass valley floor
                        if      (h > bh + 120) type = BLOCK_SNOW;
                        else if (h > bh + 95)  type = BLOCK_STONE;
                        else if (h > bh + 78)  type = BLOCK_GRAVEL;
                        else if (h > bh + 62)  type = BLOCK_MOSSY_STONE;
                        else if (h > bh + 46)  type = BLOCK_GRASS;
                        else if (h > bh + 32)  type = BLOCK_DIRT;
                        else                   type = BLOCK_GRASS;
                    } else {
                        // Default temperate (fallback biome)
                        if      (h > bh + 78)        type = BLOCK_SNOW;
                        else if (y <= seaLevel + 2)  type = BLOCK_SAND;
                        else                         type = BLOCK_GRASS;
                    }
                } else if (y == h - 1 || y == h - 2 || y == h - 3) {
                    // Sub-surface layer
                    if (isDesert || isOcean) {
                        type = BLOCK_SAND;
                    } else if (isPlains) {
                        // Plains sub-surface: dirt
                        type = BLOCK_DIRT;
                    } else if (isPolar) {
                        type = BLOCK_STONE;
                    } else if (isSavanna) {
                        float mesaRaw = std::max(0.0f, m01 - 0.70f) / 0.30f;
                        type = (mesaRaw > 0.5f) ? BLOCK_STONE : BLOCK_DIRT;
                    } else if (isAshworld) {
                        type = BLOCK_BASALT;
                    } else if (isMountains) {
                        // Rich cliff strata: STONE / GRAVEL / MOSSY_STONE / COBBLESTONE
                        // cycling every 5 blocks. Upper elevations: bare stone & gravel.
                        // Lower: mossy rock & cobblestone (sheltered wet ledges).
                        int stratum = (y / 5) % 4;
                        if (h > bh + 80) {
                            type = (stratum <= 1) ? BLOCK_STONE : BLOCK_GRAVEL;
                        } else if (h > bh + 50) {
                            type = (stratum == 0) ? BLOCK_STONE
                                 : (stratum == 1) ? BLOCK_GRAVEL
                                 : (stratum == 2) ? BLOCK_MOSSY_STONE : BLOCK_COBBLESTONE;
                        } else {
                            type = (stratum == 0) ? BLOCK_MOSSY_STONE
                                 : (stratum == 1) ? BLOCK_COBBLESTONE : BLOCK_STONE;
                        }
                    } else {
                        type = BLOCK_DIRT;
                    }
                } else {
                    // Deep underground — ore veins via 3D noise
                    float ov = oreNoise.GetNoise((float)(x + offsetX), (float)y, (float)(z + offsetZ));
                    float oa = std::abs(ov);

                    if (isAshworld) {
                        // Ashworld underground: basalt base, pumice veins, deep obsidian
                        if        (y < 20 && oa > 0.72f) { type = BLOCK_DIAMOND_ORE; }
                        else if   (y < 40 && oa > 0.68f) { type = BLOCK_GOLD_ORE;    }
                        else if   (y < 65 && oa > 0.64f) { type = BLOCK_IRON_ORE;    }
                        else if   (oa > 0.60f && oa < 0.70f) { type = BLOCK_PUMICE;  }
                        else                              { type = BLOCK_BASALT;      }
                        // Deep obsidian veins in ashworld
                        if (y > 40 && oa > 0.65f) type = BLOCK_OBSIDIAN;
                    } else if (isMountains) {
                        // Mountains: ore-rich underground
                        if        (y <  20 && oa > 0.70f) { type = BLOCK_DIAMOND_ORE; }
                        else if   (y <  40 && oa > 0.65f) { type = BLOCK_GOLD_ORE;    }
                        else if   (y <  65 && oa > 0.60f) { type = BLOCK_IRON_ORE;    }
                        else if   (y < 100 && oa > 0.56f) { type = BLOCK_COAL_ORE;    }
                        else                              { type = BLOCK_STONE;        }
                    } else {
                        if        (y <  20 && oa > 0.72f) { type = BLOCK_DIAMOND_ORE; }
                        else if   (y <  40 && oa > 0.68f) { type = BLOCK_GOLD_ORE;    }
                        else if   (y <  65 && oa > 0.64f) { type = BLOCK_IRON_ORE;    }
                        else if   (y < 100 && oa > 0.60f) { type = BLOCK_COAL_ORE;    }
                        else                              { type = BLOCK_STONE;        }
                    }
                }

                set(x, y, z, type);
            }

            // ------------------------------------------------------------------
            // Ashworld lava vent — pre-fill lava at narrow channel depressions
            // (wide craters removed; lava rivers are carved via height formula)
            // ------------------------------------------------------------------
            if (isAshworld) {
                const float avr  = std::abs(riverNoise.GetNoise(wx, wz));
                const float aW   = 0.045f;
                const float aStr = std::clamp((aW - avr) / aW, 0.0f, 1.0f);
                if (aStr > 0.6f && h > 0 && h < seaLevel + 10) {
                    for (int wy = h + 1; wy <= seaLevel + 4 && wy < SizeY; ++wy)
                        if (get(x, wy, z) == BLOCK_AIR)
                            set(x, wy, z, BLOCK_LAVA);
                }
            }

            // ------------------------------------------------------------------
            // Flora & structures
            // ------------------------------------------------------------------
            if (h > seaLevel && h < SizeY - 24) {
                // Deterministic per-column hash — replaces rand() so tree/flora
                // positions are stable across sessions and chunk re-generations.
                // Combines world-space X, Z and seed to produce a unique value
                // for every column without any global state.
                uint32_t ch = (uint32_t)((int)wx * 1664525 + (int)wz * 1013904223 + seed * 22695477);
                ch ^= ch >> 16;
                ch *= 0x45d9f3bu;
                ch ^= ch >> 16;
                int r = (int)(ch % 2000);
                float treeN = noise.GetNoise(wx * 0.08f, wz * 0.08f);

                // Forest zone gating: use treeN to create natural clearings and
                // dense patches instead of uniform random scatter.
                const bool inForestZone = (treeN > -0.10f);  // ~55% of columns eligible
                const bool inDenseZone  = (treeN >  0.35f);  // ~32% → dense canopy
                const bool inFlowerZone = (treeN >  0.28f);  // flowers cluster in meadows

                if (isAshworld) {
                    // Ashworld: obsidian spires + lava vent fire sparks — no flora
                    if (h + 1 < SizeY && h > seaLevel + 2) {
                        if      (r < 3)  StructureGenerator::generateObsidianSpire(this, x, h + 1, z);
                        else if (r < 15) set(x, h + 1, z, BLOCK_FIRE);
                    }

                } else if (isMountains) {
                    // Mountains: dense pine forests in foothills, alpine meadows, snow above
                    if (h + 1 < SizeY) {
                        if (h < bh + 32 && r < 18 && inForestZone)
                            // Valley forest: regular or pine trees
                            StructureGenerator::generatePineTree(this, x, h + 1, z);
                        else if (h >= bh + 32 && h < bh + 46 && r < 22 && inForestZone)
                            // Foothill pine forest — denser
                            StructureGenerator::generatePineTree(this, x, h + 1, z);
                        else if (h >= bh + 46 && h < bh + 62 && r < 18 && inDenseZone)
                            // Alpine pine treeline — sparse
                            StructureGenerator::generatePineTree(this, x, h + 1, z);
                        // Alpine meadow flowers in grass/moss band
                        else if (h >= bh + 46 && h < bh + 64 && r < 100 && inFlowerZone)
                            set(x, h + 1, z, (ch % 3 == 0) ? BLOCK_FLOWER_BLUE : BLOCK_FLOWER_RED);
                        // Sparse tall grass in valley floors
                        else if (h < bh + 32 && r < 280)
                            set(x, h + 1, z, BLOCK_TALL_GRASS);
                        // Upper scree: patchy snow (transition zone)
                        else if (h >= bh + 78 && h < bh + 95 && r < 500)
                            set(x, h + 1, z, BLOCK_SNOW_LAYER);
                        // Snowfield: near-total coverage above ~95 blocks
                        else if (h >= bh + 95 && r < 1600)
                            set(x, h + 1, z, BLOCK_SNOW_LAYER);
                    }

                } else if (isDesert) {
                    // Desert: cacti on sand only, well-spaced
                    if (r < 6 && get(x, h, z) == BLOCK_SAND)
                        StructureGenerator::generateCactus(this, x, h + 1, z);

                } else if (isJungle) {
                    // Jungle: dense canopy — mega trees rare, regular trees everywhere
                    int treeCut = inDenseZone ? 55 : (inForestZone ? 30 : 8);
                    if (r < treeCut) {
                        if   (r < 3) StructureGenerator::generateMegaTree(this, x, h + 1, z);
                        else         StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_LEAVES);
                    } else if (r == 101) {
                        StructureGenerator::generateRuins(this, x, h + 1, z);
                    } else if (!inForestZone && r < 350) {
                        // Open clearings in jungle get thick undergrowth
                        set(x, h + 1, z, BLOCK_TALL_GRASS);
                    } else if (r < 45 && inFlowerZone) {
                        set(x, h + 1, z, BLOCK_FLOWER_RED);
                    }

                } else if (isSavanna) {
                    // Savanna: sparse trees on flat ground; mesa tops stay bare
                    float mesaRaw = std::max(0.0f, m01 - 0.70f) / 0.30f;
                    if (r < 7 && mesaRaw < 0.4f && inForestZone)
                        StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_LEAVES);
                    else if (r < 350)
                        set(x, h + 1, z, BLOCK_TALL_GRASS);

                } else if (isSnowy || isPolar) {
                    // Cold biomes: sparse pine trees, abundant snow layers
                    if (r < 8 && inForestZone)
                        StructureGenerator::generatePineTree(this, x, h + 1, z);
                    else if (r < 1400)
                        set(x, h + 1, z, BLOCK_SNOW_LAYER);

                } else if (isPlains) {
                    // Rolling plains: grasslands with trees, flowers, villages
                    int treeCut = inDenseZone ? 22 : (inForestZone ? 9 : 0);
                    if (treeCut > 0 && r < treeCut) {
                        // Mix of oak and birch trees across the plains
                        if      (treeN > 0.30f)  StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_BIRCH_WOOD, BLOCK_BIRCH_LEAVES);
                        else                      StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD, BLOCK_LEAVES);
                    } else if (r == 88) {
                        StructureGenerator::generateSmallHouse(this, x, h + 1, z);
                    } else if (r < 55 && inFlowerZone) {
                        set(x, h + 1, z, (ch % 2 == 0) ? BLOCK_FLOWER_RED : BLOCK_FLOWER_BLUE);
                    } else if (r < 300) {
                        set(x, h + 1, z, BLOCK_TALL_GRASS);
                    }

                } else {
                    // Generic mixed forest (snowy highlands, etc.)
                    int treeCut = inDenseZone ? 28 : (inForestZone ? 11 : 0);
                    if (treeCut > 0 && r < treeCut) {
                        if      (treeN > 0.45f)  StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_CHERRY_WOOD, BLOCK_CHERRY_LEAVES);
                        else if (treeN < -0.45f) StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_BIRCH_WOOD,  BLOCK_BIRCH_LEAVES);
                        else                      StructureGenerator::generateTree(this, x, h + 1, z, BLOCK_WOOD,        BLOCK_LEAVES);
                    } else if (r == 100) {
                        StructureGenerator::generateSmallHouse(this, x, h + 1, z);
                    } else if (r < 40 && inFlowerZone) {
                        set(x, h + 1, z, BLOCK_FLOWER_RED);
                    } else if (r < 65 && inFlowerZone) {
                        set(x, h + 1, z, BLOCK_FLOWER_BLUE);
                    } else if (r < 250) {
                        set(x, h + 1, z, BLOCK_TALL_GRASS);
                    }
                }
            }
        }
    }

    generateCaves(noise, seed);
    generateWaterbodies(noise, seed, frequency, baseHeight, offsetX, offsetZ);
}

void Chunk::generateCaves(FastNoiseLite& noise, int seed) {
    if (!m_voxels) return;

    // -----------------------------------------------------------------------
    // Three overlapping cave layers to create varied underground geometry:
    //   Layer 1 — Large open caverns  (FBm, low freq, large blobs)
    //   Layer 2 — Narrow tunnels      (FBm range-threshold → tube cross-section)
    //   Layer 3 — Worm passages       (different axis orientation)
    // -----------------------------------------------------------------------

    // Layer 1: Large caverns
    FastNoiseLite cavernNoise;
    cavernNoise.SetSeed(seed + 2);
    cavernNoise.SetFrequency(0.035f);
    cavernNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    cavernNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    cavernNoise.SetFractalOctaves(2);
    cavernNoise.SetFractalLacunarity(2.0f);
    cavernNoise.SetFractalGain(0.5f);

    // Layer 2: Narrow tunnels — tube shape from double-sided threshold
    FastNoiseLite tunnelNoise;
    tunnelNoise.SetSeed(seed + 7);
    tunnelNoise.SetFrequency(0.055f);
    tunnelNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    tunnelNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    tunnelNoise.SetFractalOctaves(2);

    // Layer 3: Worm passages (different orientation via y/z swap)
    FastNoiseLite wormNoise;
    wormNoise.SetSeed(seed + 13);
    wormNoise.SetFrequency(0.060f);
    wormNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    for (int y = 3; y < SizeY - 1; ++y) {  // Start at y=3: preserve y=1,2 above bedrock
        //                                   // Prevents h=0 surface when terrain hugs bedrock
        float cavernThr = (y < 40) ? 0.52f : 0.58f;
        // Tunnels disabled above y=80 to preserve surface terrain
        bool tunnelsActive = (y < 80);

        for (int z = 0; z < SizeZ; ++z) {
            for (int x = 0; x < SizeX; ++x) {
                uint8_t cur = get(x, y, z);
                // Only carve stone (preserve ores, dirt, bedrock, etc.)
                if (cur != BLOCK_STONE && cur != BLOCK_DIRT) continue;

                // Layer 1 — large cavern blob
                float cv = cavernNoise.GetNoise((float)x, (float)y, (float)z);
                if (cv > cavernThr) {
                    set(x, y, z, BLOCK_AIR);
                    // Underground lake: fill carved voids at/below water table with water
                    if (y <= 10 && y > 1)
                        set(x, y, z, BLOCK_WATER);
                    continue;
                }

                if (!tunnelsActive) continue;

                // Layer 2 — narrow tunnel (tube via range test)
                float tv = tunnelNoise.GetNoise((float)x, (float)y, (float)z);
                float ta = std::abs(tv);
                if (ta > 0.62f && ta < 0.76f) {
                    set(x, y, z, BLOCK_AIR);
                    if (y <= 10 && y > 1)
                        set(x, y, z, BLOCK_WATER);
                    continue;
                }

                // Layer 3 — worm passage (swap y↔z axis for perpendicular tunnels)
                float wv = wormNoise.GetNoise((float)x, (float)z, (float)y);
                float wa = std::abs(wv);
                if (wa > 0.65f && wa < 0.78f) {
                    set(x, y, z, BLOCK_AIR);
                    if (y <= 10 && y > 1)
                        set(x, y, z, BLOCK_WATER);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// generateWaterbodies — second-pass after terrain+caves
//
// Pre-fills rivers, highland lakes, waterfalls, desert oases and volcano lava
// so they appear immediately on generation.  These blocks are STATIC — they
// only start simulating when the player interacts near them (like Minecraft).
//
// 1. River water      : fills river channels with BLOCK_WATER up to seaLevel.
// 2. Highland lake    : fills enclosed depressions above seaLevel.
// 3. Waterfall        : detects cliff edges, pre-fills falling column.
// 4. Desert oasis     : small water holes in desert terrain.
// 5. Volcano lava     : lava source blocks at high-peak columns.
// ---------------------------------------------------------------------------
void Chunk::generateWaterbodies(FastNoiseLite& noise, int seed, float frequency,
                                int baseHeight, int offsetX, int offsetZ) {
    if (!m_voxels) return;

    const int seaLevel = baseHeight + 6;

    FastNoiseLite riverNoise;
    riverNoise.SetSeed(seed + 5);
    riverNoise.SetFrequency(std::max(0.0020f, frequency * 0.09f));
    riverNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite continentalNoise;
    continentalNoise.SetSeed(seed + 10);
    continentalNoise.SetFrequency(frequency * 0.07f);
    continentalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    continentalNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    continentalNoise.SetFractalOctaves(3);

    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(seed + 20);
    biomeNoise.SetFrequency(frequency * 0.04f);
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    biomeNoise.SetFractalOctaves(2);

    FastNoiseLite mountainNoise;
    mountainNoise.SetSeed(seed + 1);
    mountainNoise.SetFrequency(frequency * 0.55f);
    mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    mountainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    mountainNoise.SetFractalOctaves(4);

    FastNoiseLite oasisNoise;
    oasisNoise.SetSeed(seed + 60);
    oasisNoise.SetFrequency(frequency * 1.5f);
    oasisNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite infernalNoise;
    infernalNoise.SetSeed(seed + 95);
    infernalNoise.SetFrequency(frequency * 0.022f);
    infernalNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    infernalNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    infernalNoise.SetFractalOctaves(2);

    // Pre-compute surface height (topmost TERRAIN block) per column.
    // Vegetation, decorations, and player-built structures are excluded so
    // waterfalls are never anchored to tree canopies or flower tops.
    auto isTerrainBlock = [](uint8_t b) -> bool {
        switch (b) {
            case BLOCK_AIR:
            case BLOCK_WATER:
            case BLOCK_LAVA:
            case BLOCK_WOOD:        case BLOCK_BIRCH_WOOD:   case BLOCK_CHERRY_WOOD:
            case BLOCK_LEAVES:      case BLOCK_BIRCH_LEAVES: case BLOCK_CHERRY_LEAVES:
            case BLOCK_TALL_GRASS:  case BLOCK_FLOWER_RED:   case BLOCK_FLOWER_BLUE:
            case BLOCK_SNOW_LAYER:  case BLOCK_FIRE:
            case BLOCK_OAK_PLANKS:  case BLOCK_COBBLESTONE:  case BLOCK_MOSSY_STONE:
                return false;
            default:
                return true;
        }
    };

    int heights[SizeX][SizeZ] = {};
    for (int z2 = 0; z2 < SizeZ; ++z2)
        for (int x2 = 0; x2 < SizeX; ++x2)
            for (int y = SizeY - 1; y >= 1; --y) {
                if (isTerrainBlock(get(x2, y, z2))) {
                    heights[x2][z2] = y; break;
                }
            }

    const int ddx[4] = {1, -1, 0, 0};
    const int ddz[4] = {0, 0, 1, -1};

    for (int z = 0; z < SizeZ; ++z) {
        for (int x = 0; x < SizeX; ++x) {
            float wx = (float)(x + offsetX);
            float wz = (float)(z + offsetZ);

            BiomeType biome = getBiomeAt(biomeNoise, continentalNoise, mountainNoise, wx, wz);
            if (infernalNoise.GetNoise(wx, wz) > 0.36f && biome != BIOME_OCEAN) biome = BIOME_ASHWORLD;
            const bool isDesert    = (biome == BIOME_DESERT);
            const bool isPolar     = (biome == BIOME_POLAR);
            const bool isOcean     = (biome == BIOME_OCEAN);
            const bool isAshworld  = (biome == BIOME_ASHWORLD);
            const bool isMountains = (biome == BIOME_MOUNTAINS);
            const bool isPlains    = (biome == BIOME_PLAINS);

            const int h = heights[x][z];

            // ------------------------------------------------------------------
            // 1. River water — fill channel from carved bed up to seaLevel
            // ------------------------------------------------------------------
            if (!isDesert && !isPolar && !isOcean && !isAshworld && !isPlains) {
                const float rv   = std::abs(riverNoise.GetNoise(wx, wz));
                const float rW   = 0.075f;  // wider channels than 0.060
                const float rStr = std::clamp((rW - rv) / rW, 0.0f, 1.0f);
                if (rStr > 0.0f && h > 0 && h < seaLevel) {
                    for (int wy = h + 1; wy <= seaLevel && wy < SizeY; ++wy)
                        if (get(x, wy, z) == BLOCK_AIR)
                            set(x, wy, z, BLOCK_WATER);
                }
            }

            // ------------------------------------------------------------------
            // 1b. Ashworld lava rivers — fill channel with lava instead of water
            // ------------------------------------------------------------------
            if (isAshworld) {
                const float avr  = std::abs(riverNoise.GetNoise(wx, wz));
                const float aW   = 0.045f;
                const float aStr = std::clamp((aW - avr) / aW, 0.0f, 1.0f);
                if (aStr > 0.0f && h > 0 && h < seaLevel + 8) {
                    for (int wy = h + 1; wy <= seaLevel + 4 && wy < SizeY; ++wy)
                        if (get(x, wy, z) == BLOCK_AIR)
                            set(x, wy, z, BLOCK_LAVA);
                }
            }

            // ------------------------------------------------------------------
            // 2. Highland lake — fill enclosed depressions above seaLevel.
            //    A column qualifies if AT LEAST 3 of 4 immediate neighbours are
            //    higher than it (relaxed from "all 4 must be higher" to allow
            //    natural shallow bowls and curved shorelines).
            //    Mountains: extended lake range up to h<80
            // ------------------------------------------------------------------
            {
                int lakeCap = isMountains ? 80 : 60;
                if (!isDesert && !isPolar && !isOcean && !isAshworld &&
                    h > seaLevel + 2 && h < lakeCap) {
                    int higherCount = 0;
                    int  minNeighbor  = SizeY;
                    bool edgeChunk = false;
                    for (int d = 0; d < 4; ++d) {
                        int nx2 = x + ddx[d], nz2 = z + ddz[d];
                        if (nx2 < 0 || nx2 >= SizeX || nz2 < 0 || nz2 >= SizeZ) {
                            edgeChunk = true; break;
                        }
                        int nh = heights[nx2][nz2];
                        if (nh > h) higherCount++;
                        if (nh < minNeighbor) minNeighbor = nh;
                    }
                    // Require at least 3 of 4 neighbours to be higher (allows
                    // natural curved shorelines instead of only isolated pixels).
                    if (!edgeChunk && higherCount >= 3) {
                        int lakeTop = std::min(minNeighbor - 1, h + 3);
                        for (int wy = h + 1; wy <= lakeTop && wy < SizeY; ++wy)
                            if (get(x, wy, z) == BLOCK_AIR)
                                set(x, wy, z, BLOCK_WATER);
                    }
                }
            }

            // ------------------------------------------------------------------
            // 3. Waterfall — source block on cliff top + pre-filled falling column
            //    Mountains: lower drop threshold (>=2) for dramatic mountain cascades.
            //    Others: threshold 3 (was 4) — more scenic waterfalls across all biomes.
            //    Minimum height above sea level lowered to 6 (was 10) to allow lower
            //    elevation cliff-face waterfalls.
            // ------------------------------------------------------------------
            {
                int wfMinDrop = isMountains ? 2 : 3;
                if (!isDesert && !isPolar && !isOcean && !isAshworld && h > seaLevel + 6) {
                    int maxDrop = 0, dropDX = 0, dropDZ = 0;
                    for (int d = 0; d < 4; ++d) {
                        int nx2 = x + ddx[d], nz2 = z + ddz[d];
                        if (nx2 < 0 || nx2 >= SizeX || nz2 < 0 || nz2 >= SizeZ) continue;
                        int drop = h - heights[nx2][nz2];
                        if (drop > maxDrop) { maxDrop = drop; dropDX = ddx[d]; dropDZ = ddz[d]; }
                    }

                    if (maxDrop >= wfMinDrop) {
                        float wfN = noise.GetNoise(wx * 4.0f, wz * 4.0f);
                        if (wfN > 0.28f) {  // lowered from 0.35 — more scenic waterfalls
                            // Source block — only place on empty air (never overwrite a tree trunk)
                            if (h + 1 < SizeY && get(x, h + 1, z) == BLOCK_AIR)
                                set(x, h + 1, z, BLOCK_WATER);

                            // Pre-fill cliff-face column: AIR-only (already guarded)
                            const int cliffX  = x + dropDX;
                            const int cliffZ  = z + dropDZ;
                            const int colTop  = h;
                            const int colLen  = std::min(maxDrop - 1, 16);  // was 10, allow longer falls
                            const int colBot  = colTop - colLen + 1;
                            for (int fy = colTop; fy >= colBot; --fy) {
                                if (fy >= 1 && fy < SizeY &&
                                    get(cliffX, fy, cliffZ) == BLOCK_AIR)
                                    set(cliffX, fy, cliffZ, BLOCK_WATER);
                            }
                        }
                    }
                }
            }

            // ------------------------------------------------------------------
            // 4. Desert oasis — small water hole surrounded by sand
            // ------------------------------------------------------------------
            if (isDesert && h > seaLevel + 2 && h < 40) {
                float ov = oasisNoise.GetNoise(wx, wz);
                if (ov > 0.80f) {
                    // Only replace sand surface — never overwrite a cactus base
                    if (h > 0 && h < SizeY && get(x, h, z) == BLOCK_SAND)
                        set(x, h, z, BLOCK_WATER);
                }
            }

            // ------------------------------------------------------------------
            // 5. Ashworld lava source at high-peak columns (ventvent sparks)
            // ------------------------------------------------------------------
            if (isAshworld && h > 45) {
                float lvN = noise.GetNoise(wx * 2.0f, wz * 2.0f);
                if (lvN > 0.82f && h + 1 < SizeY && get(x, h + 1, z) == BLOCK_AIR)
                    set(x, h + 1, z, BLOCK_LAVA);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Schedule fluid updates for adjacent-to-air fluid blocks.
    //
    // OCEAN water is NEVER scheduled — it is fully static on load.
    // The setBlock() wake-up handles activation when a player digs adjacent.
    // Rivers, lakes, waterfalls, and lava sources ARE scheduled.
    // -----------------------------------------------------------------------
    BiomeType colBiome[SizeX][SizeZ];
    for (int z2 = 0; z2 < SizeZ; ++z2)
        for (int x2 = 0; x2 < SizeX; ++x2) {
            BiomeType cb = getBiomeAt(biomeNoise, continentalNoise, mountainNoise,
                                      (float)(x2 + offsetX), (float)(z2 + offsetZ));
            if (infernalNoise.GetNoise((float)(x2 + offsetX), (float)(z2 + offsetZ)) > 0.36f
                && cb != BIOME_OCEAN)
                cb = BIOME_ASHWORLD;
            colBiome[x2][z2] = cb;
        }

    // -----------------------------------------------------------------------
    // Schedule fluid updates: only the TOPMOST fluid block per column.
    //
    // Scheduling every fluid-adjacent-to-air block in a river (10+ blocks
    // deep across 32 columns) floods the update queue on load, causing severe
    // frame drops and eventual crashes.  Only the surface block actually needs
    // to spread; the fluid system will naturally propagate downward from there.
    // -----------------------------------------------------------------------
    m_fluidUpdates.clear();
    for (int z = 0; z < SizeZ; ++z) {
        for (int x = 0; x < SizeX; ++x) {
            // Ocean water stays static — never schedule it.
            if (colBiome[x][z] == BIOME_OCEAN) continue;

            // Find the topmost fluid block in this column.
            for (int y = SizeY - 2; y >= 1; --y) {
                uint8_t b = get(x, y, z);
                if (b != BLOCK_WATER && b != BLOCK_LAVA) continue;

                // Only activate if the block has at least one air neighbor
                // (a fully enclosed fluid block won't flow anywhere).
                bool hasAirNeighbor = false;
                if (                             get(x,     y - 1, z    ) == BLOCK_AIR) hasAirNeighbor = true;
                else if (x > 0           &&      get(x - 1, y,     z    ) == BLOCK_AIR) hasAirNeighbor = true;
                else if (x < SizeX - 1   &&      get(x + 1, y,     z    ) == BLOCK_AIR) hasAirNeighbor = true;
                else if (z > 0           &&      get(x,     y,     z - 1) == BLOCK_AIR) hasAirNeighbor = true;
                else if (z < SizeZ - 1   &&      get(x,     y,     z + 1) == BLOCK_AIR) hasAirNeighbor = true;

                if (hasAirNeighbor)
                    m_fluidUpdates.push_back((x << 16) | (y << 8) | z);

                // Stop at the first (topmost) fluid — no need to queue the
                // rest of the column.
                break;
            }
        }
    }
}
