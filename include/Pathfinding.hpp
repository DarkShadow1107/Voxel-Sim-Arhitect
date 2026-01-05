#pragma once
#include <vector>
#include "Octree.hpp"

struct Node {
    Vector3 position;
    float gCost;
    float hCost;
    Node* parent;

    float fCost() const { return gCost + hCost; }
};

class Pathfinding {
public:
    static std::vector<Vector3> findPath(Vector3 start, Vector3 end, OctreeNode& world);
};
