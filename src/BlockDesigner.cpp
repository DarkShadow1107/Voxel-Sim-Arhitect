#include "BlockDesigner.hpp"
#include "Registry.hpp"
#include "Framebuffer.hpp"
#include "Shader.hpp"
#include "GLMesh.hpp"
#include "MeshBuilder.hpp"
#include "Math.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "imgui.h"

#include <algorithm>
#include <string>
#include <cstring>
#include <cstdio>
#include <fstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

namespace {
    std::string openFileDialog() {
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = 260;
        ofn.lpstrFilter = "Audio Files (*.wav;*.mp3)\0*.wav;*.mp3\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameA(&ofn) == TRUE) return std::string(szFile);
        return "";
    }
}
#else
namespace { std::string openFileDialog() { return ""; } }
#endif

void BlockDesigner::show(bool* open, unsigned int atlasID, Framebuffer* previewBuffer, Shader* previewShader) {
    if (!*open) return;
    ImGui::SetNextWindowSize(ImVec2(1150, 750), ImGuiCond_FirstUseEver);

    // Intercept close: if dirty, show save prompt instead
    bool windowOpen = true;
    if (!ImGui::Begin("Block Designer", &windowOpen, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::End();
        if (!windowOpen && m_dirty) {
            m_showSavePrompt = true;
            m_pendingClose = open;
        } else if (!windowOpen) {
            *open = false;
        }
        return;
    }
    if (!windowOpen && m_dirty) {
        m_showSavePrompt = true;
        m_pendingClose = open;
    } else if (!windowOpen) {
        *open = false;
    }

    auto& blocks = GameRegistry::getInstance().getAllBlocks();
    static uint8_t selectedId = 1;
    static int selectedFace = -1;

    // Update save notification timer
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

    // External edit mode banner
    if (m_externalMode) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.3f, 0.5f, 1.0f));
        ImGui::BeginChild("##ExtBanner", ImVec2(0, 32), false);
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "  Editing Part: %s", m_workingCopy.name.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - 180);
        if (ImGui::SmallButton("Done & Return")) {
            m_externalDone = true;
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // Initialize working copy on first use or when selection changes
    if (!m_externalMode && (!m_initialized || m_workingId != selectedId)) {
        if (m_dirty && m_initialized) {
            // Selection changed while dirty — auto-save backup
            saveBackup();
        }
        loadWorkingCopy(selectedId);
    }

    // Try loading crash backup on first init
    if (!m_initialized) {
        m_initialized = true;
        if (loadBackup()) {
            m_dirty = true;
        }
    }

    // --- Ctrl+Z / Ctrl+Y / Ctrl+S ---
    auto& io = ImGui::GetIO();
    bool ctrl = io.KeyCtrl;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undo();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) redo();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && m_dirty) saveToRegistry();

    const char* faceLabels[6] = { "Right (+X)", "Left (-X)", "Top (+Y)", "Bottom (-Y)", "Front (+Z)", "Back (-Z)" };
    const char* faceShort[6] = { "Right", "Left", "Top", "Bottom", "Front", "Back" };
    const ImVec4 faceColors[6] = {
        ImVec4(0.9f, 0.3f, 0.3f, 1.0f),
        ImVec4(0.3f, 0.9f, 0.3f, 1.0f),
        ImVec4(0.3f, 0.6f, 0.9f, 1.0f),
        ImVec4(0.9f, 0.9f, 0.3f, 1.0f),
        ImVec4(0.9f, 0.3f, 0.9f, 1.0f),
        ImVec4(0.3f, 0.9f, 0.9f, 1.0f),
    };

    // ===== FAR LEFT: Block List (hidden in external mode) =====
    if (!m_externalMode) {
        ImGui::BeginChild("##BlockListPanel", ImVec2(m_listPanelWidth, 0), true);
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "BLOCKS");
        ImGui::Separator();

        // Search filter
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##search", "Search...", m_searchFilter, sizeof(m_searchFilter));
        ImGui::Spacing();

        for (auto& [id, def] : blocks) {
            if (id == 0) continue;
            // Apply search filter
            if (m_searchFilter[0] != '\0') {
                std::string nameLower = def.name;
                std::string filterLower = m_searchFilter;
                for (auto& c : nameLower) c = (char)tolower(c);
                for (auto& c : filterLower) c = (char)tolower(c);
                if (nameLower.find(filterLower) == std::string::npos) continue;
            }
            ImGui::PushID(id);
            bool sel = (selectedId == id);
            if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.28f, 0.56f, 1.0f, 0.5f));
            if (ImGui::Selectable(def.name.c_str(), sel)) {
                selectedId = id;
                selectedFace = -1;
            }
            if (sel) ImGui::PopStyleColor();
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("+ Add Block", ImVec2(-1, 28))) {
            uint8_t nextId = 1;
            while (blocks.count(nextId)) nextId++;
            BlockDefinition newDef;
            newDef.id = nextId;
            newDef.name = "New Block " + std::to_string(nextId);
            for (int i = 0; i < 6; i++) { newDef.faces[i] = {0, 0, {1, 1, 1}}; }
            GameRegistry::getInstance().registerBlock(newDef);
            selectedId = nextId;
        }
        if (blocks.count(selectedId) && selectedId != 0) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button("- Delete", ImVec2(-1, 24))) {
                blocks.erase(selectedId);
                selectedId = blocks.empty() ? 0 : blocks.begin()->first;
            }
            ImGui::PopStyleColor();
        }
        ImGui::Spacing();
        // Copy / Paste block
        if (blocks.count(selectedId) && selectedId != 0) {
            float halfW = ImGui::GetContentRegionAvail().x * 0.5f - 2;
            if (ImGui::Button("Copy", ImVec2(halfW, 22))) {
                m_clipboard = blocks[selectedId];
                m_hasClipboard = true;
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, m_hasClipboard ? ImVec4(0.3f, 0.6f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            if (ImGui::Button("Paste", ImVec2(-1, 22)) && m_hasClipboard) {
                uint8_t keepId = selectedId;
                std::string keepName = blocks[selectedId].name;
                blocks[selectedId] = m_clipboard;
                blocks[selectedId].id = keepId;
                blocks[selectedId].name = keepName;
                loadWorkingCopy(selectedId);
            }
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();

        // Vertical splitter handle
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 1.0f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
        ImGui::Button("##listSplitter", ImVec2(4.0f, ImGui::GetContentRegionAvail().y));
        if (ImGui::IsItemActive()) {
            m_listPanelWidth += ImGui::GetIO().MouseDelta.x;
            m_listPanelWidth = std::clamp(m_listPanelWidth, 100.0f, 400.0f);
        }
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        if (!blocks.count(selectedId)) { ImGui::End(); return; }
    }

    // Wrap all right-side content in a child to keep layout after SameLine
    ImGui::BeginChild("##BlockRightSide", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    auto& def = m_workingCopy;

    // ===== TOOLBAR: Save / Discard / Undo / Redo =====
    {
        ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Save", ImVec2(70, 28)) && m_dirty) {
            saveToRegistry();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save changes to registry (Ctrl+S)");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.7f, 0.3f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Discard", ImVec2(70, 28)) && m_dirty) {
            discardChanges();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Discard all unsaved changes");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, m_undoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
        if (ImGui::Button("Undo", ImVec2(55, 28)) && !m_undoStack.empty()) undo();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Undo (Ctrl+Z)");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, m_redoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
        if (ImGui::Button("Redo", ImVec2(55, 28)) && !m_redoStack.empty()) redo();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Redo (Ctrl+Y)");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        if (m_dirty) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " * Unsaved Changes");
        }
    }
    ImGui::Separator();

    // Snapshot before any edit for undo tracking
    BlockDefinition preEditSnapshot = def;

    // ===== LEFT-CENTER: 2D Unfolded Cube + Properties =====
    ImGui::BeginChild("##Block2DPanel", ImVec2(m_propsPanelWidth, 0), false);
    {
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "PROPERTIES");
        ImGui::Separator();
        char nameBuf[64];
        strncpy(nameBuf, def.name.c_str(), 63); nameBuf[63] = '\0';
        if (ImGui::InputText("Name", nameBuf, 64)) def.name = nameBuf;
        if (!m_externalMode) {
            ImGui::Text("Block ID: %d", def.id);
        }
        {
            bool wasPF = def.usePerFace;
            ImGui::Checkbox("Per-Face Mode", &def.usePerFace);
            if (def.usePerFace && !wasPF) {
                // Copy global settings to all faces when first enabling
                for (int j = 0; j < 6; j++) {
                    def.faces[j].texX = def.texX;
                    def.faces[j].texY = def.texY;
                    def.faces[j].color = def.color;
                }
                selectedFace = 4; // Auto-select front face
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(or click a face below)");
        }
        if (!m_externalMode) {
            ImGui::SameLine(); ImGui::Checkbox("Transparent", &def.isTransparent);
            ImGui::SameLine(); ImGui::Checkbox("Liquid", &def.isLiquid);
        }

        ImGui::Spacing();
        ImGui::Separator();

        // -- 2D Unfolded Cube (cross layout) --
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "2D UNFOLDED CUBE");
        ImGui::Spacing();

        const float cellW = 90.0f;
        const float cellH = 90.0f;
        const float gap = 4.0f;

        struct UnfoldSlot { int col; int row; int faceIdx; };
        const UnfoldSlot slots[6] = {
            {1, 0, 2}, // Top
            {0, 1, 1}, // Left
            {1, 1, 4}, // Front
            {2, 1, 0}, // Right
            {3, 1, 5}, // Back
            {1, 2, 3}, // Bottom
        };

        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (int s = 0; s < 6; s++) {
            int col = slots[s].col;
            int row = slots[s].row;
            int fi = slots[s].faceIdx;

            float x = origin.x + col * (cellW + gap);
            float y = origin.y + row * (cellH + gap);

            ImVec4 fc = faceColors[fi];
            bool isSel = (selectedFace == fi);

            // Background: show atlas texture tile if available
            if (atlasID > 0) {
                int tx = def.usePerFace ? def.faces[fi].texX : def.texX;
                int ty = def.usePerFace ? def.faces[fi].texY : def.texY;
                Vec3 fCol = def.usePerFace ? def.faces[fi].color : def.color;
                float u0 = tx / 16.0f, v0 = ty / 16.0f;
                float u1 = (tx + 1) / 16.0f, v1 = (ty + 1) / 16.0f;
                // Tinted texture tile
                ImVec4 tint(fCol.x, fCol.y, fCol.z, 1.0f);
                drawList->AddImage((ImTextureID)(uintptr_t)atlasID,
                    ImVec2(x + 2, y + 2), ImVec2(x + cellW - 2, y + cellH - 2),
                    ImVec2(u0, v0), ImVec2(u1, v1),
                    ImGui::GetColorU32(tint));
            } else {
                ImU32 bgCol = IM_COL32((int)(fc.x * 80), (int)(fc.y * 80), (int)(fc.z * 80), 180);
                drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + cellW, y + cellH), bgCol, 4.0f);
            }

            ImU32 borderCol = isSel
                ? IM_COL32(255, 255, 50, 255)
                : IM_COL32((int)(fc.x * 255), (int)(fc.y * 255), (int)(fc.z * 255), 160);
            drawList->AddRect(ImVec2(x, y), ImVec2(x + cellW, y + cellH), borderCol, 4.0f, 0, isSel ? 3.0f : 1.5f);

            // Face label with shadow for readability over texture
            char txt[64];
            snprintf(txt, sizeof(txt), "%s", faceShort[fi]);
            ImVec2 textSize = ImGui::CalcTextSize(txt);
            float textX = x + (cellW - textSize.x) * 0.5f;
            float textY = y + 6;
            drawList->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, 180), txt);
            drawList->AddText(ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), txt);

            char coordTxt[32];
            int tx = def.usePerFace ? def.faces[fi].texX : def.texX;
            int ty = def.usePerFace ? def.faces[fi].texY : def.texY;
            snprintf(coordTxt, sizeof(coordTxt), "[%d, %d]", tx, ty);
            ImVec2 coordSize = ImGui::CalcTextSize(coordTxt);
            float coordX = x + (cellW - coordSize.x) * 0.5f;
            float coordY = y + 24;
            drawList->AddText(ImVec2(coordX + 1, coordY + 1), IM_COL32(0, 0, 0, 160), coordTxt);
            drawList->AddText(ImVec2(coordX, coordY), IM_COL32(200, 200, 200, 230), coordTxt);

            auto& faceCol = def.faces[fi].color;
            ImU32 swCol = IM_COL32((int)(faceCol.x * 255), (int)(faceCol.y * 255), (int)(faceCol.z * 255), 255);
            float swatchSize = 28.0f;
            float swX = x + (cellW - swatchSize) * 0.5f;
            float swY = y + cellH - swatchSize - 8;
            drawList->AddRectFilled(ImVec2(swX, swY), ImVec2(swX + swatchSize, swY + swatchSize), swCol, 3.0f);
            drawList->AddRect(ImVec2(swX, swY), ImVec2(swX + swatchSize, swY + swatchSize), IM_COL32(180, 180, 180, 200), 3.0f);

            ImGui::SetCursorScreenPos(ImVec2(x, y));
            ImGui::PushID(500 + fi);
            if (ImGui::InvisibleButton("##face", ImVec2(cellW, cellH))) {
                // Auto-enable Per-Face Mode when clicking a face
                if (!def.usePerFace) {
                    def.usePerFace = true;
                    // Copy global settings to all faces so nothing visually changes
                    for (int j = 0; j < 6; j++) {
                        def.faces[j].texX = def.texX;
                        def.faces[j].texY = def.texY;
                        def.faces[j].color = def.color;
                    }
                }
                selectedFace = (selectedFace == fi) ? -1 : fi;
            }
            ImGui::PopID();
        }

        ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + 3 * (cellH + gap) + 8));

        ImGui::Spacing();
        ImGui::Separator();

        // -- Face Editor --
        if (def.usePerFace) {
            if (selectedFace >= 0 && selectedFace < 6) {
                ImGui::TextColored(faceColors[selectedFace], "Editing: %s", faceLabels[selectedFace]);
                ImGui::Separator();
                auto& face = def.faces[selectedFace];
                ImGui::ColorEdit3("Color", &face.color.x, ImGuiColorEditFlags_Float);
                ImGui::DragInt("Texture X", &face.texX, 0.15f, 0, 15);
                ImGui::DragInt("Texture Y", &face.texY, 0.15f, 0, 15);
                ImGui::Spacing();
                if (ImGui::Button("Copy to All Faces")) {
                    for (int j = 0; j < 6; j++) {
                        if (j != selectedFace) def.faces[j] = face;
                    }
                }

                // Texture Atlas Picker
                if (atlasID > 0) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "TEXTURE ATLAS");
                    ImGui::TextDisabled("Click a tile to set texture coordinates.");
                    float atlasDisplaySize = std::min(ImGui::GetContentRegionAvail().x - 8, 320.0f);
                    if (atlasDisplaySize < 128) atlasDisplaySize = 128;
                    float tileSize = atlasDisplaySize / 16.0f;
                    ImVec2 atlasPos = ImGui::GetCursorScreenPos();
                    ImGui::Image((ImTextureID)(uintptr_t)atlasID, ImVec2(atlasDisplaySize, atlasDisplaySize));
                    ImDrawList* atlasDL = ImGui::GetWindowDrawList();
                    for (int gi = 0; gi <= 16; gi++) {
                        atlasDL->AddLine(ImVec2(atlasPos.x + gi * tileSize, atlasPos.y), ImVec2(atlasPos.x + gi * tileSize, atlasPos.y + atlasDisplaySize), IM_COL32(255,255,255,30));
                        atlasDL->AddLine(ImVec2(atlasPos.x, atlasPos.y + gi * tileSize), ImVec2(atlasPos.x + atlasDisplaySize, atlasPos.y + gi * tileSize), IM_COL32(255,255,255,30));
                    }
                    float hlX = atlasPos.x + face.texX * tileSize;
                    float hlY = atlasPos.y + face.texY * tileSize;
                    atlasDL->AddRect(ImVec2(hlX, hlY), ImVec2(hlX + tileSize, hlY + tileSize), IM_COL32(255, 255, 50, 255), 0, 0, 2.5f);
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        ImVec2 mouse = ImGui::GetIO().MousePos;
                        int clickX = (int)((mouse.x - atlasPos.x) / tileSize);
                        int clickY = (int)((mouse.y - atlasPos.y) / tileSize);
                        face.texX = std::clamp(clickX, 0, 15);
                        face.texY = std::clamp(clickY, 0, 15);
                    }
                }
            } else {
                ImGui::TextDisabled("Click a face on the unfolded cube to edit it.");
            }
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Global Settings");
            ImGui::ColorEdit3("Color", &def.color.x, ImGuiColorEditFlags_Float);
            ImGui::DragInt("Texture X", &def.texX, 0.15f, 0, 15);
            ImGui::DragInt("Texture Y", &def.texY, 0.15f, 0, 15);
            if (ImGui::Button("Apply to All Faces")) {
                for (int i = 0; i < 6; i++) {
                    def.faces[i].color = def.color;
                    def.faces[i].texX = def.texX;
                    def.faces[i].texY = def.texY;
                }
            }

            // Texture Atlas Picker (global)
            if (atlasID > 0) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "TEXTURE ATLAS");
                ImGui::TextDisabled("Click a tile to set texture coordinates.");
                float atlasDisplaySize = std::min(ImGui::GetContentRegionAvail().x - 8, 320.0f);
                if (atlasDisplaySize < 128) atlasDisplaySize = 128;
                float tileSize = atlasDisplaySize / 16.0f;
                ImVec2 atlasPos = ImGui::GetCursorScreenPos();
                ImGui::Image((ImTextureID)(uintptr_t)atlasID, ImVec2(atlasDisplaySize, atlasDisplaySize));
                ImDrawList* atlasDL = ImGui::GetWindowDrawList();
                for (int gi = 0; gi <= 16; gi++) {
                    atlasDL->AddLine(ImVec2(atlasPos.x + gi * tileSize, atlasPos.y), ImVec2(atlasPos.x + gi * tileSize, atlasPos.y + atlasDisplaySize), IM_COL32(255,255,255,30));
                    atlasDL->AddLine(ImVec2(atlasPos.x, atlasPos.y + gi * tileSize), ImVec2(atlasPos.x + atlasDisplaySize, atlasPos.y + gi * tileSize), IM_COL32(255,255,255,30));
                }
                float hlX = atlasPos.x + def.texX * tileSize;
                float hlY = atlasPos.y + def.texY * tileSize;
                atlasDL->AddRect(ImVec2(hlX, hlY), ImVec2(hlX + tileSize, hlY + tileSize), IM_COL32(255, 255, 50, 255), 0, 0, 2.5f);
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    ImVec2 mouse = ImGui::GetIO().MousePos;
                    int clickX = (int)((mouse.x - atlasPos.x) / tileSize);
                    int clickY = (int)((mouse.y - atlasPos.y) / tileSize);
                    def.texX = std::clamp(clickX, 0, 15);
                    def.texY = std::clamp(clickY, 0, 15);
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Block-specific sections (hidden when editing mob parts/tool pieces)
        if (!m_externalMode) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Sounds");
            char breakBuf[128], stepBuf[128];
            strncpy(breakBuf, def.breakSound.c_str(), 127); breakBuf[127] = '\0';
            strncpy(stepBuf, def.stepSound.c_str(), 127); stepBuf[127] = '\0';
            if (ImGui::InputText("Break Sound", breakBuf, 128)) def.breakSound = breakBuf;
            ImGui::SameLine();
            if (ImGui::SmallButton("Browse##brk")) {
                std::string p = openFileDialog();
                if (!p.empty()) def.breakSound = p;
            }
            if (ImGui::InputText("Step Sound", stepBuf, 128)) def.stepSound = stepBuf;
            ImGui::SameLine();
            if (ImGui::SmallButton("Browse##stp")) {
                std::string p = openFileDialog();
                if (!p.empty()) def.stepSound = p;
            }

            // ===== BLOCK MECHANICS =====
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Block Mechanics");

            ImGui::DragFloat("Break Time (s)", &def.breakTime, 0.05f, 0.0f, 60.0f, "%.2f");
            ImGui::DragFloat("Blast Resistance", &def.blastResistance, 0.2f, 0.0f, 100.0f, "%.1f");
            ImGui::DragInt("Light Emission", &def.lightEmission, 0.15f, 0, 15);
            ImGui::Checkbox("Has Gravity", &def.hasGravity);

            // Required tool
            const char* toolTypes[] = {"None (Hand)", "pickaxe", "axe", "shovel", "hoe", "sword"};
            int toolIdx = 0;
            if (def.requiredToolType == "pickaxe") toolIdx = 1;
            else if (def.requiredToolType == "axe") toolIdx = 2;
            else if (def.requiredToolType == "shovel") toolIdx = 3;
            else if (def.requiredToolType == "hoe") toolIdx = 4;
            else if (def.requiredToolType == "sword") toolIdx = 5;
            if (ImGui::Combo("Required Tool", &toolIdx, toolTypes, 6)) {
                def.requiredToolType = (toolIdx == 0) ? "" : toolTypes[toolIdx];
            }

            const char* tierNames[] = {"Hand (0)", "Wood (1)", "Stone (2)", "Iron (3)", "Diamond (4)"};
            ImGui::Combo("Min Tool Tier", &def.requiredToolTier, tierNames, 5);

            // Drop system
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "Drops");
            ImGui::Checkbox("Drops Itself", &def.dropsItself);
            if (!def.dropsItself) {
                for (int di = 0; di < (int)def.drops.size(); di++) {
                    ImGui::PushID(di);
                    auto& drop = def.drops[di];
                    int dropBlockId = (int)drop.blockId;
                    ImGui::InputInt("Block ID", &dropBlockId);
                    drop.blockId = (uint8_t)std::clamp(dropBlockId, 0, 255);
                    ImGui::InputInt("Min", &drop.minCount);
                    ImGui::SameLine(); ImGui::InputInt("Max", &drop.maxCount);
                    ImGui::DragFloat("Chance", &drop.chance, 0.005f, 0.0f, 1.0f, "%.2f");
                    if (ImGui::SmallButton("Remove")) {
                        def.drops.erase(def.drops.begin() + di);
                        di--;
                    }
                    ImGui::Separator();
                    ImGui::PopID();
                }
                if (ImGui::Button("+ Add Drop")) {
                    def.drops.push_back({0, 1, 1, 1.0f});
                }
            }

            // ===== PHYSICS PROPERTIES =====
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), "Physics Properties");
            ImGui::DragFloat("Friction", &def.friction, 0.005f, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("Slipperiness", &def.slipperiness, 0.005f, 0.0f, 1.0f, "%.2f");
            ImGui::DragInt("Opacity", &def.opacity, 0.15f, 0, 15);
            ImGui::Checkbox("Flammable", &def.flammable);
            if (def.flammable) {
                ImGui::Indent(10.0f);
                ImGui::DragInt("Burn Time (ticks)", &def.burnTime, 1.0f, 1, 600);
                ImGui::Unindent(10.0f);
            }
            ImGui::Checkbox("Replaceable", &def.replaceable);
            ImGui::DragInt("Redstone Power", &def.redstonePower, 0.15f, 0, 15);

            // ===== BLOCK INTERACTIONS =====
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.9f, 1.0f), "Block Interactions");
            ImGui::TextDisabled("What happens when specific neighbors are present.");
            for (int ii = 0; ii < (int)def.interactions.size(); ii++) {
                ImGui::PushID(900 + ii);
                auto& inter = def.interactions[ii];
                ImGui::Spacing();
                ImGui::Text("When:");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##neighbor", blocks.count(inter.neighborBlockId) ? blocks[inter.neighborBlockId].name.c_str() : "?")) {
                    for (auto& [nid, ndef] : blocks) {
                        if (nid == 0) continue;
                        if (ImGui::Selectable(ndef.name.c_str(), inter.neighborBlockId == nid))
                            inter.neighborBlockId = nid;
                    }
                    ImGui::EndCombo();
                }
                const char* conditions[] = {"adjacent", "above", "below"};
                int condIdx = 0;
                if (inter.condition == "above") condIdx = 1;
                else if (inter.condition == "below") condIdx = 2;
                ImGui::Text("Is:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                if (ImGui::Combo("##cond", &condIdx, conditions, 3)) inter.condition = conditions[condIdx];
                ImGui::Text("Becomes:");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##result", blocks.count(inter.resultBlockId) ? blocks[inter.resultBlockId].name.c_str() : "?")) {
                    for (auto& [rid, rdef] : blocks) {
                        if (ImGui::Selectable(rdef.name.c_str(), inter.resultBlockId == rid))
                            inter.resultBlockId = rid;
                    }
                    ImGui::EndCombo();
                }
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
                if (ImGui::SmallButton("Remove##inter")) {
                    def.interactions.erase(def.interactions.begin() + ii);
                    ii--;
                }
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::PopID();
            }
            if (ImGui::Button("+ Add Interaction", ImVec2(150, 24))) {
                def.interactions.push_back({});
            }

            // ===== BLOCK STATES =====
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "Block States");
            ImGui::Checkbox("Has States", &def.hasStates);
            if (def.hasStates) {
                ImGui::DragInt("State Count", &def.stateCount, 0.1f, 1, 4);
                for (int si = 0; si < def.stateCount; si++) {
                    ImGui::PushID(950 + si);
                    char stateLabel[32];
                    snprintf(stateLabel, sizeof(stateLabel), "State %d", si);
                    char stateBuf[64];
                    strncpy(stateBuf, def.stateNames[si].c_str(), 63); stateBuf[63] = '\0';
                    if (ImGui::InputText(stateLabel, stateBuf, 64)) def.stateNames[si] = stateBuf;
                    ImGui::PopID();
                }
            }
        }
    }
    ImGui::EndChild();

    // Vertical splitter between properties and 3D preview
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 1.0f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
    ImGui::Button("##propsSplitter", ImVec2(4.0f, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemActive()) {
        m_propsPanelWidth += ImGui::GetIO().MouseDelta.x;
        m_propsPanelWidth = std::clamp(m_propsPanelWidth, 250.0f, 700.0f);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // ===== RIGHT: Interactive 3D Preview =====
    ImGui::BeginChild("##Block3DPanel", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "3D PREVIEW");
        ImGui::Separator();

        // View controls (above preview so they are always visible)
        if (ImGui::Button("Reset View")) { m_yaw = 0.78f; m_pitch = 0.4f; m_dist = 3.5f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Front")) { m_yaw = 0.0f; m_pitch = 0.0f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Top")) { m_yaw = 0.0f; m_pitch = 1.3f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Side")) { m_yaw = 1.5708f; m_pitch = 0.0f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::TextDisabled("Right Mouse Button (hold): Rotate view");
        ImGui::TextDisabled("Scroll Wheel: Zoom in (toward you) / out (away)");
        ImGui::TextDisabled("Middle Mouse Button (hold): Pan view");

        m_selectedFaceForPreview = selectedFace;
        renderPreview(def, atlasID, previewBuffer, previewShader);
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float previewSide = std::min(avail.x - 16, avail.y - 8);
        if (previewSide < 128) previewSide = 128;
        ImVec2 previewSize(previewSide, previewSide);
        float padX = (avail.x - previewSide) * 0.5f;
        if (padX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padX);

        // Display preview with ImGui::Image (proper widget)
        ImGui::Image((ImTextureID)(uintptr_t)getPreviewTexture(previewBuffer),
                     previewSize, ImVec2(0, 1), ImVec2(1, 0));

        // Interaction: hover detection on the Image item
        static bool s_blockOrbit = false;
        static bool s_blockPan = false;
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                s_blockOrbit = true;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                s_blockPan = true;
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll != 0.0f) {
                m_dist += scroll * 0.3f;
                m_dist  = std::clamp(m_dist, 1.5f, 8.0f);
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
            s_blockOrbit = false;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            s_blockPan = false;
        if (s_blockOrbit) {
            ImGuiIO& pio = ImGui::GetIO();
            m_yaw   -= pio.MouseDelta.x * 0.01f;
            m_pitch += pio.MouseDelta.y * 0.01f;
            m_pitch  = std::clamp(m_pitch, -1.4f, 1.4f);
        }
        if (s_blockPan) {
            ImGuiIO& pio = ImGui::GetIO();
            m_panX -= pio.MouseDelta.x * 0.005f * m_dist;
            m_panY += pio.MouseDelta.y * 0.005f * m_dist;
        }
    }
    ImGui::EndChild();

    // --- Detect if working copy changed this frame (for undo push) ---
    {
        // Simple comparison: check if any byte changed
        bool changed = false;
        if (def.name != preEditSnapshot.name) changed = true;
        if (def.usePerFace != preEditSnapshot.usePerFace) changed = true;
        if (def.isTransparent != preEditSnapshot.isTransparent) changed = true;
        if (def.isLiquid != preEditSnapshot.isLiquid) changed = true;
        if (def.texX != preEditSnapshot.texX || def.texY != preEditSnapshot.texY) changed = true;
        if (def.breakSound != preEditSnapshot.breakSound) changed = true;
        if (def.stepSound != preEditSnapshot.stepSound) changed = true;
        for (int i = 0; i < 6; i++) {
            if (def.faces[i].texX != preEditSnapshot.faces[i].texX ||
                def.faces[i].texY != preEditSnapshot.faces[i].texY) changed = true;
            if (def.faces[i].color.x != preEditSnapshot.faces[i].color.x ||
                def.faces[i].color.y != preEditSnapshot.faces[i].color.y ||
                def.faces[i].color.z != preEditSnapshot.faces[i].color.z) changed = true;
        }
        if (def.color.x != preEditSnapshot.color.x ||
            def.color.y != preEditSnapshot.color.y ||
            def.color.z != preEditSnapshot.color.z) changed = true;
        if (def.breakTime != preEditSnapshot.breakTime) changed = true;
        if (def.blastResistance != preEditSnapshot.blastResistance) changed = true;
        if (def.lightEmission != preEditSnapshot.lightEmission) changed = true;
        if (def.hasGravity != preEditSnapshot.hasGravity) changed = true;
        if (def.requiredToolType != preEditSnapshot.requiredToolType) changed = true;
        if (def.requiredToolTier != preEditSnapshot.requiredToolTier) changed = true;
        if (def.dropsItself != preEditSnapshot.dropsItself) changed = true;
        if (def.friction != preEditSnapshot.friction) changed = true;
        if (def.slipperiness != preEditSnapshot.slipperiness) changed = true;
        if (def.opacity != preEditSnapshot.opacity) changed = true;
        if (def.flammable != preEditSnapshot.flammable) changed = true;
        if (def.burnTime != preEditSnapshot.burnTime) changed = true;
        if (def.replaceable != preEditSnapshot.replaceable) changed = true;
        if (def.redstonePower != preEditSnapshot.redstonePower) changed = true;
        if (def.hasStates != preEditSnapshot.hasStates) changed = true;
        if (def.stateCount != preEditSnapshot.stateCount) changed = true;
        if (changed) {
            // Push the pre-edit state onto undo stack
            m_undoStack.push_back(preEditSnapshot);
            if ((int)m_undoStack.size() > kMaxUndoSteps)
                m_undoStack.erase(m_undoStack.begin());
            m_redoStack.clear();
            m_dirty = true;
            saveBackup();
        }
    }

    // --- Save prompt popup ---
    if (m_showSavePrompt) {
        ImGui::OpenPopup("Save Changes?##Block");
        m_showSavePrompt = false;
    }
    if (ImGui::BeginPopupModal("Save Changes?##Block", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (m_externalMode) {
            ImGui::Text("You have unsaved changes to this part.");
            ImGui::Text("Do you want to apply them before returning?");
        } else {
            ImGui::Text("You have unsaved changes in Block Designer.");
            ImGui::Text("Do you want to save before closing?");
        }
        ImGui::Spacing();
        if (ImGui::Button(m_externalMode ? "Apply" : "Save", ImVec2(100, 30))) {
            if (m_externalMode) {
                m_externalSaved = true;
            }
            saveToRegistry();
            if (m_pendingClose) *m_pendingClose = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(100, 30))) {
            discardChanges();
            if (m_pendingClose) *m_pendingClose = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::EndChild(); // end ##BlockRightSide
    ImGui::End();
}

void BlockDesigner::renderPreview(const BlockDefinition& def, unsigned int atlasID, Framebuffer* buf, Shader* shdr) {
    if (!buf || !shdr) return;

    // Save GL state
    GLint prevFBO = 0, prevViewport[4] = {};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    buf->bind();
    int pw = buf->getWidth(), ph = buf->getHeight();
    glViewport(0, 0, pw, ph);
    // Gradient background: draw two triangles without shader
    glClearColor(0.14f, 0.14f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    // Simple gradient via glClear is enough; use darker bottom
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    shdr->use();

    // Use external shape dimensions when editing a mob part / tool piece
    float bx = m_externalMode ? m_externalShape.x : 1.0f;
    float by = m_externalMode ? m_externalShape.y : 1.0f;
    float bz = m_externalMode ? m_externalShape.z : 1.0f;

    float cx = bx * 0.5f, cy = by * 0.5f, cz = bz * 0.5f;
    float camX = cx + m_dist * cosf(m_pitch) * sinf(m_yaw);
    float camY = cy + m_dist * sinf(m_pitch);
    float camZ = cz + m_dist * cosf(m_pitch) * cosf(m_yaw);
    // Apply pan offset in camera-local right/up directions
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
    Mat4 proj = perspective(40.0f * (3.14159f / 180.0f), aspect, 0.1f, 30.0f);
    Mat4 model = Mat4::identity();

    shdr->setMat4("uProjection", proj);
    shdr->setMat4("uView", view);
    shdr->setMat4("uModel", model);
    shdr->setVec3("uLightDir", {-0.4f, -0.8f, -0.5f});
    shdr->setVec3("uColorTint", {1, 1, 1});
    shdr->setInt("uTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasID);

    // --- Draw floor grid ---
    {
        MeshBuilder gridMb;
        float gridY = -0.02f;
        float gridExtent = 3.0f;
        float gridStep = 0.5f;
        // Use a neutral grey tile for grid lines
        float gs = 1.0f / 16.0f;
        float gu = 0.0f, gv = 0.0f; // top-left tile as neutral
        for (float gx = -gridExtent; gx <= gridExtent; gx += gridStep) {
            // X-aligned line
            gridMb.addFace({gx, gridY, -gridExtent}, {gx + 0.02f, gridY, -gridExtent},
                           {gx + 0.02f, gridY, gridExtent}, {gx, gridY, gridExtent},
                           {0,1,0}, gu, gv, gu+gs, gv+gs, 0.25f, 0.25f, 0.3f);
            // Z-aligned line
            gridMb.addFace({-gridExtent, gridY, gx}, {gridExtent, gridY, gx},
                           {gridExtent, gridY, gx + 0.02f}, {-gridExtent, gridY, gx + 0.02f},
                           {0,1,0}, gu, gv, gu+gs, gv+gs, 0.25f, 0.25f, 0.3f);
        }
        GLMesh gridMesh;
        gridMesh.upload(gridMb.getVertices());
        gridMesh.draw();
        gridMesh.destroy();
    }

    // --- Draw solid block ---
    MeshBuilder mb;
    float s = 1.0f / 16.0f;

    auto addBlockFace = [&](int faceIdx, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n) {
        int tx = def.texX, ty = def.texY;
        Vec3 color = def.color;
        if (def.usePerFace) {
            tx = def.faces[faceIdx].texX;
            ty = def.faces[faceIdx].texY;
            color = def.faces[faceIdx].color;
        }
        float u1 = tx * s, v1 = ty * s;
        float u2 = u1 + s, v2 = v1 + s;
        mb.addFace(p1, p2, p3, p4, n, u1, v1, u2, v2, color.x, color.y, color.z);
    };

    addBlockFace(0, {bx,0,0}, {bx,0,bz}, {bx,by,bz}, {bx,by,0}, {1,0,0});
    addBlockFace(1, {0,0,bz}, {0,0,0}, {0,by,0}, {0,by,bz}, {-1,0,0});
    addBlockFace(2, {0,by,0}, {bx,by,0}, {bx,by,bz}, {0,by,bz}, {0,1,0});
    addBlockFace(3, {0,0,bz}, {bx,0,bz}, {bx,0,0}, {0,0,0}, {0,-1,0});
    addBlockFace(4, {0,0,bz}, {bx,0,bz}, {bx,by,bz}, {0,by,bz}, {0,0,1});
    addBlockFace(5, {bx,0,0}, {0,0,0}, {0,by,0}, {bx,by,0}, {0,0,-1});

    GLMesh mesh;
    mesh.upload(mb.getVertices());
    mesh.draw();

    mesh.destroy();

    // --- Face highlight outline ---
    if (m_selectedFaceForPreview >= 0 && m_selectedFaceForPreview < 6) {
        struct FaceQuad { Vec3 p1, p2, p3, p4; Vec3 n; };
        const FaceQuad faceQuads[6] = {
            {{bx,0,0}, {bx,0,bz}, {bx,by,bz}, {bx,by,0}, {1,0,0}},   // +X
            {{0,0,bz}, {0,0,0}, {0,by,0}, {0,by,bz}, {-1,0,0}},  // -X
            {{0,by,0}, {bx,by,0}, {bx,by,bz}, {0,by,bz}, {0,1,0}},   // +Y
            {{0,0,bz}, {bx,0,bz}, {bx,0,0}, {0,0,0}, {0,-1,0}},  // -Y
            {{0,0,bz}, {bx,0,bz}, {bx,by,bz}, {0,by,bz}, {0,0,1}},   // +Z
            {{bx,0,0}, {0,0,0}, {0,by,0}, {bx,by,0}, {0,0,-1}},  // -Z
        };
        const auto& fq = faceQuads[m_selectedFaceForPreview];
        MeshBuilder selMb;
        float gs = 1.0f / 16.0f;
        selMb.addFace(fq.p1, fq.p2, fq.p3, fq.p4, fq.n, 0, 0, gs, gs, 1.0f, 1.0f, 0.3f);
        GLMesh selMesh;
        selMesh.upload(selMb.getVertices());

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);
        glDisable(GL_DEPTH_TEST);
        shdr->setVec3("uColorTint", {1.0f, 1.0f, 0.3f});
        selMesh.draw();
        selMesh.destroy();
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        shdr->setVec3("uColorTint", {1, 1, 1});
    }
    buf->resolve();
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glDepthFunc(GL_LEQUAL); // Restore main renderer's depth func
}

unsigned int BlockDesigner::getPreviewTexture(Framebuffer* buf) {
    return buf ? buf->getTexture() : 0;
}

// --- Save / Undo / Redo Implementation ---

void BlockDesigner::loadWorkingCopy(uint8_t id) {
    auto& blocks = GameRegistry::getInstance().getAllBlocks();
    if (blocks.count(id)) {
        m_workingCopy = blocks[id];
    } else {
        m_workingCopy = {};
        m_workingCopy.id = id;
    }
    m_workingId = id;
    m_dirty = false;
    m_undoStack.clear();
    m_redoStack.clear();
}

void BlockDesigner::pushUndo() {
    m_undoStack.push_back(m_workingCopy);
    if ((int)m_undoStack.size() > kMaxUndoSteps)
        m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();
    m_dirty = true;
}

void BlockDesigner::undo() {
    if (m_undoStack.empty()) return;
    m_redoStack.push_back(m_workingCopy);
    m_workingCopy = m_undoStack.back();
    m_undoStack.pop_back();
    m_dirty = !m_undoStack.empty();
    saveBackup();
}

void BlockDesigner::redo() {
    if (m_redoStack.empty()) return;
    m_undoStack.push_back(m_workingCopy);
    m_workingCopy = m_redoStack.back();
    m_redoStack.pop_back();
    m_dirty = true;
    saveBackup();
}

void BlockDesigner::saveToRegistry() {
    if (m_externalMode) {
        m_externalSaved = true;
        m_dirty = false;
        m_saveNotifyTimer = 2.0f;
        return;
    }
    auto& blocks = GameRegistry::getInstance().getAllBlocks();
    blocks[m_workingId] = m_workingCopy;
    m_dirty = false;
    m_saveNotifyTimer = 2.0f;
    m_undoStack.clear();
    m_redoStack.clear();
    clearBackup();
}

void BlockDesigner::discardChanges() {
    loadWorkingCopy(m_workingId);
    clearBackup();
}

void BlockDesigner::saveBackup() const {
    std::ofstream out(kBackupFile, std::ios::binary);
    if (!out) return;
    // Simple binary backup: id + name length + name + per-face + faces + sounds
    out.write((const char*)&m_workingId, sizeof(m_workingId));
    uint32_t nameLen = (uint32_t)m_workingCopy.name.size();
    out.write((const char*)&nameLen, sizeof(nameLen));
    out.write(m_workingCopy.name.data(), nameLen);
    out.write((const char*)&m_workingCopy.usePerFace, sizeof(m_workingCopy.usePerFace));
    out.write((const char*)&m_workingCopy.isTransparent, sizeof(m_workingCopy.isTransparent));
    out.write((const char*)&m_workingCopy.isLiquid, sizeof(m_workingCopy.isLiquid));
    out.write((const char*)&m_workingCopy.color, sizeof(Vec3));
    out.write((const char*)&m_workingCopy.texX, sizeof(int));
    out.write((const char*)&m_workingCopy.texY, sizeof(int));
    for (int i = 0; i < 6; i++) {
        out.write((const char*)&m_workingCopy.faces[i].texX, sizeof(int));
        out.write((const char*)&m_workingCopy.faces[i].texY, sizeof(int));
        out.write((const char*)&m_workingCopy.faces[i].color, sizeof(Vec3));
    }
}

bool BlockDesigner::loadBackup() {
    std::ifstream in(kBackupFile, std::ios::binary);
    if (!in) return false;

    uint8_t id;
    in.read((char*)&id, sizeof(id));
    if (!in) return false;

    uint32_t nameLen;
    in.read((char*)&nameLen, sizeof(nameLen));
    if (!in || nameLen > 256) return false;

    m_workingId = id;
    m_workingCopy.id = id;
    m_workingCopy.name.resize(nameLen);
    in.read(m_workingCopy.name.data(), nameLen);
    in.read((char*)&m_workingCopy.usePerFace, sizeof(m_workingCopy.usePerFace));
    in.read((char*)&m_workingCopy.isTransparent, sizeof(m_workingCopy.isTransparent));
    in.read((char*)&m_workingCopy.isLiquid, sizeof(m_workingCopy.isLiquid));
    in.read((char*)&m_workingCopy.color, sizeof(Vec3));
    in.read((char*)&m_workingCopy.texX, sizeof(int));
    in.read((char*)&m_workingCopy.texY, sizeof(int));
    for (int i = 0; i < 6; i++) {
        in.read((char*)&m_workingCopy.faces[i].texX, sizeof(int));
        in.read((char*)&m_workingCopy.faces[i].texY, sizeof(int));
        in.read((char*)&m_workingCopy.faces[i].color, sizeof(Vec3));
    }
    return in.good();
}

void BlockDesigner::clearBackup() const {
    std::remove(kBackupFile);
}

void BlockDesigner::loadExternalBlock(const BlockDefinition& def) {
    m_workingCopy = def;
    m_workingId = def.id;
    m_externalMode = true;
    m_externalSaved = false;
    m_externalDone = false;
    m_externalShape = {1.0f, 1.0f, 1.0f};
    m_dirty = false;
    m_undoStack.clear();
    m_redoStack.clear();
}

void BlockDesigner::loadExternalBlock(const BlockDefinition& def, Vec3 shapeSize) {
    loadExternalBlock(def);
    m_externalShape = shapeSize;
}

void BlockDesigner::exitExternalMode() {
    m_externalMode = false;
    m_externalSaved = false;
    m_externalDone = false;
    m_externalShape = {1.0f, 1.0f, 1.0f};
    m_dirty = false;
    m_undoStack.clear();
    m_redoStack.clear();
    m_initialized = false;
}
