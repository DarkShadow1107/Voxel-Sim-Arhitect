#include "AINodeEditor.hpp"
#include "imgui.h"
#include <cmath>
#include <algorithm>

// Helper: get node screen rect given canvas offset and zoom
static void getNodeRect(const AINode& node, float offsetX, float offsetY, float zoom,
                        float& x1, float& y1, float& x2, float& y2, float nodeW, float nodeH) {
    x1 = node.posX * zoom + offsetX;
    y1 = node.posY * zoom + offsetY;
    x2 = x1 + nodeW * zoom;
    y2 = y1 + nodeH * zoom;
}

// Helper: get output pin position (right side, vertically centered)
static ImVec2 getOutputPin(const AINode& node, float offsetX, float offsetY, float zoom, float nodeW, float nodeH) {
    float x = (node.posX + nodeW) * zoom + offsetX;
    float y = (node.posY + nodeH * 0.5f) * zoom + offsetY;
    return {x, y};
}

// Helper: get input pin position (left side, vertically centered)
static ImVec2 getInputPin(const AINode& node, float offsetX, float offsetY, float zoom, float nodeW, float nodeH) {
    float x = node.posX * zoom + offsetX;
    float y = (node.posY + nodeH * 0.5f) * zoom + offsetY;
    return {x, y};
}

// Color for node type
static ImU32 getNodeColor(AINodeType type) {
    switch (type) {
        case AINodeType::STATE:     return IM_COL32(60, 100, 180, 255);
        case AINodeType::CONDITION: return IM_COL32(50, 160, 80, 255);
        case AINodeType::ACTION:    return IM_COL32(200, 120, 50, 255);
    }
    return IM_COL32(128, 128, 128, 255);
}

static ImU32 getNodeColorHovered(AINodeType type) {
    switch (type) {
        case AINodeType::STATE:     return IM_COL32(80, 130, 220, 255);
        case AINodeType::CONDITION: return IM_COL32(70, 200, 100, 255);
        case AINodeType::ACTION:    return IM_COL32(240, 150, 70, 255);
    }
    return IM_COL32(160, 160, 160, 255);
}

void AINodeEditor::show(AIGraph& graph) {
    // Split: canvas on left/top, properties on right/bottom
    float propsHeight = 200.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float canvasH = avail.y - propsHeight - 8;
    if (canvasH < 200) canvasH = 200;

    // ===== CANVAS =====
    ImGui::BeginChild("##AICanvas", ImVec2(0, canvasH), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Clip to canvas
        dl->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

        // Background
        dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                          IM_COL32(30, 30, 35, 255));

        float offsetX = canvasPos.x + m_scrollX;
        float offsetY = canvasPos.y + m_scrollY;

        // Grid
        float gridStep = 40.0f * m_zoom;
        if (gridStep > 5.0f) {
            ImU32 gridCol = IM_COL32(50, 50, 55, 255);
            for (float x = fmodf(m_scrollX, gridStep); x < canvasSize.x; x += gridStep)
                dl->AddLine(ImVec2(canvasPos.x + x, canvasPos.y),
                           ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y), gridCol);
            for (float y = fmodf(m_scrollY, gridStep); y < canvasSize.y; y += gridStep)
                dl->AddLine(ImVec2(canvasPos.x, canvasPos.y + y),
                           ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y), gridCol);
        }

        // --- Draw connections ---
        for (const auto& conn : graph.connections) {
            const AINode* from = graph.findNode(conn.fromNodeId);
            const AINode* to = graph.findNode(conn.toNodeId);
            if (!from || !to) continue;

            ImVec2 p1 = getOutputPin(*from, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);
            ImVec2 p2 = getInputPin(*to, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);

            float dx = std::abs(p2.x - p1.x) * 0.5f;
            if (dx < 40.0f) dx = 40.0f;
            ImVec2 cp1 = {p1.x + dx, p1.y};
            ImVec2 cp2 = {p2.x - dx, p2.y};

            ImU32 connColor = (conn.id == m_selectedConnection)
                ? IM_COL32(255, 255, 100, 255) : IM_COL32(200, 200, 200, 180);
            dl->AddBezierCubic(p1, cp1, cp2, p2, connColor, 2.5f * m_zoom);

            // Arrow head
            ImVec2 dir = {p2.x - cp2.x, p2.y - cp2.y};
            float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (len > 0) { dir.x /= len; dir.y /= len; }
            float arrowSize = 8.0f * m_zoom;
            ImVec2 a1 = {p2.x - dir.x * arrowSize - dir.y * arrowSize * 0.5f,
                         p2.y - dir.y * arrowSize + dir.x * arrowSize * 0.5f};
            ImVec2 a2 = {p2.x - dir.x * arrowSize + dir.y * arrowSize * 0.5f,
                         p2.y - dir.y * arrowSize - dir.x * arrowSize * 0.5f};
            dl->AddTriangleFilled(p2, a1, a2, connColor);

            // Condition label on the connection
            if (!conn.conditionType.empty()) {
                ImVec2 mid = {(p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f - 10.0f * m_zoom};
                char label[64];
                snprintf(label, 64, "%s %s %.0f", conn.conditionType.c_str(), conn.comparison.c_str(), conn.conditionValue);
                dl->AddText(mid, IM_COL32(220, 220, 180, 220), label);
            }
        }

        // --- Draw in-progress connection ---
        if (m_connectingFromNode >= 0) {
            const AINode* from = graph.findNode(m_connectingFromNode);
            if (from) {
                ImVec2 p1 = getOutputPin(*from, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);
                ImVec2 mousePos = ImGui::GetMousePos();
                float dx = std::abs(mousePos.x - p1.x) * 0.5f;
                if (dx < 40.0f) dx = 40.0f;
                dl->AddBezierCubic(p1, {p1.x + dx, p1.y}, {mousePos.x - dx, mousePos.y},
                                   mousePos, IM_COL32(255, 255, 255, 150), 2.0f);
            }
        }

        // --- Draw nodes ---
        for (auto& node : graph.nodes) {
            float x1, y1, x2, y2;
            getNodeRect(node, offsetX, offsetY, m_zoom, x1, y1, x2, y2, NODE_WIDTH, NODE_HEIGHT);

            bool isSelected = (node.id == m_selectedNode);
            bool isStart = (node.id == graph.startNodeId);
            ImU32 nodeCol = isSelected ? getNodeColorHovered(node.type) : getNodeColor(node.type);

            // Node body
            dl->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), nodeCol, 6.0f * m_zoom);
            if (isSelected) {
                dl->AddRect(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(255, 255, 255, 255), 6.0f * m_zoom, 0, 2.0f);
            }
            if (isStart) {
                dl->AddRect(ImVec2(x1 - 2, y1 - 2), ImVec2(x2 + 2, y2 + 2), IM_COL32(0, 255, 0, 200), 8.0f * m_zoom, 0, 2.0f);
            }

            // Title bar
            float titleH = 22.0f * m_zoom;
            dl->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y1 + titleH),
                              IM_COL32(0, 0, 0, 80), 6.0f * m_zoom);

            // Type label
            const char* typeStr = "?";
            if (node.type == AINodeType::STATE) typeStr = "STATE";
            else if (node.type == AINodeType::CONDITION) typeStr = "COND";
            else if (node.type == AINodeType::ACTION) typeStr = "ACTION";
            dl->AddText(ImVec2(x1 + 6 * m_zoom, y1 + 3 * m_zoom), IM_COL32(255, 255, 255, 200), typeStr);

            // Node name
            dl->AddText(ImVec2(x1 + 6 * m_zoom, y1 + titleH + 4 * m_zoom),
                       IM_COL32(255, 255, 255, 255), node.name.c_str());

            // Input pin (left)
            ImVec2 inPin = getInputPin(node, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);
            dl->AddCircleFilled(inPin, PIN_RADIUS * m_zoom, IM_COL32(100, 200, 100, 255));

            // Output pin (right)
            ImVec2 outPin = getOutputPin(node, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);
            dl->AddCircleFilled(outPin, PIN_RADIUS * m_zoom, IM_COL32(200, 100, 100, 255));
        }

        dl->PopClipRect();

        // ===== INPUT HANDLING =====
        ImGui::SetCursorScreenPos(canvasPos);
        ImGui::InvisibleButton("##canvas_btn", canvasSize);
        bool canvasHovered = ImGui::IsItemHovered();

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;
        ImVec2 mouseDelta = io.MouseDelta;

        // Scroll/zoom
        if (canvasHovered) {
            float scroll = io.MouseWheel;
            if (scroll != 0.0f) {
                float prevZoom = m_zoom;
                m_zoom *= (scroll > 0) ? 1.1f : 0.9f;
                m_zoom = std::clamp(m_zoom, 0.2f, 3.0f);
                // Zoom toward mouse position
                float zoomRatio = m_zoom / prevZoom;
                m_scrollX = mousePos.x - canvasPos.x - (mousePos.x - canvasPos.x - m_scrollX) * zoomRatio;
                m_scrollY = mousePos.y - canvasPos.y - (mousePos.y - canvasPos.y - m_scrollY) * zoomRatio;
            }
        }

        // Middle-mouse pan canvas
        if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            m_canvasDragging = true;
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) m_canvasDragging = false;
        if (m_canvasDragging) {
            m_scrollX += mouseDelta.x;
            m_scrollY += mouseDelta.y;
        }

        // Left-click: select node, start drag, start connection from pin
        if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_selectedNode = -1;
            m_selectedConnection = -1;
            m_draggingNode = -1;

            // Check if clicking on a node or its pin
            for (auto& node : graph.nodes) {
                float x1, y1, x2, y2;
                getNodeRect(node, offsetX, offsetY, m_zoom, x1, y1, x2, y2, NODE_WIDTH, NODE_HEIGHT);

                // Check output pin click (to start connection)
                ImVec2 outPin = getOutputPin(node, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);
                float distOut = sqrtf((mousePos.x - outPin.x) * (mousePos.x - outPin.x) +
                                      (mousePos.y - outPin.y) * (mousePos.y - outPin.y));
                if (distOut <= PIN_RADIUS * m_zoom * 2.0f) {
                    m_connectingFromNode = node.id;
                    break;
                }

                // Check node body click
                if (mousePos.x >= x1 && mousePos.x <= x2 && mousePos.y >= y1 && mousePos.y <= y2) {
                    m_selectedNode = node.id;
                    m_draggingNode = node.id;
                    break;
                }
            }

            // Check if clicking on a connection (near its midpoint)
            if (m_selectedNode < 0) {
                for (const auto& conn : graph.connections) {
                    const AINode* from = graph.findNode(conn.fromNodeId);
                    const AINode* to = graph.findNode(conn.toNodeId);
                    if (!from || !to) continue;
                    ImVec2 p1 = getOutputPin(*from, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);
                    ImVec2 p2 = getInputPin(*to, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);
                    ImVec2 mid = {(p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f};
                    float dist = sqrtf((mousePos.x - mid.x) * (mousePos.x - mid.x) +
                                       (mousePos.y - mid.y) * (mousePos.y - mid.y));
                    if (dist < 15.0f * m_zoom) {
                        m_selectedConnection = conn.id;
                        break;
                    }
                }
            }
        }

        // Drag node
        if (m_draggingNode >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            AINode* node = graph.findNode(m_draggingNode);
            if (node) {
                node->posX += mouseDelta.x / m_zoom;
                node->posY += mouseDelta.y / m_zoom;
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            // Finish connection if releasing on an input pin
            if (m_connectingFromNode >= 0) {
                for (auto& node : graph.nodes) {
                    if (node.id == m_connectingFromNode) continue;
                    ImVec2 inPin = getInputPin(node, offsetX, offsetY, m_zoom, NODE_WIDTH, NODE_HEIGHT);
                    float dist = sqrtf((mousePos.x - inPin.x) * (mousePos.x - inPin.x) +
                                       (mousePos.y - inPin.y) * (mousePos.y - inPin.y));
                    if (dist <= PIN_RADIUS * m_zoom * 2.5f) {
                        // Check for duplicate
                        bool exists = false;
                        for (const auto& c : graph.connections) {
                            if (c.fromNodeId == m_connectingFromNode && c.toNodeId == node.id) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            AIConnection conn;
                            conn.id = graph.nextConnectionId++;
                            conn.fromNodeId = m_connectingFromNode;
                            conn.toNodeId = node.id;
                            conn.conditionType = "timer_expired";
                            conn.conditionValue = 3.0f;
                            conn.comparison = ">";
                            graph.connections.push_back(conn);
                            m_selectedConnection = conn.id;
                        }
                        break;
                    }
                }
                m_connectingFromNode = -1;
            }
            m_draggingNode = -1;
        }

        // Right-click: context menu
        if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            m_showContextMenu = true;
            m_contextMenuX = (mousePos.x - offsetX) / m_zoom;
            m_contextMenuY = (mousePos.y - offsetY) / m_zoom;
        }

        if (m_showContextMenu) {
            ImGui::OpenPopup("##AIContextMenu");
            m_showContextMenu = false;
        }

        if (ImGui::BeginPopup("##AIContextMenu")) {
            ImGui::TextDisabled("Add Node");
            ImGui::Separator();
            if (ImGui::BeginMenu("States")) {
                const char* states[] = {"Idle", "Wander", "Follow", "Flee", "Attack", "Patrol", "Sleep"};
                for (const char* s : states) {
                    if (ImGui::MenuItem(s)) {
                        AINode n;
                        n.id = graph.nextNodeId++;
                        n.type = AINodeType::STATE;
                        n.name = s;
                        n.posX = m_contextMenuX;
                        n.posY = m_contextMenuY;
                        graph.nodes.push_back(n);
                        m_selectedNode = n.id;
                        if (graph.nodes.size() == 1) graph.startNodeId = n.id;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Actions")) {
                const char* actions[] = {"wander_random", "follow_player", "flee_from_player",
                                         "attack_player", "play_sound", "jump", "swim_wander", "fly_wander"};
                for (const char* a : actions) {
                    if (ImGui::MenuItem(a)) {
                        AINode n;
                        n.id = graph.nextNodeId++;
                        n.type = AINodeType::ACTION;
                        n.name = a;
                        n.actionType = a;
                        n.posX = m_contextMenuX;
                        n.posY = m_contextMenuY;
                        graph.nodes.push_back(n);
                        m_selectedNode = n.id;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (m_selectedNode >= 0) {
                if (ImGui::MenuItem("Set as Start Node")) {
                    graph.startNodeId = m_selectedNode;
                }
                if (ImGui::MenuItem("Delete Node")) {
                    // Remove connections
                    graph.connections.erase(
                        std::remove_if(graph.connections.begin(), graph.connections.end(),
                            [&](const AIConnection& c) { return c.fromNodeId == m_selectedNode || c.toNodeId == m_selectedNode; }),
                        graph.connections.end());
                    graph.nodes.erase(
                        std::remove_if(graph.nodes.begin(), graph.nodes.end(),
                            [&](const AINode& n) { return n.id == m_selectedNode; }),
                        graph.nodes.end());
                    m_selectedNode = -1;
                }
            }
            if (m_selectedConnection >= 0) {
                if (ImGui::MenuItem("Delete Connection")) {
                    graph.connections.erase(
                        std::remove_if(graph.connections.begin(), graph.connections.end(),
                            [&](const AIConnection& c) { return c.id == m_selectedConnection; }),
                        graph.connections.end());
                    m_selectedConnection = -1;
                }
            }
            ImGui::EndPopup();
        }

        // Delete key
        if (canvasHovered && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            if (m_selectedConnection >= 0) {
                graph.connections.erase(
                    std::remove_if(graph.connections.begin(), graph.connections.end(),
                        [&](const AIConnection& c) { return c.id == m_selectedConnection; }),
                    graph.connections.end());
                m_selectedConnection = -1;
            } else if (m_selectedNode >= 0) {
                graph.connections.erase(
                    std::remove_if(graph.connections.begin(), graph.connections.end(),
                        [&](const AIConnection& c) { return c.fromNodeId == m_selectedNode || c.toNodeId == m_selectedNode; }),
                    graph.connections.end());
                graph.nodes.erase(
                    std::remove_if(graph.nodes.begin(), graph.nodes.end(),
                        [&](const AINode& n) { return n.id == m_selectedNode; }),
                    graph.nodes.end());
                m_selectedNode = -1;
            }
        }
    }
    ImGui::EndChild();

    // ===== PROPERTIES PANEL =====
    ImGui::BeginChild("##AIProps", ImVec2(0, 0), true);
    {
        if (m_selectedNode >= 0) {
            showNodeProperties(graph);
        } else if (m_selectedConnection >= 0) {
            showConnectionProperties(graph);
        } else {
            ImGui::TextDisabled("Select a node or connection to edit properties.");
            ImGui::TextDisabled("Right-click canvas to add nodes. Drag from output pin (red) to input pin (green).");
        }
    }
    ImGui::EndChild();
}

void AINodeEditor::showNodeProperties(AIGraph& graph) {
    AINode* node = graph.findNode(m_selectedNode);
    if (!node) { m_selectedNode = -1; return; }

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Node Properties");
    ImGui::Separator();

    char nameBuf[64];
    strncpy(nameBuf, node->name.c_str(), 63); nameBuf[63] = '\0';
    if (ImGui::InputText("Name", nameBuf, 64)) node->name = nameBuf;

    const char* typeNames[] = {"State", "Condition", "Action"};
    int typeIdx = (int)node->type;
    if (ImGui::Combo("Type", &typeIdx, typeNames, 3)) {
        node->type = (AINodeType)typeIdx;
    }

    bool isStart = (node->id == graph.startNodeId);
    if (ImGui::Checkbox("Start Node", &isStart)) {
        if (isStart) graph.startNodeId = node->id;
    }

    ImGui::Separator();

    if (node->type == AINodeType::STATE) {
        ImGui::SliderFloat("Duration (0=inf)", &node->duration, 0.0f, 30.0f, "%.1f s");
        ImGui::SliderFloat("Move Speed", &node->moveSpeed, 0.0f, 5.0f, "%.2f");
    }

    if (node->type == AINodeType::ACTION) {
        const char* actionTypes[] = {"wander_random", "follow_player", "flee_from_player",
                                      "attack_player", "play_sound", "jump", "swim_wander", "fly_wander"};
        int actionIdx = 0;
        for (int i = 0; i < 8; i++) {
            if (node->actionType == actionTypes[i]) { actionIdx = i; break; }
        }
        if (ImGui::Combo("Action", &actionIdx, actionTypes, 8)) {
            node->actionType = actionTypes[actionIdx];
        }
        ImGui::SliderFloat("Parameter", &node->actionParam, 0.0f, 20.0f, "%.1f");
    }
}

void AINodeEditor::showConnectionProperties(AIGraph& graph) {
    AIConnection* conn = nullptr;
    for (auto& c : graph.connections) {
        if (c.id == m_selectedConnection) { conn = &c; break; }
    }
    if (!conn) { m_selectedConnection = -1; return; }

    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.6f, 1.0f), "Connection Properties");
    ImGui::Separator();

    const AINode* from = graph.findNode(conn->fromNodeId);
    const AINode* to = graph.findNode(conn->toNodeId);
    ImGui::Text("From: %s -> To: %s",
                from ? from->name.c_str() : "?",
                to ? to->name.c_str() : "?");

    ImGui::Separator();

    const char* condTypes[] = {"distance_to_player", "hp_below", "hp_above",
                                "is_day", "is_night", "in_water", "on_fire",
                                "timer_expired", "random_chance", "was_attacked"};
    int condIdx = 0;
    for (int i = 0; i < 10; i++) {
        if (conn->conditionType == condTypes[i]) { condIdx = i; break; }
    }
    if (ImGui::Combo("Condition", &condIdx, condTypes, 10)) {
        conn->conditionType = condTypes[condIdx];
    }

    const char* compTypes[] = {"<", ">", "<=", ">=", "=="};
    int compIdx = 0;
    for (int i = 0; i < 5; i++) {
        if (conn->comparison == compTypes[i]) { compIdx = i; break; }
    }
    if (ImGui::Combo("Comparison", &compIdx, compTypes, 5)) {
        conn->comparison = compTypes[compIdx];
    }

    ImGui::SliderFloat("Value", &conn->conditionValue, 0.0f, 100.0f, "%.1f");
}
