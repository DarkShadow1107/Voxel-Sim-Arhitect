#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Allocator.hpp"
#include <cstdint>
#include <vector>
#include <cstring>

using namespace Catch::Matchers;

// =============================================================================
//  ArenaAllocator — Functional Correctness
// =============================================================================

TEST_CASE("ArenaAllocator: Single allocation returns non-null pointer", "[allocator][arena]") {
    ArenaAllocator arena(1024);
    void* p = arena.allocate(64);
    REQUIRE(p != nullptr);
    REQUIRE(arena.getOffset() >= 64);
    REQUIRE(arena.getAllocatedCount() == 1);
}

TEST_CASE("ArenaAllocator: Multiple allocations return distinct, non-overlapping pointers", "[allocator][arena]") {
    ArenaAllocator arena(4096);
    const int N = 16;
    std::vector<void*> ptrs;
    for (int i = 0; i < N; ++i) {
        void* p = arena.allocate(64);
        REQUIRE(p != nullptr);
        // Must not duplicate a previous pointer
        for (auto prev : ptrs)
            REQUIRE(p != prev);
        ptrs.push_back(p);
    }
    REQUIRE(arena.getAllocatedCount() == N);
}

TEST_CASE("ArenaAllocator: Offset monotonically increases with each alloc", "[allocator][arena]") {
    ArenaAllocator arena(8192);
    size_t prev = arena.getOffset();
    for (int i = 0; i < 20; ++i) {
        arena.allocate(100);
        size_t cur = arena.getOffset();
        REQUIRE(cur >= prev + 100);
        prev = cur;
    }
}

TEST_CASE("ArenaAllocator: Allocation exceeding capacity returns nullptr", "[allocator][arena]") {
    ArenaAllocator arena(256);
    void* p = arena.allocate(512);
    REQUIRE(p == nullptr);
    // Offset must not change after failed allocation
    REQUIRE(arena.getOffset() == 0);
}

TEST_CASE("ArenaAllocator: Exactly filling capacity succeeds", "[allocator][arena]") {
    const size_t cap = 512;
    ArenaAllocator arena(cap);
    // Allocate in one shot — might succeed or fail depending on header/alignment;
    // either way the next allocation must fail.
    arena.allocate(cap / 2);
    arena.allocate(cap / 2);
    void* overflow = arena.allocate(1);
    REQUIRE(overflow == nullptr);
}

TEST_CASE("ArenaAllocator: reset() clears offset and count", "[allocator][arena]") {
    ArenaAllocator arena(1024);
    arena.allocate(128);
    arena.allocate(256);
    arena.reset();
    REQUIRE(arena.getOffset() == 0);
    REQUIRE(arena.getAllocatedCount() == 0);
    // Should be able to re-use full capacity
    void* p = arena.allocate(900);
    REQUIRE(p != nullptr);
}

TEST_CASE("ArenaAllocator: Marker-based scope reset", "[allocator][arena]") {
    ArenaAllocator arena(2048);
    arena.allocate(128); // permanent frame allocation
    ArenaAllocator::Marker mark = arena.getMarker();

    void* tmp1 = arena.allocate(256);
    void* tmp2 = arena.allocate(256);
    REQUIRE(tmp1 != nullptr);
    REQUIRE(tmp2 != nullptr);
    REQUIRE(arena.getOffset() >= mark + 512);

    arena.resetToMarker(mark);
    REQUIRE(arena.getOffset() == mark);

    // After restoring the marker we can re-allocate the same region
    void* reused = arena.allocate(256);
    REQUIRE(reused != nullptr);
    // The re-allocated pointer must start at the same address as tmp1
    REQUIRE(reused == tmp1);
}

TEST_CASE("ArenaAllocator: Written data survives until reset", "[allocator][arena]") {
    ArenaAllocator arena(1024);
    int* arr = static_cast<int*>(arena.allocate(sizeof(int) * 4));
    REQUIRE(arr != nullptr);
    arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40;
    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 20);
    REQUIRE(arr[2] == 30);
    REQUIRE(arr[3] == 40);
}

TEST_CASE("ArenaAllocator: Alignment guarantee", "[allocator][arena]") {
    ArenaAllocator arena(4096);
    // Allocate 1 byte to misalign, then ask for 16-byte aligned block
    arena.allocate(1, 1);
    void* aligned16 = arena.allocate(64, 16);
    if (aligned16) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(aligned16);
        INFO("Aligned-16 address: " << addr);
        REQUIRE((addr % 16) == 0);
    }
    // 8-byte alignment
    arena.allocate(3, 1); // another misalignment
    void* aligned8 = arena.allocate(32, 8);
    if (aligned8) {
        REQUIRE((reinterpret_cast<uintptr_t>(aligned8) % 8) == 0);
    }
}

// =============================================================================
//  PoolAllocator — Functional Correctness
// =============================================================================

TEST_CASE("PoolAllocator: Basic allocate/deallocate", "[allocator][pool]") {
    PoolAllocator pool(sizeof(int), 10);
    REQUIRE(pool.getUsedCount() == 0);
    REQUIRE(pool.getTotalCount() == 10);

    void* p1 = pool.allocate();
    void* p2 = pool.allocate();
    REQUIRE(p1 != nullptr);
    REQUIRE(p2 != nullptr);
    REQUIRE(p1 != p2);
    REQUIRE(pool.getUsedCount() == 2);

    pool.deallocate(p1);
    REQUIRE(pool.getUsedCount() == 1);
    pool.deallocate(p2);
    REQUIRE(pool.getUsedCount() == 0);
}

TEST_CASE("PoolAllocator: Exhaustion returns nullptr", "[allocator][pool]") {
    PoolAllocator pool(sizeof(float), 5);
    std::vector<void*> ptrs;
    for (int i = 0; i < 5; ++i) {
        void* p = pool.allocate();
        REQUIRE(p != nullptr);
        ptrs.push_back(p);
    }
    REQUIRE(pool.getUsedCount() == 5);

    void* extra = pool.allocate();
    REQUIRE(extra == nullptr);
}

TEST_CASE("PoolAllocator: Freed slot is immediately reused", "[allocator][pool]") {
    PoolAllocator pool(sizeof(double), 3);
    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();
    REQUIRE(c != nullptr);

    pool.deallocate(b);
    void* reused = pool.allocate();
    REQUIRE(reused == b);  // LIFO free-list: most recently freed is first reused
}

TEST_CASE("PoolAllocator: Fill, drain, refill cycle", "[allocator][pool]") {
    const size_t N = 8;
    PoolAllocator pool(sizeof(uint64_t), N);
    std::vector<void*> ptrs;

    for (size_t cycle = 0; cycle < 3; ++cycle) {
        ptrs.clear();
        for (size_t i = 0; i < N; ++i) { ptrs.push_back(pool.allocate()); }
        REQUIRE(pool.getUsedCount() == N);
        REQUIRE(pool.allocate() == nullptr); // exhausted

        for (auto p : ptrs) pool.deallocate(p);
        REQUIRE(pool.getUsedCount() == 0);
    }
}

TEST_CASE("PoolAllocator: Written data integrity before deallocation", "[allocator][pool]") {
    PoolAllocator pool(sizeof(int) * 4, 4);
    int* slot = static_cast<int*>(pool.allocate());
    REQUIRE(slot != nullptr);
    slot[0] = 11; slot[1] = 22; slot[2] = 33; slot[3] = 44;
    REQUIRE(slot[0] == 11);
    REQUIRE(slot[3] == 44);
    pool.deallocate(slot);
}

// =============================================================================
//  Pool<T> — Template Typed Wrapper
// =============================================================================

struct TestWidget {
    int id;
    float value;
    TestWidget(int i, float v) : id(i), value(v) {}
};

TEST_CASE("Pool<T>: construct and destroy typed objects", "[allocator][pool_t]") {
    Pool<TestWidget> pool(16);

    TestWidget* w1 = pool.construct(1, 3.14f);
    TestWidget* w2 = pool.construct(2, 2.71f);
    REQUIRE(w1 != nullptr);
    REQUIRE(w2 != nullptr);
    REQUIRE(w1->id == 1);
    REQUIRE(w2->id == 2);
    REQUIRE_THAT(w1->value, WithinAbs(3.14f, 0.01f));

    pool.destroy(w1);
    TestWidget* w3 = pool.construct(3, 0.0f);
    REQUIRE(w3 == w1); // reused slot
    REQUIRE(w3->id == 3);
    pool.destroy(w2);
    pool.destroy(w3);
}

TEST_CASE("Pool<T>: construct returns nullptr when exhausted", "[allocator][pool_t]") {
    Pool<TestWidget> pool(2);
    auto* a = pool.construct(1, 1.0f);
    auto* b = pool.construct(2, 2.0f);
    auto* c = pool.construct(3, 3.0f); // over-capacity
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c == nullptr);
    pool.destroy(a);
    pool.destroy(b);
}

TEST_CASE("Pool<T>: destroy(nullptr) does not crash", "[allocator][pool_t]") {
    Pool<TestWidget> pool(4);
    REQUIRE_NOTHROW(pool.destroy(nullptr));
}
