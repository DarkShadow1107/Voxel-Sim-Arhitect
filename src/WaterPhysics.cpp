#include "WaterPhysics.hpp"
#include "Chunk.hpp"
#include "World.hpp"

// ---------------------------------------------------------------------------
// Coordinate packing — identical layout to the legacy fluidKey lambda.
// ---------------------------------------------------------------------------

int64_t WaterPhysics::key(int x, int y, int z) noexcept {
    return ((int64_t)((x + 32768) & 0xFFFF) << 32)
         | ((int64_t)(y & 0xFF) << 16)
         | ((int64_t)((z + 32768) & 0xFFFF));
}

void WaterPhysics::decode(int64_t k, int& x, int& y, int& z) noexcept {
    z = (int)(k & 0xFFFF) - 32768;
    y = (int)((k >> 16) & 0xFF);
    x = (int)((k >> 32) & 0xFFFF) - 32768;
}

// ---------------------------------------------------------------------------

// O(1) — inserts a single chunk key. No per-block scan needed.
void WaterPhysics::registerChunk(const Chunk* /*chunk*/, int chunkX, int chunkZ) {
    m_staticChunks.insert(chunkKey(chunkX, chunkZ));
}

// O(1) — removes the chunk from the static registry.
// Orphaned entries in m_activated whose chunk was (chunkX,chunkZ) are benign:
// isStatic() checks m_staticChunks first, so those entries are never visible.
void WaterPhysics::unregisterChunk(int chunkX, int chunkZ) {
    m_staticChunks.erase(chunkKey(chunkX, chunkZ));
}

// ---------------------------------------------------------------------------

bool WaterPhysics::isStatic(int x, int y, int z) const noexcept {
    // Derive chunk coordinates using floor-division (handles negatives).
    int cx = (x >= 0) ? (x / Chunk::SizeX)
                      : ((x - Chunk::SizeX + 1) / Chunk::SizeX);
    int cz = (z >= 0) ? (z / Chunk::SizeZ)
                      : ((z - Chunk::SizeZ + 1) / Chunk::SizeZ);

    if (!m_staticChunks.count(chunkKey(cx, cz))) return false;
    return m_activated.count(key(x, y, z)) == 0;
}

// ---------------------------------------------------------------------------

void WaterPhysics::onBlockRemoved(World& world, int bx, int by, int bz) {
    static constexpr int adx[] = { 1,-1, 0, 0, 0, 0 };
    static constexpr int ady[] = { 0, 0, 1,-1, 0, 0 };
    static constexpr int adz[] = { 0, 0, 0, 0, 1,-1 };
    for (int i = 0; i < 6; ++i) {
        int ax = bx + adx[i];
        int ay = by + ady[i];
        int az = bz + adz[i];
        if (ay < 0 || ay >= Chunk::SizeY) continue; // guard out-of-bounds y
        if (!isStatic(ax, ay, az)) continue;
        int64_t k = key(ax, ay, az);
        m_activated.insert(k);                       // mark as no longer static
        uint8_t b = world.getBlock(ax, ay, az);
        m_dist[k] = 0;
        if (blockIsLava(b)) m_lavaQ.push_back(k);
        else                m_waterQ.push_back(k);
    }
}

void WaterPhysics::activateFluid(int x, int y, int z, bool isLava) {
    int64_t k = key(x, y, z);
    m_activated.insert(k);
    m_dist[k] = 0;
    if (isLava) m_lavaQ.push_back(k);
    else        m_waterQ.push_back(k);
}

void WaterPhysics::onFluidPlaced(int x, int y, int z, bool isLava) {
    int64_t k = key(x, y, z);
    m_activated.insert(k); // player-placed = always active
    m_dist[k] = 0;
    if (isLava) m_lavaQ.push_back(k);
    else        m_waterQ.push_back(k);
}
