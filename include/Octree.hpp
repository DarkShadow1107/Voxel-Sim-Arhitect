#pragma once
#include <array>
#include <memory>

struct Voxel {
    unsigned char type;
};

struct Vector3 {
    float x, y, z;
};

struct AABB {
    Vector3 min;
    Vector3 max;

    bool contains(const Vector3& point) const {
        return (point.x >= min.x && point.x <= max.x) &&
               (point.y >= min.y && point.y <= max.y) &&
               (point.z >= min.z && point.z <= max.z);
    }
};

class OctreeNode {
public:
    OctreeNode(AABB bounds, int depth = 0);
    ~OctreeNode();

    void insert(Vector3 position, Voxel voxel);
    Voxel* query(Vector3 position);

private:
    void subdivide();
    int getChildIndex(Vector3 position);

    AABB m_bounds;
    int m_depth;
    bool m_isLeaf;
    Voxel m_voxel;
    std::array<std::unique_ptr<OctreeNode>, 8> m_children;

    static constexpr int MAX_DEPTH = 8;
};
