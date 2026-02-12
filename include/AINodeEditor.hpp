#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Math.hpp"

// ---- AI Graph Data Structures ----

enum class AINodeType { STATE, CONDITION, ACTION };

struct AINode {
    int id = 0;
    AINodeType type = AINodeType::STATE;
    std::string name;
    float posX = 0.0f, posY = 0.0f;  // Canvas position

    // State params
    float duration = 0.0f;        // How long to stay (0 = indefinite)
    float moveSpeed = 1.0f;       // Speed multiplier in this state

    // Action params
    std::string actionType;       // "wander", "follow_player", "flee_player", "attack",
                                  // "play_sound", "jump", "swim", "fly"
    float actionParam = 0.0f;     // Generic parameter (range, damage, etc.)
};

struct AIConnection {
    int id = 0;
    int fromNodeId = 0;
    int toNodeId = 0;

    // Transition condition
    std::string conditionType;    // "distance_to_player", "hp_below", "hp_above",
                                  // "is_day", "is_night", "in_water", "on_fire",
                                  // "timer_expired", "random_chance", "was_attacked"
    float conditionValue = 0.0f;
    std::string comparison = "<"; // "<", ">", "<=", ">=", "=="
};

struct AIGraph {
    std::vector<AINode> nodes;
    std::vector<AIConnection> connections;
    int startNodeId = 0;
    int nextNodeId = 1;
    int nextConnectionId = 1;

    AINode* findNode(int id) {
        for (auto& n : nodes) if (n.id == id) return &n;
        return nullptr;
    }
    const AINode* findNode(int id) const {
        for (const auto& n : nodes) if (n.id == id) return &n;
        return nullptr;
    }
};

// ---- Visual Node Editor ----

class AINodeEditor {
public:
    void show(AIGraph& graph);

private:
    // Canvas state
    float m_scrollX = 0.0f, m_scrollY = 0.0f;
    float m_zoom = 1.0f;
    int m_selectedNode = -1;
    int m_selectedConnection = -1;
    int m_draggingNode = -1;
    int m_connectingFromNode = -1;
    bool m_canvasDragging = false;
    bool m_showContextMenu = false;
    float m_contextMenuX = 0.0f, m_contextMenuY = 0.0f;

    static constexpr float NODE_WIDTH = 160.0f;
    static constexpr float NODE_HEIGHT = 60.0f;
    static constexpr float PIN_RADIUS = 6.0f;

    void showNodeProperties(AIGraph& graph);
    void showConnectionProperties(AIGraph& graph);
};
