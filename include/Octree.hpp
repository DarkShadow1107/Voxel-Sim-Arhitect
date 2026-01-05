#pragma once
#include "Math.hpp"
#include <vector>
#include <memory>

// Forward declaration
struct ChunkData;

class Octree {
public:
    Octree(const AABB& bounds, int maxDepth = 6);
    ~Octree();

    void insert(ChunkData* chunk, const AABB& chunkBounds);
    void query(const Frustum& frustum, std::vector<ChunkData*>& results);
    void clear();

private:
    struct Node {
        AABB bounds;
        std::vector<std::pair<ChunkData*, AABB>> chunks; // Store bounds with chunk
        std::unique_ptr<Node> children[8];
        bool isLeaf = true;

        Node(const AABB& b) : bounds(b) {}
    };

    std::unique_ptr<Node> m_root;
    int m_maxDepth;

    void insert(Node* node, ChunkData* chunk, const AABB& chunkBounds, int depth);
    void query(Node* node, const Frustum& frustum, std::vector<ChunkData*>& results);
};
