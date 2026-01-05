#include "Allocator.hpp"
#include <cstdlib>
#include <iostream>
#include <cassert>

ArenaAllocator::ArenaAllocator(size_t size) : m_size(size), m_offset(0), m_allocatedCount(0) {
    m_buffer = std::malloc(size);
    if (!m_buffer) {
        std::cerr << "CRITICAL ERROR: Failed to allocate arena of size " << size << ". System out of memory." << std::endl;
        std::abort();
    }
}

ArenaAllocator::~ArenaAllocator() {
    if (m_buffer) {
        std::free(m_buffer);
        m_buffer = nullptr;
    }
}

void* ArenaAllocator::allocate(size_t size, size_t alignment) {
    if (size == 0) return nullptr;

    size_t current_ptr = reinterpret_cast<size_t>(m_buffer) + m_offset;
    size_t padding = (alignment - (current_ptr % alignment)) % alignment;
    
    if (m_offset + padding + size > m_size) {
        std::cerr << "ARENA ALLOCATION FAILED: Requested " << size << " bytes, but only " 
                  << (m_size - m_offset) << " bytes remain in arena." << std::endl;
        return nullptr;
    }
    
    m_offset += padding;
    void* ptr = static_cast<char*>(m_buffer) + m_offset;
    m_offset += size;
    m_allocatedCount++;
    return ptr;
}

void ArenaAllocator::reset() {
    m_offset = 0;
    m_allocatedCount = 0;
}

// Pool Allocator Implementation
PoolAllocator::PoolAllocator(size_t objectSize, size_t objectCount) 
    : m_objectSize(std::max(objectSize, sizeof(Node))), m_totalSize(m_objectSize * objectCount), m_totalCount(objectCount), m_usedCount(0) {
    m_buffer = std::malloc(m_totalSize);
    if (!m_buffer) {
        std::cerr << "CRITICAL ERROR: Failed to allocate pool of size " << m_totalSize << ". System out of memory." << std::endl;
        std::abort();
    }
    m_freeList = static_cast<Node*>(m_buffer);
    
    Node* current = m_freeList;
    for (size_t i = 0; i < objectCount - 1; ++i) {
        current->next = reinterpret_cast<Node*>(reinterpret_cast<char*>(current) + m_objectSize);
        current = current->next;
    }
    current->next = nullptr;
}

PoolAllocator::~PoolAllocator() {
    if (m_usedCount > 0) {
        std::cerr << "WARNING: PoolAllocator destroyed with " << m_usedCount << " active allocations (Memory leak!)" << std::endl;
    }
    if (m_buffer) {
        std::free(m_buffer);
        m_buffer = nullptr;
    }
}

void* PoolAllocator::allocate() {
    if (!m_freeList) {
        std::cerr << "POOL ALLOCATION FAILED: No free blocks remaining in pool." << std::endl;
        return nullptr;
    }
    
    Node* node = m_freeList;
    m_freeList = m_freeList->next;
    m_usedCount++;
    return node;
}

void PoolAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    
    // Simple safety check: is ptr within our buffer?
    if (ptr < m_buffer || ptr >= static_cast<char*>(m_buffer) + m_totalSize) {
        std::cerr << "CRITICAL ERROR: Attempted to deallocate pointer outside of PoolAllocator bounds!" << std::endl;
        assert(false);
        return;
    }

    Node* node = static_cast<Node*>(ptr);
    node->next = m_freeList;
    m_freeList = node;
    m_usedCount--;
}
