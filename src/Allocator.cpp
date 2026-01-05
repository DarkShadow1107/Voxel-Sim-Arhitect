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
    if (!m_freeList) return nullptr;
    
    Node* node = m_freeList;
    m_freeList = m_freeList->next;
    m_usedCount++;
    return node;
}

void PoolAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    Node* node = static_cast<Node*>(ptr);
    node->next = m_freeList;
    m_freeList = node;
    m_usedCount--;
}
