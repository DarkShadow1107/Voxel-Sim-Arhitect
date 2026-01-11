#pragma once
#include <cstddef>
#include <new>
#include <utility>
#include <algorithm>

class ArenaAllocator {
public:
    ArenaAllocator(size_t size);
    ~ArenaAllocator();

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    void reset();

    // Added: Marker-based reset for nested scopes
    using Marker = size_t;
    Marker getMarker() const { return m_offset; }
    void resetToMarker(Marker marker) { 
        m_offset = marker; 
        // We don't easily track m_allocatedCount for markers without a stack, 
        // but for a simple arena this is usually fine.
    }

    size_t getOffset() const { return m_offset; }
    size_t getSize() const { return m_size; }
    size_t getAllocatedCount() const { return m_allocatedCount; }

private:
    void* m_buffer;
    size_t m_size;
    size_t m_offset;
    size_t m_allocatedCount;
};

class PoolAllocator {
public:
    PoolAllocator(size_t objectSize, size_t objectCount);
    ~PoolAllocator();

    void* allocate();
    void deallocate(void* ptr);

    size_t getUsedCount() const { return m_usedCount; }
    size_t getTotalCount() const { return m_totalCount; }

private:
    struct Node {
        Node* next;
    };

    void* m_buffer;
    Node* m_freeList;
    size_t m_objectSize;
    size_t m_totalSize;
    size_t m_totalCount;
    size_t m_usedCount;
};

template<typename T>
class Pool {
public:
    Pool(size_t count) : m_pool(sizeof(T), count) {}
    
    template<typename... Args>
    T* construct(Args&&... args) {
        void* ptr = m_pool.allocate();
        if (!ptr) return nullptr;
        return new(ptr) T(std::forward<Args>(args)...);
    }
    
    void destroy(T* ptr) {
        if (!ptr) return;
        ptr->~T();
        m_pool.deallocate(ptr);
    }

private:
    PoolAllocator m_pool;
};
