#pragma once
// ---------------------------------------------------------------------------
// WaterPhysics — tracks generation-placed static fluids.
// Generation water/lava stays quiescent until a neighbour block is removed.
//
// Design: chunk-level static registry (O(1) register/unregister).
//   m_staticChunks  — set of chunk keys whose generation fluids are still at rest.
//   m_activated     — individual blocks that have been explicitly woken up.
//   A block is "static" when its chunk is in m_staticChunks AND the block
//   itself is NOT in m_activated.
//   On chunk unload, only the single chunk key is erased — no per-block scan.
// ---------------------------------------------------------------------------
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

class Chunk;
class World;

class WaterPhysics {
public:
    // Encode / decode world-space block coordinates into a compact 64-bit key.
    static int64_t  key(int x, int y, int z) noexcept;
    static void     decode(int64_t k, int& x, int& y, int& z) noexcept;

    // Pack (chunkX, chunkZ) into a single 64-bit value for m_staticChunks.
    static uint64_t chunkKey(int cx, int cz) noexcept {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32)
             |  static_cast<uint64_t>(static_cast<uint32_t>(cz));
    }

    // Mark an entire freshly-generated chunk's fluids as static (O(1)).
    // Call once per chunk after it has been committed to the world map.
    void registerChunk(const Chunk* chunk, int chunkX, int chunkZ);

    // Remove a chunk from the static registry when it is unloaded (O(1)).
    // Prevents the activated-block set from leaking memory across session.
    void unregisterChunk(int chunkX, int chunkZ);

    // Call when the player removes a solid block at (bx, by, bz).
    // Every adjacent fluid that is still static is woken up and enqueued.
    void onBlockRemoved(World& world, int bx, int by, int bz);

    // Directly activate a single fluid block (used by internal helpers).
    void activateFluid(int x, int y, int z, bool isLava);

    // Register a water/lava block placed by the player — starts flowing immediately.
    void onFluidPlaced(int x, int y, int z, bool isLava);

    // Returns true if (x,y,z) is a generation-placed static fluid.
    bool isStatic(int x, int y, int z) const noexcept;

    // Queue access for the main loop's tryFlow logic.
    std::deque<int64_t>&              waterQueue() { return m_waterQ; }
    std::deque<int64_t>&              lavaQueue()  { return m_lavaQ;  }
    std::unordered_map<int64_t, int>& distances()  { return m_dist;   }

private:
    std::deque<int64_t>              m_waterQ;        // pending water flow ticks
    std::deque<int64_t>              m_lavaQ;         // pending lava flow ticks
    std::unordered_map<int64_t, int> m_dist;          // fluid key → spread distance
    std::unordered_set<uint64_t>     m_staticChunks;  // chunks whose fluids are at rest
    std::unordered_set<int64_t>      m_activated;     // per-block exceptions (woken up)
};
