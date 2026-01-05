#include "Allocator.hpp"
#include <cstdlib>
#include <iostream>

ArenaAllocator::ArenaAllocator(size_t size) : m_size(size), m_offset(0) {
    m_buffer = std::malloc(size);
    if (!m_buffer) {
        std::cerr << "Failed to allocate arena of size " << size << std::endl;
    }
}

ArenaAllocator::~ArenaAllocator() {
    std::free(m_buffer);
}

void* ArenaAllocator::allocate(size_t size, size_t alignment) {
    if (!m_buffer) return nullptr;

    size_t current_ptr = reinterpret_cast<size_t>(m_buffer) + m_offset;
    size_t padding = (alignment - (current_ptr % alignment)) % alignment;
    
    if (m_offset + padding + size > m_size) {
        return nullptr;
    }
    
    m_offset += padding;
    void* ptr = static_cast<char*>(m_buffer) + m_offset;
    m_offset += size;
    return ptr;
}

void ArenaAllocator::reset() {
    m_offset = 0;
}

// Pool Allocator Implementation
PoolAllocator::PoolAllocator(size_t objectSize, size_t objectCount) 
    : m_objectSize(std::max(objectSize, sizeof(Node))), m_totalSize(m_objectSize * objectCount), m_totalCount(objectCount), m_usedCount(0) {
    m_buffer = std::malloc(m_totalSize);
    m_freeList = static_cast<Node*>(m_buffer);
    
    Node* current = m_freeList;
    for (size_t i = 0; i < objectCount - 1; ++i) {
        current->next = reinterpret_cast<Node*>(reinterpret_cast<char*>(current) + m_objectSize);
        current = current->next;
    }
    current->next = nullptr;
}

PoolAllocator::~PoolAllocator() {
    std::free(m_buffer);
}

void* PoolAllocator::allocate() {
    if (!m_buffer || !m_freeList) return nullptr;
    
    Node* node = m_freeList;
    m_freeList = m_freeList->next;
    m_usedCount++;
    return node;
}

void PoolAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    
    // Safety check: Ensure pointer belongs to this pool
    if (!owns(ptr)) {
        std::cerr << "PoolAllocator: Attempted to deallocate pointer " << ptr << " which does not belong to pool!" << std::endl;
        return;
    }

    Node* node = static_cast<Node*>(ptr);
    node->next = m_freeList;
    m_freeList = node;
    m_usedCount--;
}

bool PoolAllocator::owns(void* ptr) const {
    if (!m_buffer) return false;
    size_t start = reinterpret_cast<size_t>(m_buffer);
    size_t end = start + m_totalSize;
    size_t p = reinterpret_cast<size_t>(ptr);
    return p >= start && p < end;
}
