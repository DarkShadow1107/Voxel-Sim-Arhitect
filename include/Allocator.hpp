#pragma once
#include <cstddef>

class ArenaAllocator {
public:
    ArenaAllocator(size_t size);
    ~ArenaAllocator();

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    void reset();

    size_t getOffset() const { return m_offset; }
    size_t getSize() const { return m_size; }

private:
    void* m_buffer;
    size_t m_size;
    size_t m_offset;
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
