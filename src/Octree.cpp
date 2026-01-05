#include "Octree.hpp"

Octree::Octree(const AABB& bounds, int maxDepth) 
    : m_root(std::make_unique<Node>(bounds)), m_maxDepth(maxDepth) {
}

Octree::~Octree() {}

void Octree::clear() {
    m_root->chunks.clear();
    for(int i=0; i<8; ++i) m_root->children[i].reset();
    m_root->isLeaf = true;
}

void Octree::insert(ChunkData* chunk, const AABB& chunkBounds) {
    insert(m_root.get(), chunk, chunkBounds, 0);
}

void Octree::insert(Node* node, ChunkData* chunk, const AABB& chunkBounds, int depth) {
    if (!node->bounds.intersects(chunkBounds)) return;

    if (depth >= m_maxDepth) {
        node->chunks.push_back({chunk, chunkBounds});
        return;
    }

    if (node->isLeaf) {
        // Split if too many items? For now, just split if not max depth
        // Actually, let's just split lazily
        if (node->chunks.size() > 8) { // Threshold
            node->isLeaf = false;
            Vec3 center = node->bounds.getCenter();
            Vec3 min = node->bounds.min;
            Vec3 max = node->bounds.max;

            for (int i = 0; i < 8; ++i) {
                AABB childBounds;
                childBounds.min.x = (i & 1) ? center.x : min.x;
                childBounds.max.x = (i & 1) ? max.x : center.x;
                childBounds.min.y = (i & 2) ? center.y : min.y;
                childBounds.max.y = (i & 2) ? max.y : center.y;
                childBounds.min.z = (i & 4) ? center.z : min.z;
                childBounds.max.z = (i & 4) ? max.z : center.z;
                node->children[i] = std::make_unique<Node>(childBounds);
            }

            // Re-distribute existing chunks
            for (auto& pair : node->chunks) {
                for (int i = 0; i < 8; ++i) {
                    insert(node->children[i].get(), pair.first, pair.second, depth + 1);
                }
            }
            node->chunks.clear();
        } else {
            node->chunks.push_back({chunk, chunkBounds});
            return;
        }
    }

    if (!node->isLeaf) {
        for (int i = 0; i < 8; ++i) {
            insert(node->children[i].get(), chunk, chunkBounds, depth + 1);
        }
    }
}

void Octree::query(const Frustum& frustum, std::vector<ChunkData*>& results) {
    query(m_root.get(), frustum, results);
}

void Octree::query(Node* node, const Frustum& frustum, std::vector<ChunkData*>& results) {
    if (!frustum.testAABB(node->bounds)) return;

    for (auto& pair : node->chunks) {
        if (frustum.testAABB(pair.second)) {
            results.push_back(pair.first);
        }
    }

    if (!node->isLeaf) {
        for (int i = 0; i < 8; ++i) {
            query(node->children[i].get(), frustum, results);
        }
    }
}
