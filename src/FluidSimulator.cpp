#include "FluidSimulator.hpp"
#include "Chunk.hpp"
#include "World.hpp"

// ---------------------------------------------------------------------------
//  Coordinate encoding — identical layout to WaterPhysics for compatibility.
//  x,z range: ±32 768 blocks from origin.  y range: 0-255.
// ---------------------------------------------------------------------------

int64_t FluidSimulator::encodeKey(int x, int y, int z) noexcept {
    return ((int64_t)((x + 32768) & 0xFFFF) << 32)
         | ((int64_t)( y          & 0xFF  ) << 16)
         | ((int64_t)((z + 32768) & 0xFFFF));
}

void FluidSimulator::decodeKey(int64_t k, int& x, int& y, int& z) noexcept {
    z = (int)( k        & 0xFFFF) - 32768;
    y = (int)((k >> 16) & 0xFF);
    x = (int)((k >> 32) & 0xFFFF) - 32768;
}

uint64_t FluidSimulator::chunkKey(int cx, int cz) noexcept {
    return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32)
         |  static_cast<uint64_t>(static_cast<uint32_t>(cz));
}

// ---------------------------------------------------------------------------
//  Internal helper — derive chunk coords for (x,z) using floor-division.
// ---------------------------------------------------------------------------

int64_t FluidSimulator::_chunkLocalKey(int x, int y, int z,
                                        int& cx, int& cz) const noexcept {
    cx = (x >= 0) ? (x / Chunk::SizeX) : ((x - Chunk::SizeX + 1) / Chunk::SizeX);
    cz = (z >= 0) ? (z / Chunk::SizeZ) : ((z - Chunk::SizeZ + 1) / Chunk::SizeZ);
    return encodeKey(x, y, z);
}

// ---------------------------------------------------------------------------
//  Chunk lifecycle
// ---------------------------------------------------------------------------

void FluidSimulator::registerChunk(const Chunk* /*chunk*/, int chunkX, int chunkZ) {
    m_dormantChunks.insert(chunkKey(chunkX, chunkZ));
}

void FluidSimulator::unregisterChunk(int chunkX, int chunkZ) {
    m_dormantChunks.erase(chunkKey(chunkX, chunkZ));
}

// ---------------------------------------------------------------------------
//  State query
// ---------------------------------------------------------------------------

bool FluidSimulator::isDormant(int x, int y, int z) const noexcept {
    int cx, cz;
    int64_t k = 0;
    // Derive chunk without calling _chunkLocalKey (that needs non-const this)
    cx = (x >= 0) ? (x / Chunk::SizeX) : ((x - Chunk::SizeX + 1) / Chunk::SizeX);
    cz = (z >= 0) ? (z / Chunk::SizeZ) : ((z - Chunk::SizeZ + 1) / Chunk::SizeZ);
    k  = encodeKey(x, y, z);

    if (!m_dormantChunks.count(chunkKey(cx, cz))) return false;
    return m_activated.count(k) == 0;
}

// ---------------------------------------------------------------------------
//  Block-Update events
// ---------------------------------------------------------------------------

void FluidSimulator::onBlockChanged(int bx, int by, int bz) {
    // Push all six face-adjacent positions to the event queue.
    static constexpr int adx[] = { 1,-1, 0, 0, 0, 0 };
    static constexpr int ady[] = { 0, 0, 1,-1, 0, 0 };
    static constexpr int adz[] = { 0, 0, 0, 0, 1,-1 };
    for (int i = 0; i < 6; ++i)
        m_events.push({ bx + adx[i], by + ady[i], bz + adz[i] });
}

// ---------------------------------------------------------------------------
//  Player-placed fluid — immediately ACTIVE
// ---------------------------------------------------------------------------

void FluidSimulator::onFluidPlaced(int x, int y, int z, bool isLava) {
    int64_t k = encodeKey(x, y, z);
    m_activated.insert(k);   // wake even if within a registered chunk
    m_dist[k] = 0;
    if (isLava) m_lavaQ.push_back(k);
    else        m_waterQ.push_back(k);
}

// ---------------------------------------------------------------------------
//  Tick: drain BlockUpdateEvents → transition DORMANT neighbours to ACTIVE
// ---------------------------------------------------------------------------

int FluidSimulator::processTick(World& world, int limit) {
    int processed = 0;
    while (!m_events.empty() && processed < limit) {
        auto ev = m_events.front();
        m_events.pop();
        ++processed;

        const int ax = ev.x, ay = ev.y, az = ev.z;
        if (ay < 0 || ay >= Chunk::SizeY) continue; // guard y bounds

        if (!isDormant(ax, ay, az)) continue;        // already ACTIVE or non-fluid

        int64_t k  = encodeKey(ax, ay, az);
        uint8_t b  = world.getBlock(ax, ay, az);

        if (b == BLOCK_WATER || b == BLOCK_LAVA) {
            // Transition DORMANT → ACTIVE
            m_activated.insert(k);
            m_dist[k] = 0;
            if (b == BLOCK_LAVA) m_lavaQ.push_back(k);
            else                 m_waterQ.push_back(k);

            // Cascade: newly active block may expose further dormant neighbours.
            onBlockChanged(ax, ay, az);
        } else {
            // Block was already replaced (mined, evaporated, etc.) — just evict.
            m_activated.insert(k);  // prevent re-checking same stale position
        }
    }
    return processed;
}

// ---------------------------------------------------------------------------

void FluidSimulator::clear() {
    while (!m_events.empty()) m_events.pop();
    m_waterQ.clear();
    m_lavaQ.clear();
    m_dist.clear();
    m_dormantChunks.clear();
    m_activated.clear();
}
