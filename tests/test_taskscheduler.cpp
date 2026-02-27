#include <catch2/catch_test_macros.hpp>
#include "TaskScheduler.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <set>
#include <thread>
#include <stdexcept>

// =============================================================================
//  FUNCTIONAL CORRECTNESS
// =============================================================================

TEST_CASE("TaskScheduler: Single task executes exactly once", "[scheduler][basic]") {
    TaskScheduler scheduler(2);
    std::atomic<int> counter{0};
    scheduler.enqueue([&counter]() { counter++; });
    scheduler.wait();
    // Must be exactly 1 — not 0 (missed), not 2 (double-dispatched)
    REQUIRE(counter.load() == 1);
}

TEST_CASE("TaskScheduler: wait() on empty scheduler returns immediately", "[scheduler][basic]") {
    TaskScheduler scheduler(4);
    // Must not hang
    auto t0 = std::chrono::steady_clock::now();
    scheduler.wait();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    INFO("wait() on empty queue took " << elapsed << " ms");
    REQUIRE(elapsed < 200);
}

TEST_CASE("TaskScheduler: All N tasks complete via atomic counter", "[scheduler][basic]") {
    SECTION("10 tasks, 2 threads") {
        TaskScheduler sched(2);
        std::atomic<int> c{0};
        for (int i = 0; i < 10; ++i) sched.enqueue([&c]{ c++; });
        sched.wait();
        REQUIRE(c.load() == 10);
    }
    SECTION("1000 tasks, 4 threads") {
        TaskScheduler sched(4);
        std::atomic<int> c{0};
        for (int i = 0; i < 1000; ++i) sched.enqueue([&c]{ c++; });
        sched.wait();
        REQUIRE(c.load() == 1000);
    }
    SECTION("1 task, 1 thread") {
        TaskScheduler sched(1);
        std::atomic<int> c{0};
        sched.enqueue([&c]{ c++; });
        sched.wait();
        REQUIRE(c.load() == 1);
    }
}

// =============================================================================
//  ARITHMETIC CORRECTNESS (sum must be deterministic)
// =============================================================================

TEST_CASE("TaskScheduler: Atomic accumulate — sum 1..100 = 5050", "[scheduler][arithmetic]") {
    TaskScheduler scheduler(4);
    std::atomic<long long> sum{0};
    for (int i = 1; i <= 100; ++i)
        scheduler.enqueue([&sum, i]{ sum.fetch_add(i, std::memory_order_relaxed); });
    scheduler.wait();
    REQUIRE(sum.load() == 5050LL);
}

TEST_CASE("TaskScheduler: Atomic accumulate — sum 1..1000 = 500500", "[scheduler][arithmetic]") {
    TaskScheduler scheduler(8);
    std::atomic<long long> sum{0};
    for (int i = 1; i <= 1000; ++i)
        scheduler.enqueue([&sum, i]{ sum.fetch_add(i, std::memory_order_relaxed); });
    scheduler.wait();
    REQUIRE(sum.load() == 500500LL);
}

// =============================================================================
//  CONCURRENT DATA STRUCTURE SAFETY
// =============================================================================

TEST_CASE("TaskScheduler: Mutex-protected write uniqueness", "[scheduler][concurrency]") {
    // Each task appends its index to a shared vector under a mutex.
    // After wait(), the vector must contain every index exactly once.
    TaskScheduler scheduler(8);
    std::vector<int> results;
    std::mutex mtx;
    const int N = 500;
    for (int i = 0; i < N; ++i)
        scheduler.enqueue([&results, &mtx, i]{
            std::lock_guard<std::mutex> lk(mtx);
            results.push_back(i);
        });
    scheduler.wait();
    REQUIRE(results.size() == (size_t)N);
    // All unique values present
    std::set<int> unique(results.begin(), results.end());
    REQUIRE(unique.size() == (size_t)N);
}

TEST_CASE("TaskScheduler: Atomic bitwise flags — all 32 bits set", "[scheduler][concurrency]") {
    // 32 tasks each set one distinct bit in a shared flags word.
    TaskScheduler scheduler(4);
    std::atomic<uint32_t> flags{0};
    for (int i = 0; i < 32; ++i)
        scheduler.enqueue([&flags, i]{
            flags.fetch_or(1u << i, std::memory_order_relaxed);
        });
    scheduler.wait();
    REQUIRE(flags.load() == 0xFFFFFFFFu);
}

// =============================================================================
//  SLEEP-TOLERANT / LATENCY
// =============================================================================

TEST_CASE("TaskScheduler: Tasks with sleep complete correctly", "[scheduler][latency]") {
    TaskScheduler scheduler(4);
    std::atomic<int> counter{0};
    const int N = 8;
    for (int i = 0; i < N; ++i)
        scheduler.enqueue([&counter]{
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    scheduler.wait();
    REQUIRE(counter.load() == N);
}

TEST_CASE("TaskScheduler: Wall-clock speedup — parallel faster than serial", "[scheduler][latency]") {
    // 8 tasks each sleep 20ms.  4 threads should finish in ~40ms, not 160ms.
    const int N = 8;
    const int sleepMs = 20;

    TaskScheduler scheduler(4);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i)
        scheduler.enqueue([sleepMs]{
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        });
    scheduler.wait();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    long long serialMs = (long long)N * sleepMs;
    INFO("Parallel elapsed: " << elapsed << "ms,  serial would be: " << serialMs << "ms");
    // Parallel must be meaningfully faster (< 75% of serial time)
    REQUIRE(elapsed < serialMs * 75 / 100);
}

// =============================================================================
//  RE-USE AFTER WAIT
// =============================================================================

TEST_CASE("TaskScheduler: Re-use after wait() works multiple times", "[scheduler][reuse]") {
    TaskScheduler scheduler(4);
    for (int round = 0; round < 5; ++round) {
        std::atomic<int> c{0};
        const int N = 200;
        for (int i = 0; i < N; ++i)
            scheduler.enqueue([&c]{ c.fetch_add(1, std::memory_order_relaxed); });
        scheduler.wait();
        INFO("Round " << round << " counter: " << c.load());
        REQUIRE(c.load() == N);
    }
}

// =============================================================================
//  SELF-SCHEDULING (tasks that enqueue more tasks)
// =============================================================================

TEST_CASE("TaskScheduler: Tasks that enqueue child tasks", "[scheduler][advanced]") {
    TaskScheduler scheduler(4);
    std::atomic<int> counter{0};
    // Enqueue 10 parent tasks; each parent enqueues 10 children.
    const int parents = 10, children = 10;
    for (int p = 0; p < parents; ++p) {
        scheduler.enqueue([&scheduler, &counter, children]{
            counter.fetch_add(1, std::memory_order_relaxed); // parent work
            for (int c = 0; c < children; ++c)
                scheduler.enqueue([&counter]{
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
        });
    }
    scheduler.wait();
    // parents + parents*children
    REQUIRE(counter.load() == parents + parents * children);
}

// =============================================================================
//  EXCEPTION RESILIENCE
// =============================================================================

TEST_CASE("TaskScheduler: Exception in one task does not prevent others", "[scheduler][resilience]") {
    // The scheduler catches exceptions internally (as implemented in TaskScheduler.cpp).
    // All non-throwing tasks must still complete.
    TaskScheduler scheduler(2);
    std::atomic<int> counter{0};
    scheduler.enqueue([]{ throw std::runtime_error("Intentional test exception"); });
    for (int i = 0; i < 20; ++i)
        scheduler.enqueue([&counter]{ counter.fetch_add(1, std::memory_order_relaxed); });
    scheduler.enqueue([]{ throw std::logic_error("Second intentional exception"); });
    scheduler.wait();
    REQUIRE(counter.load() == 20);
}

// =============================================================================
//  HIGH-CONTENTION STRESS TEST
// =============================================================================

TEST_CASE("TaskScheduler: 50 000 tasks on max hardware threads", "[scheduler][stress]") {
    const size_t threads = std::max(2u, std::thread::hardware_concurrency());
    TaskScheduler scheduler(threads);
    std::atomic<int> counter{0};
    const int N = 50000;
    for (int i = 0; i < N; ++i)
        scheduler.enqueue([&counter]{ counter.fetch_add(1, std::memory_order_relaxed); });
    scheduler.wait();
    REQUIRE(counter.load() == N);
}

TEST_CASE("TaskScheduler: Stress — mixed fast and slow tasks", "[scheduler][stress]") {
    TaskScheduler scheduler(4);
    std::atomic<int> fastCount{0}, slowCount{0};
    const int fast = 500, slow = 10;
    for (int i = 0; i < fast; ++i)
        scheduler.enqueue([&fastCount]{ fastCount.fetch_add(1, std::memory_order_relaxed); });
    for (int i = 0; i < slow; ++i)
        scheduler.enqueue([&slowCount]{
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            slowCount.fetch_add(1, std::memory_order_relaxed);
        });
    scheduler.wait();
    REQUIRE(fastCount.load() == fast);
    REQUIRE(slowCount.load() == slow);
}
