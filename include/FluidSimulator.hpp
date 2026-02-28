#pragma once
// ---------------------------------------------------------------------------
//  FluidSimulator — Event-driven Block-Update fluid physics.
//
//  TDD §1 specification:
//    • All generation-placed fluids start in STATE_DORMANT — zero CPU cost.
//    • A player block-break / block-place fires onBlockChanged() which pushes
//      a BlockUpdateEvent for each face-adjacent cell.
//    • processTick() drains the event queue: DORMANT fluids adjacent to the
//      changed block transition to STATE_ACTIVE and are enqueued for flow.
//    • Once an ACTIVE fluid has spread and settled it stays ACTIVE (outside
//      static chunks) — dormancy is a generation-only optimisation.
//
//  Integration:
//    • Call registerChunk()  after  a chunk is committed (replaces WaterPhysics)
//    • Call unregisterChunk() when   a chunk is evicted
//    • Call onBlockChanged()  whenever the player breaks / places any block
//    • Call onFluidPlaced()   when   the player places water or lava directly
//    • Call processTick()     once per frame to drain the event queue
//    • Replace isStatic()     checks with isDormant()
// ---------------------------------------------------------------------------
#include <queue>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

class Chunk;
class World;

// State of a fluid block in the simulation.
enum class FluidState : uint8_t {
    DORMANT = 0,  // generation-placed; quiescent; no simulation tick
    ACTIVE  = 1,  // disturbed or player-placed; actively flowing
};

// An event pushed whenever a block changes (break, place, or explosion).
// Every face-adjacent cell is checked for DORMANT fluids when this is drained.
struct BlockUpdateEvent {
    int x, y, z;
};

class FluidSimulator {
public:
    // ── Coordinate helpers (same encoding as legacy WaterPhysics) ──────────
    static int64_t  encodeKey(int x, int y, int z) noexcept;
    static void     decodeKey(int64_t k, int& x, int& y, int& z) noexcept;
    static uint64_t chunkKey(int cx, int cz) noexcept;

    // ── Chunk lifecycle ─────────────────────────────────────────────────────
    // Mark all fluids in a freshly-generated chunk as DORMANT (O(1) — no
    // per-block scan needed; the chunk key is sufficient).
    void registerChunk(const Chunk* chunk, int chunkX, int chunkZ);

    // Remove the chunk from the dormant registry on eviction.
    // Orphaned per-block activation entries are benign — they are never
    // queried after the chunk key is gone.
    void unregisterChunk(int chunkX, int chunkZ);

    // ── Block-Update events ─────────────────────────────────────────────────
    // Push a change at (bx, by, bz). All 6 face-adjacent cells receive a
    // BlockUpdateEvent that will be processed by the next processTick() call.
    // Call for every block break AND every block placement.
    void onBlockChanged(int bx, int by, int bz);

    // Register a player-placed water/lava block — it is immediately ACTIVE
    // and enqueued for flow without waiting for an event.
    void onFluidPlaced(int x, int y, int z, bool isLava);

    // ── State query ─────────────────────────────────────────────────────────
    // Returns true if (x,y,z) is a DORMANT (generation-placed, undisturbed)
    // fluid block.  Use instead of the old isStatic() check.
    bool isDormant(int x, int y, int z) const noexcept;

    // ── Tick processing ─────────────────────────────────────────────────────
    // Drain up to `limit` BlockUpdateEvents, transitioning adjacent DORMANT
    // fluids to ACTIVE. Returns the number of events processed this tick.
    // Call once per frame (before the main flow-loop).
    int processTick(World& world, int limit = 64);

    // ── Queue access (consumed by main.cpp tryFlow logic) ───────────────────
    std::deque<int64_t>&              waterQueue() { return m_waterQ; }
    std::deque<int64_t>&              lavaQueue()  { return m_lavaQ;  }
    std::unordered_map<int64_t, int>& distances()  { return m_dist;   }

    // Clear all state (call on world clear / new game).
    void clear();

private:
    int64_t _chunkLocalKey(int x, int y, int z, int& cx, int& cz) const noexcept;

    std::queue<BlockUpdateEvent>      m_events;       // pending BUD event queue
    std::deque<int64_t>              m_waterQ;        // water blocks queued to flow
    std::deque<int64_t>              m_lavaQ;         // lava blocks queued to flow
    std::unordered_map<int64_t, int> m_dist;          // key → spread distance
    std::unordered_set<uint64_t>     m_dormantChunks; // chunks whose fluids are at rest
    std::unordered_set<int64_t>      m_activated;     // per-block exceptions (woken)
};
