#include "Octree.hpp"

OctreeNode::OctreeNode(AABB bounds, int depth) 
    : m_bounds(bounds), m_depth(depth), m_isLeaf(true), m_voxel{0} {
}

OctreeNode::~OctreeNode() {
}

void OctreeNode::insert(Vector3 position, Voxel voxel) {
    if (!m_bounds.contains(position)) return;

    if (m_depth >= MAX_DEPTH) {
        m_voxel = voxel;
        return;
    }

    if (m_isLeaf) {
        subdivide();
    }

    int index = getChildIndex(position);
    m_children[index]->insert(position, voxel);
}

Voxel* OctreeNode::query(Vector3 position) {
    if (!m_bounds.contains(position)) return nullptr;

    if (m_isLeaf) {
        return &m_voxel;
    }

    int index = getChildIndex(position);
    return m_children[index]->query(position);
}

void OctreeNode::subdivide() {
    m_isLeaf = false;
    Vector3 center = {
        (m_bounds.min.x + m_bounds.max.x) / 2.0f,
        (m_bounds.min.y + m_bounds.max.y) / 2.0f,
        (m_bounds.min.z + m_bounds.max.z) / 2.0f
    };

    for (int i = 0; i < 8; ++i) {
        AABB childBounds;
        childBounds.min.x = (i & 1) ? center.x : m_bounds.min.x;
        childBounds.max.x = (i & 1) ? m_bounds.max.x : center.x;
        childBounds.min.y = (i & 2) ? center.y : m_bounds.min.y;
        childBounds.max.y = (i & 2) ? m_bounds.max.y : center.y;
        childBounds.min.z = (i & 4) ? center.z : m_bounds.min.z;
        childBounds.max.z = (i & 4) ? m_bounds.max.z : center.z;

        m_children[i] = std::make_unique<OctreeNode>(childBounds, m_depth + 1);
    }
}

int OctreeNode::getChildIndex(Vector3 position) {
    Vector3 center = {
        (m_bounds.min.x + m_bounds.max.x) / 2.0f,
        (m_bounds.min.y + m_bounds.max.y) / 2.0f,
        (m_bounds.min.z + m_bounds.max.z) / 2.0f
    };

    int index = 0;
    if (position.x >= center.x) index |= 1;
    if (position.y >= center.y) index |= 2;
    if (position.z >= center.z) index |= 4;
    return index;
}
