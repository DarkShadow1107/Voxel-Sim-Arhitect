#include "ToolDesigner.hpp"
#include "Registry.hpp"
#include "Framebuffer.hpp"
#include "Shader.hpp"
#include "GLMesh.hpp"
#include "MeshBuilder.hpp"
#include "Math.hpp"
#include "imgui.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>

void ToolDesigner::show(bool* open, unsigned int atlasID, Framebuffer* previewBuffer, Shader* previewShader) {
    if (!*open) return;
    ImVec2 mvCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(mvCenter, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(1050, 650), ImGuiCond_FirstUseEver);

    bool windowOpen = true;
    if (!ImGui::Begin("Tool Designer", &windowOpen)) {
        ImGui::End();
        if (!windowOpen) *open = false;
        return;
    }
    if (!windowOpen) *open = false;

    // Save notification
    if (m_saveNotifyTimer > 0.0f) {
        m_saveNotifyTimer -= ImGui::GetIO().DeltaTime;
        float alpha = std::min(1.0f, m_saveNotifyTimer * 2.0f);
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        const char* msg = "Saved!";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        float nx = winPos.x + winSize.x - ts.x - 20;
        float ny = winPos.y + 8;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(nx - 8, ny - 4), ImVec2(nx + ts.x + 8, ny + ts.y + 4),
            IM_COL32(40, 120, 60, (int)(alpha * 200)), 4.0f);
        ImGui::GetWindowDrawList()->AddText(ImVec2(nx, ny),
            IM_COL32(255, 255, 255, (int)(alpha * 255)), msg);
    }

    auto& tools = GameRegistry::getInstance().getAllTools();
    static int selectedToolId = -1;

    // Pick a valid selection if current is invalid
    if (!tools.count(selectedToolId) && !tools.empty()) {
        selectedToolId = tools.begin()->first;
    }

    // Initialize working copy
    if (!m_initialized || m_workingId != selectedToolId) {
        if (m_dirty && m_initialized) saveToRegistry();
        loadWorkingCopy(selectedToolId);
    }
    if (!m_initialized) m_initialized = true;

    // Ctrl+Z / Ctrl+Y / Ctrl+S
    auto& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) redo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && m_dirty) saveToRegistry();

    // ===== LEFT PANEL: Tool List =====
    ImGui::BeginChild("##ToolListPanel", ImVec2(m_listPanelWidth, 0), true);
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "TOOLS");
    ImGui::Separator();

    // Search filter
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##toolSearch", "Search...", m_searchFilter, sizeof(m_searchFilter));
    ImGui::Spacing();

    for (auto& [id, tool] : tools) {
        // Apply search filter
        if (m_searchFilter[0] != '\0') {
            std::string nameLower = tool.name;
            std::string filterLower = m_searchFilter;
            for (auto& c : nameLower) c = (char)tolower(c);
            for (auto& c : filterLower) c = (char)tolower(c);
            if (nameLower.find(filterLower) == std::string::npos) continue;
        }
        ImGui::PushID(id);
        bool sel = (selectedToolId == id);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.5f, 0.1f, 0.5f));

        char label[128];
        snprintf(label, sizeof(label), "%s (T%d)", tool.name.c_str(), tool.tier);
        if (ImGui::Selectable(label, sel)) {
            selectedToolId = id;
        }
        if (sel) ImGui::PopStyleColor();
        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("+ Add Tool", ImVec2(-1, 28))) {
        int newId = 100;
        while (tools.count(newId)) newId++;
        ToolDefinition t;
        t.id = newId;
        t.name = "New Tool";
        t.toolType = "pickaxe";
        t.tier = 1;
        t.speedMultiplier = 2.0f;
        t.damage = 2.0f;
        t.durability = 100;
        GameRegistry::getInstance().registerTool(t);
        selectedToolId = newId;
        loadWorkingCopy(newId);
    }

    if (tools.count(selectedToolId)) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("- Delete Tool", ImVec2(-1, 24))) {
            tools.erase(selectedToolId);
            selectedToolId = tools.empty() ? -1 : tools.begin()->first;
            loadWorkingCopy(selectedToolId);
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    // Vertical splitter handle
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 1.0f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
    ImGui::Button("##toolListSplitter", ImVec2(4.0f, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemActive()) {
        m_listPanelWidth += ImGui::GetIO().MouseDelta.x;
        m_listPanelWidth = std::clamp(m_listPanelWidth, 120.0f, 400.0f);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    if (!tools.count(selectedToolId) || selectedToolId < 0) {
        ImGui::TextDisabled("No tool selected. Add a tool or select one from the list.");
        ImGui::End();
        return;
    }

    // Ensure custom pieces match procedural piece count
    ensureCustomPieces();

    // If editing a piece in Block Designer, show banner and skip normal UI
    if (m_editingPieceExternally) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.3f, 0.5f, 1.0f));
        ImGui::BeginChild("##ExtEditBanner", ImVec2(0, 0), true);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f),
            "  Piece '%s' is being edited in Block Designer.",
            m_editingPieceIdx >= 0 && m_editingPieceIdx < (int)m_workingCopy.customPieces.size()
                ? m_workingCopy.customPieces[m_editingPieceIdx].name.c_str() : "?");
        ImGui::Spacing();
        ImGui::TextDisabled("  Use the Block Designer window to edit textures/colors, then click 'Done & Return'.");
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    // ===== CENTER PANEL: Properties + All sections =====
    ImGui::BeginChild("##ToolPropsPanel", ImVec2(m_propsPanelWidth, 0), true);
    {
        auto& t = m_workingCopy;

        // Toolbar
        {
            ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            if (ImGui::Button("Save", ImVec2(70, 28)) && m_dirty) saveToRegistry();
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.7f, 0.3f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            if (ImGui::Button("Discard", ImVec2(70, 28)) && m_dirty) discardChanges();
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, m_undoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
            if (ImGui::Button("Undo", ImVec2(55, 28)) && !m_undoStack.empty()) undo();
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, m_redoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
            if (ImGui::Button("Redo", ImVec2(55, 28)) && !m_redoStack.empty()) redo();
            ImGui::PopStyleColor();

            if (m_dirty) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " * Unsaved");
            }
        }
        ImGui::Separator();

        // Snapshot for undo tracking
        ToolDefinition preEdit = t;

        // Properties
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "TOOL PROPERTIES");
        ImGui::Separator();

        ImGui::Text("Tool ID: %d", t.id);

        char nameBuf[64];
        strncpy(nameBuf, t.name.c_str(), 63); nameBuf[63] = '\0';
        if (ImGui::InputText("Name", nameBuf, 64)) t.name = nameBuf;

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Classification");

        const char* tTypes[] = {"pickaxe", "axe", "shovel", "hoe", "sword", "bow", "shield", "fishing_rod"};
        int tIdx = 0;
        for (int i = 0; i < 8; i++) if (t.toolType == tTypes[i]) { tIdx = i; break; }
        if (ImGui::Combo("Tool Type", &tIdx, tTypes, 8)) t.toolType = tTypes[tIdx];

        const char* tierNames[] = {"Wood (1)", "Stone (2)", "Iron (3)", "Diamond (4)"};
        int tierIdx = std::clamp(t.tier - 1, 0, 3);
        if (ImGui::Combo("Tier", &tierIdx, tierNames, 4)) t.tier = tierIdx + 1;

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Stats");

        ImGui::DragFloat("Speed Multiplier", &t.speedMultiplier, 0.1f, 0.5f, 20.0f, "%.1f");
        ImGui::DragFloat("Damage", &t.damage, 0.1f, 0.0f, 20.0f, "%.1f");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Combat");
        ImGui::DragFloat("Attack Speed", &t.attackSpeed, 0.02f, 0.1f, 5.0f, "%.1f /s");
        ImGui::DragFloat("Knockback", &t.knockback, 0.02f, 0.0f, 5.0f, "%.1f");
        float dps = t.damage * t.attackSpeed;
        ImGui::Text("DPS: %.1f", dps);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Durability");
        ImGui::DragInt("Max Uses", &t.durability, 1.0f, 1, 2000);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Special Effect");
        const char* effectNames[] = {"None", "Fire Aspect", "Silk Touch", "Fortune", "Efficiency", "Unbreaking"};
        const char* effectValues[] = {"", "fire_aspect", "silk_touch", "fortune", "efficiency", "unbreaking"};
        int effectIdx = 0;
        for (int i = 1; i < 6; i++) if (t.specialEffect == effectValues[i]) { effectIdx = i; break; }
        if (ImGui::Combo("Effect", &effectIdx, effectNames, 6)) t.specialEffect = effectValues[effectIdx];

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Appearance");
        ImGui::ColorEdit3("Color", &t.color.x, ImGuiColorEditFlags_Float);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Tool Pieces");
        ImGui::TextDisabled("Click to select/highlight in 3D preview. Edit in Block Designer.");
        for (int pi = 0; pi < (int)t.customPieces.size(); pi++) {
            ImGui::PushID(800 + pi);
            auto& piece = t.customPieces[pi];
            bool isSel = (m_selectedPieceForPreview == pi);

            // Color-coded selectable item
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(piece.color.x * 0.4f, piece.color.y * 0.4f, piece.color.z * 0.4f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(piece.color.x * 0.6f, piece.color.y * 0.6f, piece.color.z * 0.6f, 0.9f));
            if (isSel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.28f, 0.56f, 1.0f, 0.5f));

            char pieceLabel[128];
            Vec3 shape = getPieceShape(pi);
            snprintf(pieceLabel, sizeof(pieceLabel), "%s  (%.1f x %.1f x %.1f)", piece.name.c_str(), shape.x, shape.y, shape.z);
            if (ImGui::Selectable(pieceLabel, isSel)) {
                m_selectedPieceForPreview = (m_selectedPieceForPreview == pi) ? -1 : pi;
            }

            if (isSel) ImGui::PopStyleColor();
            ImGui::PopStyleColor(2);

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            if (ImGui::SmallButton("Edit in Block Designer")) {
                m_editingPieceIdx = pi;
                m_editingPieceExternally = true;
                m_wantsBlockDesigner = true;
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // --- Crafting Recipe (3x3 grid) ---
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "CRAFTING RECIPE");
        ImGui::TextDisabled("3x3 grid. Click a slot to assign a block.");
        ImGui::Separator();
        {
            auto& blocks = GameRegistry::getInstance().getAllBlocks();
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    int idx = row * 3 + col;
                    if (col > 0) ImGui::SameLine();
                    ImGui::PushID(700 + idx);
                    uint8_t bid = t.craftingRecipe[idx];
                    const char* blockName = "Empty";
                    if (bid != 0 && blocks.count(bid)) blockName = blocks[bid].name.c_str();
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
                    ImGui::SetNextItemWidth(120);
                    if (ImGui::BeginCombo("##craft", blockName)) {
                        if (ImGui::Selectable("Empty", bid == 0)) t.craftingRecipe[idx] = 0;
                        for (auto& [id, bdef] : blocks) {
                            if (id == 0) continue;
                            if (ImGui::Selectable(bdef.name.c_str(), bid == id)) t.craftingRecipe[idx] = id;
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // --- Effectiveness preview ---
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "EFFECTIVENESS");
        ImGui::TextDisabled("How this tool performs on blocks.");
        ImGui::Separator();

        auto& blocks = GameRegistry::getInstance().getAllBlocks();
        ImGui::BeginChild("##EffectivenessTable", ImVec2(0, 200), false);
        ImGui::Columns(3, "##EffCols", true);
        ImGui::SetColumnWidth(0, 130);
        ImGui::SetColumnWidth(1, 80);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Block"); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Base"); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "W/ Tool"); ImGui::NextColumn();
        ImGui::Separator();

        for (auto& [id, bdef] : blocks) {
            if (id == 0) continue;
            if (bdef.requiredToolType.empty() && bdef.breakTime <= 0) continue;
            if (bdef.breakTime <= 0 || bdef.breakTime > 1e6f) continue;

            bool matches = (bdef.requiredToolType == t.toolType);
            bool canHarvest = bdef.requiredToolType.empty() || (matches && t.tier >= bdef.requiredToolTier);

            if (matches || bdef.requiredToolType.empty()) {
                ImGui::Text("%s", bdef.name.c_str()); ImGui::NextColumn();
                ImGui::Text("%.1fs", bdef.breakTime); ImGui::NextColumn();

                float effTime = matches ? bdef.breakTime / t.speedMultiplier : bdef.breakTime;
                if (canHarvest) {
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%.1fs", effTime);
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Can't");
                }
                ImGui::NextColumn();
            }
        }
        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();

        // --- Stat Comparison ---
        if (ImGui::CollapsingHeader("Compare Tools")) {
            if (ImGui::BeginCombo("##CompareSelect", m_compareToolId >= 0 && tools.count(m_compareToolId) ? tools[m_compareToolId].name.c_str() : "Select...")) {
                for (auto& [id, ct] : tools) {
                    if (id == selectedToolId) continue;
                    if (ImGui::Selectable(ct.name.c_str(), m_compareToolId == id)) m_compareToolId = id;
                }
                ImGui::EndCombo();
            }

            if (m_compareToolId >= 0 && tools.count(m_compareToolId)) {
                auto& cmp = tools[m_compareToolId];
                ImGui::Spacing();

                struct StatBar { const char* name; float cur; float other; float maxVal; };
                StatBar stats[] = {
                    {"Damage",     t.damage,          cmp.damage,          20.0f},
                    {"Atk Speed",  t.attackSpeed,     cmp.attackSpeed,     5.0f},
                    {"DPS",        t.damage * t.attackSpeed, cmp.damage * cmp.attackSpeed, 40.0f},
                    {"Knockback",  t.knockback,       cmp.knockback,       5.0f},
                    {"Durability", (float)t.durability, (float)cmp.durability, 2000.0f},
                    {"Speed Mult", t.speedMultiplier, cmp.speedMultiplier, 20.0f},
                };

                ImDrawList* dl = ImGui::GetWindowDrawList();
                for (auto& sb : stats) {
                    ImGui::Text("%s", sb.name);
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    float barW = ImGui::GetContentRegionAvail().x - 10;
                    float barH = 14.0f;
                    float curRatio = std::clamp(sb.cur / sb.maxVal, 0.0f, 1.0f);
                    float otherRatio = std::clamp(sb.other / sb.maxVal, 0.0f, 1.0f);
                    dl->AddRectFilled(pos, ImVec2(pos.x + barW, pos.y + barH), IM_COL32(40, 40, 40, 200));
                    dl->AddRectFilled(pos, ImVec2(pos.x + barW * otherRatio, pos.y + barH), IM_COL32(60, 100, 200, 150));
                    dl->AddRectFilled(pos, ImVec2(pos.x + barW * curRatio, pos.y + barH * 0.5f), IM_COL32(60, 200, 80, 220));
                    char valLabel[64];
                    snprintf(valLabel, sizeof(valLabel), "%.1f vs %.1f", sb.cur, sb.other);
                    dl->AddText(ImVec2(pos.x + 4, pos.y), IM_COL32(255, 255, 255, 230), valLabel);
                    ImGui::Dummy(ImVec2(barW, barH + 2));
                }
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Green = Current");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.9f, 1.0f), "Blue = Comparison");
            }
        }

        // Detect changes for undo
        {
            bool changed = false;
            if (t.name != preEdit.name) changed = true;
            if (t.toolType != preEdit.toolType) changed = true;
            if (t.tier != preEdit.tier) changed = true;
            if (t.speedMultiplier != preEdit.speedMultiplier) changed = true;
            if (t.damage != preEdit.damage) changed = true;
            if (t.color.x != preEdit.color.x || t.color.y != preEdit.color.y || t.color.z != preEdit.color.z) changed = true;
            if (t.durability != preEdit.durability) changed = true;
            if (t.attackSpeed != preEdit.attackSpeed) changed = true;
            if (t.knockback != preEdit.knockback) changed = true;
            if (t.specialEffect != preEdit.specialEffect) changed = true;
            for (int i = 0; i < 9; i++) if (t.craftingRecipe[i] != preEdit.craftingRecipe[i]) { changed = true; break; }
            if (!changed && t.customPieces.size() != preEdit.customPieces.size()) changed = true;
            if (!changed) {
                for (size_t i = 0; i < t.customPieces.size() && i < preEdit.customPieces.size(); i++) {
                    auto& a = t.customPieces[i]; auto& b = preEdit.customPieces[i];
                    if (a.color.x != b.color.x || a.color.y != b.color.y || a.color.z != b.color.z ||
                        a.usePerFace != b.usePerFace || a.texX != b.texX || a.texY != b.texY ||
                        a.name != b.name) { changed = true; break; }
                    if (a.usePerFace) {
                        for (int f = 0; f < 6; f++) {
                            if (a.faces[f].texX != b.faces[f].texX || a.faces[f].texY != b.faces[f].texY ||
                                a.faces[f].color.x != b.faces[f].color.x || a.faces[f].color.y != b.faces[f].color.y ||
                                a.faces[f].color.z != b.faces[f].color.z) { changed = true; break; }
                        }
                    }
                    if (changed) break;
                }
            }
            if (changed) {
                m_undoStack.push_back(preEdit);
                if ((int)m_undoStack.size() > kMaxUndoSteps) m_undoStack.erase(m_undoStack.begin());
                m_redoStack.clear();
                m_dirty = true;
            }
        }
    }
    ImGui::EndChild();

    // Vertical splitter between properties and 3D preview
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 1.0f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
    ImGui::Button("##toolPropsSplitter", ImVec2(4.0f, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemActive()) {
        m_propsPanelWidth += ImGui::GetIO().MouseDelta.x;
        m_propsPanelWidth = std::clamp(m_propsPanelWidth, 250.0f, 700.0f);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // ===== RIGHT PANEL: 3D Preview =====
    ImGui::BeginChild("##Tool3DPanel", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "3D PREVIEW");
        ImGui::Separator();

        if (ImGui::Button("Reset View")) { m_yaw = 0.6f; m_pitch = 0.35f; m_dist = 6.0f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Front")) { m_yaw = 0.0f; m_pitch = 0.0f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Top")) { m_yaw = 0.0f; m_pitch = 1.3f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Side")) { m_yaw = 1.5708f; m_pitch = 0.0f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::TextDisabled("RMB: Rotate | Scroll: Zoom | MMB: Pan");

        renderPreview(m_workingCopy, atlasID, previewBuffer, previewShader);
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float previewSide = std::min(avail.x - 16, avail.y - 8);
        if (previewSide < 128) previewSide = 128;
        ImVec2 previewSize(previewSide, previewSide);
        float padX = (avail.x - previewSide) * 0.5f;
        if (padX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padX);

        ImGui::Image((ImTextureID)(uintptr_t)getPreviewTexture(previewBuffer),
                     previewSize, ImVec2(0, 1), ImVec2(1, 0));

        // Interaction
        static bool s_toolOrbit = false;
        static bool s_toolPan = false;
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                s_toolOrbit = true;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                s_toolPan = true;
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll != 0.0f) {
                m_dist += scroll * 0.4f;
                m_dist  = std::clamp(m_dist, 2.0f, 12.0f);
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
            s_toolOrbit = false;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            s_toolPan = false;
        if (s_toolOrbit) {
            ImGuiIO& pio = ImGui::GetIO();
            m_yaw   -= pio.MouseDelta.x * 0.01f;
            m_pitch += pio.MouseDelta.y * 0.01f;
            m_pitch  = std::clamp(m_pitch, -1.4f, 1.4f);
        }
        if (s_toolPan) {
            ImGuiIO& pio = ImGui::GetIO();
            m_panX -= pio.MouseDelta.x * 0.005f * m_dist;
            m_panY += pio.MouseDelta.y * 0.005f * m_dist;
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void ToolDesigner::renderPreview(const ToolDefinition& def, unsigned int atlasID, Framebuffer* buf, Shader* shdr) {
    if (!buf || !shdr) return;

    GLint prevFBO = 0, prevViewport[4] = {};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    buf->bind();
    int pw = buf->getWidth(), ph = buf->getHeight();
    glViewport(0, 0, pw, ph);
    glClearColor(0.14f, 0.14f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    shdr->use();

    // Camera centered on tool
    float cx = 0.5f, cy = 2.5f, cz = 0.0f;
    float camX = cx + m_dist * cosf(m_pitch) * sinf(m_yaw);
    float camY = cy + m_dist * sinf(m_pitch);
    float camZ = cz + m_dist * cosf(m_pitch) * cosf(m_yaw);
    Vec3 fwd = normalize(Vec3{cx - camX, cy - camY, cz - camZ});
    Vec3 right = normalize(cross(fwd, {0, 1, 0}));
    Vec3 up = cross(right, fwd);
    camX += right.x * m_panX + up.x * m_panY;
    camY += right.y * m_panX + up.y * m_panY;
    camZ += right.z * m_panX + up.z * m_panY;
    cx   += right.x * m_panX + up.x * m_panY;
    cy   += right.y * m_panX + up.y * m_panY;
    cz   += right.z * m_panX + up.z * m_panY;
    Mat4 view = lookAt({camX, camY, camZ}, {cx, cy, cz}, {0, 1, 0});
    float aspect = (float)pw / (float)ph;
    Mat4 proj = perspective(40.0f * (3.14159f / 180.0f), aspect, 0.1f, 40.0f);

    shdr->setMat4("uProjection", proj);
    shdr->setMat4("uView", view);
    shdr->setMat4("uModel", Mat4::identity());
    shdr->setVec3("uLightDir", {-0.4f, -0.8f, -0.5f});
    shdr->setVec3("uColorTint", {1, 1, 1});
    shdr->setInt("uTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasID);

    // --- Floor grid ---
    {
        MeshBuilder gridMb;
        float gridY = -0.02f;
        float gridExtent = 3.0f;
        float gridStep = 0.5f;
        float gs = 1.0f / 16.0f;
        float gu = 0.0f, gv = 0.0f;
        for (float gx = -gridExtent; gx <= gridExtent; gx += gridStep) {
            gridMb.addFace({gx, gridY, -gridExtent}, {gx + 0.02f, gridY, -gridExtent},
                           {gx + 0.02f, gridY, gridExtent}, {gx, gridY, gridExtent},
                           {0,1,0}, gu, gv, gu+gs, gv+gs, 0.25f, 0.25f, 0.3f);
            gridMb.addFace({-gridExtent, gridY, gx}, {gridExtent, gridY, gx},
                           {gridExtent, gridY, gx + 0.02f}, {-gridExtent, gridY, gx + 0.02f},
                           {0,1,0}, gu, gv, gu+gs, gv+gs, 0.25f, 0.25f, 0.3f);
        }
        GLMesh gridMesh;
        gridMesh.upload(gridMb.getVertices());
        gridMesh.draw();
        gridMesh.destroy();
    }

    // --- Build tool shape from cubes ---
    struct ToolPiece { Vec3 offset; Vec3 size; };
    std::vector<ToolPiece> pieces;

    if (def.toolType == "pickaxe") {
        pieces.push_back({{0.0f, 0.0f, 0.0f}, {0.3f, 3.0f, 0.3f}});   // handle
        pieces.push_back({{-1.0f, 3.0f, 0.0f}, {2.3f, 0.5f, 0.3f}});   // head
        pieces.push_back({{-1.2f, 2.7f, 0.0f}, {0.4f, 0.3f, 0.3f}});   // left tip
        pieces.push_back({{1.1f,  2.7f, 0.0f}, {0.4f, 0.3f, 0.3f}});   // right tip
    } else if (def.toolType == "axe") {
        pieces.push_back({{0.0f, 0.0f, 0.0f}, {0.3f, 3.0f, 0.3f}});
        pieces.push_back({{0.3f, 2.5f, 0.0f}, {1.0f, 1.0f, 0.3f}});
        pieces.push_back({{1.0f, 2.0f, 0.0f}, {0.5f, 0.5f, 0.3f}});
    } else if (def.toolType == "shovel") {
        pieces.push_back({{0.0f, 0.5f, 0.0f}, {0.3f, 2.5f, 0.3f}});
        pieces.push_back({{-0.15f, 0.0f, -0.05f}, {0.6f, 0.8f, 0.4f}});
    } else if (def.toolType == "hoe") {
        pieces.push_back({{0.0f, 0.0f, 0.0f}, {0.3f, 3.0f, 0.3f}});
        pieces.push_back({{0.3f, 3.0f, 0.0f}, {1.2f, 0.3f, 0.3f}});
    } else if (def.toolType == "sword") {
        pieces.push_back({{0.0f, 0.0f, 0.0f}, {0.4f, 0.5f, 0.4f}});   // pommel
        pieces.push_back({{-0.2f, 0.5f, 0.1f}, {0.8f, 0.2f, 0.2f}});   // guard
        pieces.push_back({{0.05f, 0.7f, 0.05f}, {0.3f, 2.8f, 0.3f}});  // blade
        pieces.push_back({{0.05f, 3.5f, 0.05f}, {0.3f, 0.4f, 0.2f}});  // tip
    } else if (def.toolType == "bow") {
        // Curved limbs (3 offset cubes) + string
        pieces.push_back({{0.0f, 0.5f, 0.0f}, {0.15f, 1.2f, 0.15f}});    // lower limb
        pieces.push_back({{0.1f, 1.7f, 0.0f}, {0.15f, 1.2f, 0.15f}});    // middle limb
        pieces.push_back({{0.0f, 2.9f, 0.0f}, {0.15f, 1.2f, 0.15f}});    // upper limb
        pieces.push_back({{-0.15f, 0.5f, 0.06f}, {0.04f, 3.6f, 0.04f}}); // string
    } else if (def.toolType == "shield") {
        // Large flat front + handle behind
        pieces.push_back({{-1.0f, 0.0f, 0.0f}, {2.0f, 2.5f, 0.3f}});   // shield face
        pieces.push_back({{-0.15f, 0.8f, 0.3f}, {0.3f, 0.8f, 0.3f}});   // handle
    } else if (def.toolType == "fishing_rod") {
        // Long thin handle + hook
        pieces.push_back({{0.0f, 0.0f, 0.0f}, {0.15f, 4.0f, 0.15f}});   // rod
        pieces.push_back({{0.0f, 4.0f, 0.0f}, {0.08f, 0.3f, 0.08f}});   // tip
        pieces.push_back({{-0.1f, -0.4f, -0.1f}, {0.1f, 0.4f, 0.1f}});  // hook
        pieces.push_back({{0.03f, 4.0f, 0.03f}, {0.02f, 0.02f, 0.02f}});// line anchor (tiny)
    } else {
        pieces.push_back({{0.0f, 0.0f, 0.0f}, {0.3f, 3.0f, 0.3f}});
        pieces.push_back({{-0.35f, 3.0f, -0.15f}, {1.0f, 0.6f, 0.6f}});
    }

    // Draw each piece using custom piece appearance if available
    float s = 1.0f / 16.0f;
    auto buildCube = [&](Vec3 off, Vec3 sz, Vec3 col) {
        MeshBuilder mb;
        float ox = off.x, oy = off.y, oz = off.z;
        float sx = sz.x, sy = sz.y, sz2 = sz.z;
        float u = 0.0f, v = 0.0f;
        mb.addFace({ox+sx,oy,oz}, {ox+sx,oy,oz+sz2}, {ox+sx,oy+sy,oz+sz2}, {ox+sx,oy+sy,oz},
                   {1,0,0}, u,v,u+s,v+s, col.x, col.y, col.z);
        mb.addFace({ox,oy,oz+sz2}, {ox,oy,oz}, {ox,oy+sy,oz}, {ox,oy+sy,oz+sz2},
                   {-1,0,0}, u,v,u+s,v+s, col.x, col.y, col.z);
        mb.addFace({ox,oy+sy,oz}, {ox+sx,oy+sy,oz}, {ox+sx,oy+sy,oz+sz2}, {ox,oy+sy,oz+sz2},
                   {0,1,0}, u,v,u+s,v+s, col.x, col.y, col.z);
        mb.addFace({ox,oy,oz+sz2}, {ox+sx,oy,oz+sz2}, {ox+sx,oy,oz}, {ox,oy,oz},
                   {0,-1,0}, u,v,u+s,v+s, col.x, col.y, col.z);
        mb.addFace({ox,oy,oz+sz2}, {ox+sx,oy,oz+sz2}, {ox+sx,oy+sy,oz+sz2}, {ox,oy+sy,oz+sz2},
                   {0,0,1}, u,v,u+s,v+s, col.x, col.y, col.z);
        mb.addFace({ox+sx,oy,oz}, {ox,oy,oz}, {ox,oy+sy,oz}, {ox+sx,oy+sy,oz},
                   {0,0,-1}, u,v,u+s,v+s, col.x, col.y, col.z);
        return mb;
    };

    shdr->setMat4("uModel", Mat4::identity());
    for (int i = 0; i < (int)pieces.size(); i++) {
        Vec3 col;
        bool hasCustom = (i < (int)def.customPieces.size());
        if (hasCustom) {
            col = def.customPieces[i].color;
        } else {
            Vec3 toolColor = def.color;
            Vec3 handleColor = {toolColor.x * 0.5f + 0.2f, toolColor.y * 0.3f + 0.15f, toolColor.z * 0.1f + 0.05f};
            if (def.toolType == "bow") {
                col = (i < 3) ? handleColor : Vec3{0.7f, 0.7f, 0.7f};
            } else if (def.toolType == "shield") {
                col = (i == 0) ? toolColor : handleColor;
            } else {
                col = (i == 0) ? handleColor : toolColor;
            }
        }

        // Build cube with per-face textures if custom piece has them
        if (hasCustom && def.customPieces[i].usePerFace) {
            MeshBuilder mb;
            float ox = pieces[i].offset.x, oy = pieces[i].offset.y, oz = pieces[i].offset.z;
            float sx = pieces[i].size.x, sy = pieces[i].size.y, sz2 = pieces[i].size.z;
            const auto& cp = def.customPieces[i];
            auto addPieceFace = [&](int fi, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n) {
                int tx = cp.faces[fi].texX, ty = cp.faces[fi].texY;
                Vec3 fc = cp.faces[fi].color;
                float u1 = tx * s, v1 = ty * s;
                mb.addFace(p1, p2, p3, p4, n, u1, v1, u1+s, v1+s, fc.x, fc.y, fc.z);
            };
            addPieceFace(0, {ox+sx,oy,oz},{ox+sx,oy,oz+sz2},{ox+sx,oy+sy,oz+sz2},{ox+sx,oy+sy,oz}, {1,0,0});
            addPieceFace(1, {ox,oy,oz+sz2},{ox,oy,oz},{ox,oy+sy,oz},{ox,oy+sy,oz+sz2}, {-1,0,0});
            addPieceFace(2, {ox,oy+sy,oz},{ox+sx,oy+sy,oz},{ox+sx,oy+sy,oz+sz2},{ox,oy+sy,oz+sz2}, {0,1,0});
            addPieceFace(3, {ox,oy,oz+sz2},{ox+sx,oy,oz+sz2},{ox+sx,oy,oz},{ox,oy,oz}, {0,-1,0});
            addPieceFace(4, {ox,oy,oz+sz2},{ox+sx,oy,oz+sz2},{ox+sx,oy+sy,oz+sz2},{ox,oy+sy,oz+sz2}, {0,0,1});
            addPieceFace(5, {ox+sx,oy,oz},{ox,oy,oz},{ox,oy+sy,oz},{ox+sx,oy+sy,oz}, {0,0,-1});
            GLMesh mesh;
            mesh.upload(mb.getVertices());
            shdr->setVec3("uColorTint", {1, 1, 1});
            mesh.draw();
            mesh.destroy();
        } else {
            MeshBuilder mb = buildCube(pieces[i].offset, pieces[i].size, col);
            GLMesh mesh;
            mesh.upload(mb.getVertices());
            shdr->setVec3("uColorTint", {1, 1, 1});
            mesh.draw();
            mesh.destroy();
        }

        // Highlight selected piece with yellow wireframe
        if (i == m_selectedPieceForPreview) {
            MeshBuilder hlMb = buildCube(pieces[i].offset, pieces[i].size, {1.0f, 1.0f, 0.3f});
            GLMesh hlMesh;
            hlMesh.upload(hlMb.getVertices());
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(3.0f);
            glDisable(GL_DEPTH_TEST);
            shdr->setVec3("uColorTint", {1.0f, 1.0f, 0.3f});
            hlMesh.draw();
            hlMesh.destroy();
            glEnable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            shdr->setVec3("uColorTint", {1, 1, 1});
        }
    }

    // Restore GL state
    buf->resolve();
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glDepthFunc(GL_LEQUAL);
}

unsigned int ToolDesigner::getPreviewTexture(Framebuffer* buf) {
    return buf ? buf->getTexture() : 0;
}

// --- Save / Undo / Redo ---

void ToolDesigner::loadWorkingCopy(int id) {
    auto& tools = GameRegistry::getInstance().getAllTools();
    if (tools.count(id)) {
        m_workingCopy = tools[id];
    } else {
        m_workingCopy = {};
        m_workingCopy.id = id;
    }
    m_workingId = id;
    m_dirty = false;
    m_undoStack.clear();
    m_redoStack.clear();
}

void ToolDesigner::pushUndo() {
    m_undoStack.push_back(m_workingCopy);
    if ((int)m_undoStack.size() > kMaxUndoSteps) m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();
    m_dirty = true;
}

void ToolDesigner::undo() {
    if (m_undoStack.empty()) return;
    m_redoStack.push_back(m_workingCopy);
    m_workingCopy = m_undoStack.back();
    m_undoStack.pop_back();
    m_dirty = !m_undoStack.empty();
}

void ToolDesigner::redo() {
    if (m_redoStack.empty()) return;
    m_undoStack.push_back(m_workingCopy);
    m_workingCopy = m_redoStack.back();
    m_redoStack.pop_back();
    m_dirty = true;
}

void ToolDesigner::saveToRegistry() {
    auto& tools = GameRegistry::getInstance().getAllTools();
    tools[m_workingId] = m_workingCopy;
    m_dirty = false;
    m_saveNotifyTimer = 2.0f;
    m_undoStack.clear();
    m_redoStack.clear();
}

void ToolDesigner::discardChanges() {
    loadWorkingCopy(m_workingId);
}

int ToolDesigner::getProceduralPieceCount() const {
    const auto& type = m_workingCopy.toolType;
    if (type == "pickaxe") return 4;
    if (type == "axe") return 3;
    if (type == "shovel") return 2;
    if (type == "hoe") return 2;
    if (type == "sword") return 4;
    if (type == "bow") return 4;
    if (type == "shield") return 2;
    if (type == "fishing_rod") return 4;
    return 2; // default
}

void ToolDesigner::ensureCustomPieces() {
    int count = getProceduralPieceCount();
    if ((int)m_workingCopy.customPieces.size() != count) {
        // Build default pieces from the procedural coloring
        Vec3 toolColor = m_workingCopy.color;
        Vec3 handleColor = {toolColor.x * 0.5f + 0.2f, toolColor.y * 0.3f + 0.15f, toolColor.z * 0.1f + 0.05f};
        m_workingCopy.customPieces.resize(count);
        const char* pieceNames[] = {"Piece 0", "Piece 1", "Piece 2", "Piece 3"};
        for (int i = 0; i < count; i++) {
            auto& p = m_workingCopy.customPieces[i];
            p.name = (i < 4) ? pieceNames[i] : "Piece " + std::to_string(i);
            if (m_workingCopy.toolType == "bow") {
                p.color = (i < 3) ? handleColor : Vec3{0.7f, 0.7f, 0.7f};
            } else if (m_workingCopy.toolType == "shield") {
                p.color = (i == 0) ? toolColor : handleColor;
            } else {
                p.color = (i == 0) ? handleColor : toolColor;
            }
        }
    }
}

BlockDefinition ToolDesigner::getPieceAsBlock(int pieceIdx) const {
    BlockDefinition block{};
    block.id = 254; // Temporary ID
    if (pieceIdx >= 0 && pieceIdx < (int)m_workingCopy.customPieces.size()) {
        const auto& piece = m_workingCopy.customPieces[pieceIdx];
        block.name = piece.name;
        block.usePerFace = piece.usePerFace;
        block.color = piece.color;
        block.texX = piece.texX;
        block.texY = piece.texY;
        for (int i = 0; i < 6; i++) block.faces[i] = piece.faces[i];
    } else {
        block.name = "Tool Piece";
    }
    return block;
}

void ToolDesigner::applyBlockToPiece(int pieceIdx, const BlockDefinition& block) {
    if (pieceIdx < 0 || pieceIdx >= (int)m_workingCopy.customPieces.size()) return;
    pushUndo();
    auto& piece = m_workingCopy.customPieces[pieceIdx];
    piece.name = block.name;
    piece.usePerFace = block.usePerFace;
    piece.color = block.color;
    piece.texX = block.texX;
    piece.texY = block.texY;
    for (int i = 0; i < 6; i++) piece.faces[i] = block.faces[i];
    m_dirty = true;
}

Vec3 ToolDesigner::getPieceShape(int pieceIdx) const {
    // Must match the piece geometry defined in renderPreview()
    struct PieceGeom { Vec3 offset; Vec3 size; };
    std::vector<PieceGeom> pieces;
    const auto& type = m_workingCopy.toolType;

    if (type == "pickaxe") {
        pieces = {{{0,0,0},{0.3f,3.0f,0.3f}}, {{-1,3,0},{2.3f,0.5f,0.3f}},
                  {{-1.2f,2.7f,0},{0.4f,0.3f,0.3f}}, {{1.1f,2.7f,0},{0.4f,0.3f,0.3f}}};
    } else if (type == "axe") {
        pieces = {{{0,0,0},{0.3f,3.0f,0.3f}}, {{0.3f,2.5f,0},{1.0f,1.0f,0.3f}},
                  {{1.0f,2.0f,0},{0.5f,0.5f,0.3f}}};
    } else if (type == "shovel") {
        pieces = {{{0,0.5f,0},{0.3f,2.5f,0.3f}}, {{-0.15f,0,-0.05f},{0.6f,0.8f,0.4f}}};
    } else if (type == "hoe") {
        pieces = {{{0,0,0},{0.3f,3.0f,0.3f}}, {{0.3f,3.0f,0},{1.2f,0.3f,0.3f}}};
    } else if (type == "sword") {
        pieces = {{{0,0,0},{0.4f,0.5f,0.4f}}, {{-0.2f,0.5f,0.1f},{0.8f,0.2f,0.2f}},
                  {{0.05f,0.7f,0.05f},{0.3f,2.8f,0.3f}}, {{0.05f,3.5f,0.05f},{0.3f,0.4f,0.2f}}};
    } else if (type == "bow") {
        pieces = {{{0,0.5f,0},{0.15f,1.2f,0.15f}}, {{0.1f,1.7f,0},{0.15f,1.2f,0.15f}},
                  {{0,2.9f,0},{0.15f,1.2f,0.15f}}, {{-0.15f,0.5f,0.06f},{0.04f,3.6f,0.04f}}};
    } else if (type == "shield") {
        pieces = {{{-1,0,0},{2.0f,2.5f,0.3f}}, {{-0.15f,0.8f,0.3f},{0.3f,0.8f,0.3f}}};
    } else if (type == "fishing_rod") {
        pieces = {{{0,0,0},{0.15f,4.0f,0.15f}}, {{0,4.0f,0},{0.08f,0.3f,0.08f}},
                  {{-0.1f,-0.4f,-0.1f},{0.1f,0.4f,0.1f}}, {{0.03f,4.0f,0.03f},{0.02f,0.02f,0.02f}}};
    } else {
        pieces = {{{0,0,0},{0.3f,3.0f,0.3f}}, {{-0.35f,3.0f,-0.15f},{1.0f,0.6f,0.6f}}};
    }

    if (pieceIdx >= 0 && pieceIdx < (int)pieces.size())
        return pieces[pieceIdx].size;
    return {1.0f, 1.0f, 1.0f};
}
