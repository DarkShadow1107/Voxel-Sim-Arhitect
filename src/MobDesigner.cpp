#include "MobDesigner.hpp"
#include "Registry.hpp"
#include "Framebuffer.hpp"
#include "Shader.hpp"
#include "GLMesh.hpp"
#include "MeshBuilder.hpp"
#include "Math.hpp"
#include "AINodeEditor.hpp"

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

void MobDesigner::show(bool* open, unsigned int atlasID, Framebuffer* previewBuffer, Shader* previewShader, Framebuffer* partPreviewBuf) {
    if (!*open) return;
    ImGui::SetNextWindowSize(ImVec2(1100, 700), ImGuiCond_FirstUseEver);

    // Intercept close: if dirty, show save prompt
    bool windowOpen = true;
    if (!ImGui::Begin("Mob Designer", &windowOpen, ImGuiWindowFlags_NoScrollbar)) {
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

    auto& reg = GameRegistry::getInstance();
    auto& mobs = reg.getAllMobs();
    static MobType selectedType = MOB_COW;
    static int selectedPartIdx = -1;

    // Initialize or switch working copy
    if (!m_initialized || m_workingType != selectedType) {
        if (m_dirty && m_initialized) saveBackup();
        loadWorkingCopy(selectedType);
    }
    if (!m_initialized) {
        m_initialized = true;
        if (loadBackup()) m_dirty = true;
    }

    // --- Ctrl+Z / Ctrl+Y ---
    auto& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) redo();

    // ===== LEFT PANEL: Mob List =====
    ImGui::BeginChild("##MobListPanel", ImVec2(m_listPanelWidth, 0), true);
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "MOBS");
    ImGui::Separator();

    for (auto& [type, def] : mobs) {
        ImGui::PushID((int)type);
        bool sel = (selectedType == type);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.7f, 0.4f, 0.5f));
        char label[128];
        snprintf(label, sizeof(label), "%s  [%zu parts]", def.name.c_str(), def.parts.size());
        if (ImGui::Selectable(label, sel)) {
            selectedType = type;
            selectedPartIdx = -1;
        }
        if (sel) ImGui::PopStyleColor();
        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("+ Add Mob", ImVec2(-1, 28))) {
        int nextId = (int)MOB_COUNT;
        while (mobs.count((MobType)nextId)) nextId++;
        MobDefinition newMob;
        newMob.type = (MobType)nextId;
        newMob.name = "New Mob";
        newMob.maxHp = 10.0f;
        newMob.speed = 2.0f;
        MobPart body;
        body.name = "Body";
        body.offset = {-0.4f, 0.3f, -0.5f};
        body.size = {0.8f, 0.6f, 1.0f};
        body.color = {0.7f, 0.5f, 0.3f};
        newMob.parts.push_back(body);
        reg.registerMob(newMob);
        selectedType = (MobType)nextId;
        selectedPartIdx = 0;
    }
    if (mobs.count(selectedType)) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("- Delete Mob", ImVec2(-1, 24))) {
            mobs.erase(selectedType);
            selectedType = mobs.empty() ? MOB_COW : mobs.begin()->first;
            selectedPartIdx = -1;
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    // Vertical splitter handle
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 1.0f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
    ImGui::Button("##mobListSplitter", ImVec2(4.0f, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemActive()) {
        m_listPanelWidth += ImGui::GetIO().MouseDelta.x;
        m_listPanelWidth = std::clamp(m_listPanelWidth, 120.0f, 400.0f);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    if (!mobs.count(selectedType)) { ImGui::End(); return; }

    // If editing a part in Block Designer, show banner and skip normal UI
    if (m_editingPartExternally) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.3f, 0.5f, 1.0f));
        ImGui::BeginChild("##ExtEditBanner", ImVec2(0, 0), true);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f),
            "  Part '%s' is being edited in Block Designer.",
            m_editingPartIdx >= 0 && m_editingPartIdx < (int)m_workingCopy.parts.size()
                ? m_workingCopy.parts[m_editingPartIdx].name.c_str() : "?");
        ImGui::Spacing();
        ImGui::TextDisabled("  Use the Block Designer window to edit textures/colors, then click 'Done & Return'.");
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    // Wrap all right-side content in a child to keep layout after SameLine
    ImGui::BeginChild("##MobRightSide", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    auto& def = m_workingCopy;

    // ===== TOOLBAR: Save / Discard / Undo / Redo =====
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
        ImGui::SameLine();

        if (m_dirty) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " * Unsaved Changes");
        }
    }
    ImGui::Separator();

    // Snapshot before edits for undo tracking
    MobDefinition preEditSnapshot = def;

    // ===== CENTER: Properties + Parts List =====
    ImGui::BeginChild("##MobCenterPanel", ImVec2(m_propsPanelWidth, 0), false);
    {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "PROPERTIES");
        ImGui::Separator();

        char nameBuf[64];
        strncpy(nameBuf, def.name.c_str(), 63); nameBuf[63] = '\0';
        if (ImGui::InputText("Mob Name", nameBuf, 64)) def.name = nameBuf;

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Stats");

        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::SliderFloat("Max Health", &def.maxHp, 1.0f, 200.0f, "%.0f HP");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.3f, 0.9f, 0.6f, 1.0f));
        ImGui::SliderFloat("Move Speed", &def.speed, 0.1f, 15.0f, "%.1f m/s");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Behavior");
        ImGui::Checkbox("Aquatic", &def.isAquatic);
        ImGui::SameLine();
        ImGui::Checkbox("Hostile", &def.isHostile);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Sound");
        char soundBuf[128];
        strncpy(soundBuf, def.ambientSound.c_str(), 127); soundBuf[127] = '\0';
        if (ImGui::InputText("Ambient Sound", soundBuf, 128)) def.ambientSound = soundBuf;
        ImGui::SameLine();
        if (ImGui::SmallButton("Browse##mobsnd")) {
            std::string p = openFileDialog();
            if (!p.empty()) def.ambientSound = p;
        }

        ImGui::Spacing();
        ImGui::Separator();

        // -- Selected Part Editor --
        if (selectedPartIdx >= 0 && selectedPartIdx < (int)def.parts.size()) {
            auto& part = def.parts[selectedPartIdx];
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "PART: %s", part.name.c_str());
            ImGui::Separator();

            char pName[64];
            strncpy(pName, part.name.c_str(), 63); pName[63] = '\0';
            if (ImGui::InputText("Part Name", pName, 64)) part.name = pName;

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.4f, 1.0f), "Transform");
            ImGui::DragFloat3("Offset##part", &part.offset.x, 0.02f, -10.0f, 10.0f, "%.2f");
            ImGui::DragFloat3("Size##part", &part.size.x, 0.02f, 0.01f, 10.0f, "%.2f");
            ImGui::DragFloat3("Pivot##part", &part.pivot.x, 0.02f, -5.0f, 5.0f, "%.2f");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.4f, 1.0f), "Appearance");
            ImGui::ColorEdit3("Part Color", &part.color.x, ImGuiColorEditFlags_Float);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.4f, 1.0f), "Texture");
            ImGui::Checkbox("Per-Face Textures", &part.usePerFace);

            if (!part.usePerFace) {
                ImGui::SliderInt("Texture X##part", &part.texX, 0, 15);
                ImGui::SliderInt("Texture Y##part", &part.texY, 0, 15);

                // --- Texture Atlas Picker (non per-face mode) ---
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.4f, 1.0f), "Texture Atlas");
                ImGui::TextDisabled("Click to pick a texture tile.");
                {
                    ImVec2 atlasSize(256, 256);
                    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                    ImGui::Image((ImTextureID)(uintptr_t)atlasID, atlasSize);
                    if (ImGui::IsItemClicked()) {
                        ImVec2 mouse = ImGui::GetMousePos();
                        int tx = (int)((mouse.x - cursorPos.x) / (atlasSize.x / 16.0f));
                        int ty = (int)((mouse.y - cursorPos.y) / (atlasSize.y / 16.0f));
                        tx = std::clamp(tx, 0, 15);
                        ty = std::clamp(ty, 0, 15);
                        part.texX = tx;
                        part.texY = ty;
                    }
                    // Grid overlay + highlight
                    ImDrawList* adl = ImGui::GetWindowDrawList();
                    float cellW = atlasSize.x / 16.0f;
                    float cellH = atlasSize.y / 16.0f;
                    for (int i = 0; i <= 16; i++) {
                        adl->AddLine(ImVec2(cursorPos.x + i * cellW, cursorPos.y),
                                     ImVec2(cursorPos.x + i * cellW, cursorPos.y + atlasSize.y), IM_COL32(255,255,255,40));
                        adl->AddLine(ImVec2(cursorPos.x, cursorPos.y + i * cellH),
                                     ImVec2(cursorPos.x + atlasSize.x, cursorPos.y + i * cellH), IM_COL32(255,255,255,40));
                    }
                    // Highlight selected cell
                    ImVec2 selMin(cursorPos.x + part.texX * cellW, cursorPos.y + part.texY * cellH);
                    ImVec2 selMax(selMin.x + cellW, selMin.y + cellH);
                    adl->AddRect(selMin, selMax, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                }
            } else {
                // --- 2D Unfolded Cube (cross pattern) with texture swatches ---
                ImGui::TextDisabled("Click a face to select/edit. Hover to highlight in 3D.");
                const float cellSize = 80.0f;
                const float gap = 3.0f;
                const float cellStep = cellSize + gap;
                const char* faceNames[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};

                // Layout:         [Top(2)]
                // [Left(1)] [Front(4)] [Right(0)] [Back(5)]
                //                 [Bottom(3)]
                struct FaceCell { int face; int col; int row; };
                FaceCell cells[] = {
                    {2, 1, 0},  // Top
                    {1, 0, 1},  // Left
                    {4, 1, 1},  // Front
                    {0, 2, 1},  // Right
                    {5, 3, 1},  // Back
                    {3, 1, 2},  // Bottom
                };

                m_hoveredFace = -1; // Reset each frame

                ImVec2 origin = ImGui::GetCursorScreenPos();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImTextureID texId = (ImTextureID)(uintptr_t)atlasID;
                float ts = 1.0f / 16.0f;

                for (int ci = 0; ci < 6; ci++) {
                    auto& c = cells[ci];
                    float x = origin.x + c.col * cellStep;
                    float y = origin.y + c.row * cellStep;
                    auto& fc = part.faces[c.face];

                    // Background tinted by face color
                    ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(ImVec4(fc.color.x * 0.4f, fc.color.y * 0.4f, fc.color.z * 0.4f, 1.0f));
                    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + cellSize, y + cellSize), bgCol);

                    // Texture swatch inside cell
                    ImVec2 uv0(fc.texX * ts, fc.texY * ts);
                    ImVec2 uv1((fc.texX + 1) * ts, (fc.texY + 1) * ts);
                    dl->AddImage(texId, ImVec2(x + 4, y + 4), ImVec2(x + cellSize - 4, y + cellSize - 20), uv0, uv1);

                    // Face label at bottom of cell
                    char cellLabel[32];
                    snprintf(cellLabel, sizeof(cellLabel), "%s [%d,%d]", faceNames[c.face], fc.texX, fc.texY);
                    ImVec2 textSize = ImGui::CalcTextSize(cellLabel);
                    dl->AddText(ImVec2(x + (cellSize - textSize.x) * 0.5f, y + cellSize - 16),
                                IM_COL32(255, 255, 255, 220), cellLabel);

                    // Selection / hover outline
                    if (m_selectedFace == c.face) {
                        dl->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + cellSize + 1, y + cellSize + 1),
                                    IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.5f);
                    } else {
                        dl->AddRect(ImVec2(x, y), ImVec2(x + cellSize, y + cellSize), IM_COL32(80, 80, 80, 200));
                    }
                }

                // Invisible buttons for click + hover detection
                for (int ci = 0; ci < 6; ci++) {
                    auto& c = cells[ci];
                    float x = origin.x + c.col * cellStep;
                    float y = origin.y + c.row * cellStep;
                    ImGui::SetCursorScreenPos(ImVec2(x, y));
                    ImGui::PushID(500 + c.face);
                    if (ImGui::InvisibleButton("##faceBtn", ImVec2(cellSize, cellSize))) {
                        m_selectedFace = (m_selectedFace == c.face) ? -1 : c.face;
                    }
                    if (ImGui::IsItemHovered()) {
                        m_hoveredFace = c.face;
                        // Draw hover highlight
                        dl->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + cellSize + 1, y + cellSize + 1),
                                    IM_COL32(100, 200, 255, 200), 0.0f, 0, 2.0f);
                    }
                    ImGui::PopID();
                }

                // Advance cursor past the grid
                ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + 3 * cellStep + gap));
                ImGui::Spacing();

                // --- Face Editor ---
                if (m_selectedFace >= 0 && m_selectedFace < 6) {
                    auto& selFace = part.faces[m_selectedFace];
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Face: %s", faceNames[m_selectedFace]);
                    ImGui::Separator();
                    ImGui::PushID(600 + m_selectedFace);
                    ImGui::ColorEdit3("Face Color", &selFace.color.x, ImGuiColorEditFlags_Float);
                    ImGui::SliderInt("Tex X##face", &selFace.texX, 0, 15);
                    ImGui::SliderInt("Tex Y##face", &selFace.texY, 0, 15);
                    if (ImGui::Button("Copy to All Faces", ImVec2(-1, 24))) {
                        for (int f = 0; f < 6; f++) {
                            part.faces[f] = selFace;
                        }
                    }
                    ImGui::PopID();
                    ImGui::Spacing();
                }

                // --- Mirror / Symmetry Tools ---
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.4f, 1.0f), "Symmetry Tools");
                if (ImGui::Button("Mirror X")) { std::swap(part.faces[0], part.faces[1]); }
                ImGui::SameLine();
                if (ImGui::Button("Mirror Y")) { std::swap(part.faces[2], part.faces[3]); }
                ImGui::SameLine();
                if (ImGui::Button("Mirror Z")) { std::swap(part.faces[4], part.faces[5]); }

                // --- Gradient Color Fill ---
                if (ImGui::Button("Gradient Fill (Top->Bottom)")) {
                    Vec3 top = part.faces[2].color;    // +Y
                    Vec3 bottom = part.faces[3].color; // -Y
                    Vec3 mid = {(top.x+bottom.x)*0.5f, (top.y+bottom.y)*0.5f, (top.z+bottom.z)*0.5f};
                    part.faces[0].color = mid;
                    part.faces[1].color = mid;
                    part.faces[4].color = mid;
                    part.faces[5].color = mid;
                }

                // --- Copy Face From Another Part ---
                if (m_selectedFace >= 0 && def.parts.size() > 1) {
                    ImGui::SameLine();
                    if (ImGui::Button("Paste Face From...")) ImGui::OpenPopup("##PasteFace");
                    if (ImGui::BeginPopup("##PasteFace")) {
                        for (int pi = 0; pi < (int)def.parts.size(); pi++) {
                            if (pi == selectedPartIdx) continue;
                            if (ImGui::BeginMenu(def.parts[pi].name.c_str())) {
                                for (int fi = 0; fi < 6; fi++) {
                                    if (ImGui::MenuItem(faceNames[fi])) {
                                        part.faces[m_selectedFace] = def.parts[pi].faces[fi];
                                    }
                                }
                                ImGui::EndMenu();
                            }
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::Spacing();

                // --- Texture Atlas Picker ---
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.4f, 1.0f), "Texture Atlas");
                ImGui::TextDisabled("Click to set texture for %s.", m_selectedFace >= 0 ? faceNames[m_selectedFace] : "selected face");
                {
                    ImVec2 atlasSize(256, 256);
                    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                    ImGui::Image((ImTextureID)(uintptr_t)atlasID, atlasSize);
                    if (ImGui::IsItemClicked()) {
                        ImVec2 mouse = ImGui::GetMousePos();
                        int tx = (int)((mouse.x - cursorPos.x) / (atlasSize.x / 16.0f));
                        int ty = (int)((mouse.y - cursorPos.y) / (atlasSize.y / 16.0f));
                        tx = std::clamp(tx, 0, 15);
                        ty = std::clamp(ty, 0, 15);
                        if (m_selectedFace >= 0) {
                            part.faces[m_selectedFace].texX = tx;
                            part.faces[m_selectedFace].texY = ty;
                        }
                    }
                    // Grid overlay
                    ImDrawList* adl = ImGui::GetWindowDrawList();
                    float cellW = atlasSize.x / 16.0f;
                    float cellH = atlasSize.y / 16.0f;
                    for (int i = 0; i <= 16; i++) {
                        adl->AddLine(ImVec2(cursorPos.x + i * cellW, cursorPos.y),
                                     ImVec2(cursorPos.x + i * cellW, cursorPos.y + atlasSize.y), IM_COL32(255,255,255,40));
                        adl->AddLine(ImVec2(cursorPos.x, cursorPos.y + i * cellH),
                                     ImVec2(cursorPos.x + atlasSize.x, cursorPos.y + i * cellH), IM_COL32(255,255,255,40));
                    }
                    // Highlight current face's texture cell
                    if (m_selectedFace >= 0) {
                        auto& sf = part.faces[m_selectedFace];
                        ImVec2 selMin(cursorPos.x + sf.texX * cellW, cursorPos.y + sf.texY * cellH);
                        ImVec2 selMax(selMin.x + cellW, selMin.y + cellH);
                        adl->AddRect(selMin, selMax, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                    }
                }
            }

            // --- Mini 3D Part Preview ---
            if (partPreviewBuf && previewShader) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.4f, 1.0f), "Part Preview");
                renderPartPreview(part, atlasID, partPreviewBuf, previewShader);
                float pvSide = 180.0f;
                ImGui::Image((ImTextureID)(uintptr_t)getPreviewTexture(partPreviewBuf),
                             ImVec2(pvSide, pvSide), ImVec2(0, 1), ImVec2(1, 0));
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            if (ImGui::Button("Edit in Block Designer", ImVec2(-1, 30))) {
                m_editingPartIdx = selectedPartIdx;
                m_editingPartExternally = true;
                m_wantsBlockDesigner = true;
            }
            ImGui::PopStyleColor();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.4f, 1.0f), "Animation");
            ImGui::Checkbox("Walk/Leg Animation", &part.affectedByLegAnim);
            ImGui::Checkbox("Head Bob Animation", &part.affectedByHeadAnim);

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.5f, 1.0f));
            if (selectedPartIdx > 0 && ImGui::Button("Move Up")) {
                std::swap(def.parts[selectedPartIdx], def.parts[selectedPartIdx - 1]);
                selectedPartIdx--;
            }
            ImGui::SameLine();
            if (selectedPartIdx < (int)def.parts.size() - 1 && ImGui::Button("Move Down")) {
                std::swap(def.parts[selectedPartIdx], def.parts[selectedPartIdx + 1]);
                selectedPartIdx++;
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button("Delete Part")) {
                def.parts.erase(def.parts.begin() + selectedPartIdx);
                if (selectedPartIdx >= (int)def.parts.size()) selectedPartIdx = (int)def.parts.size() - 1;
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::TextDisabled("Select a part from the list to edit it.");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // -- Parts List --
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "MODEL PARTS (%zu)", def.parts.size());

        if (ImGui::Button("+ Add Part", ImVec2(120, 24))) {
            MobPart part;
            part.name = "Part " + std::to_string(def.parts.size());
            part.offset = {0, 0.5f, 0};
            part.size = {0.5f, 0.5f, 0.5f};
            part.pivot = {0.25f, 0.25f, 0.25f};
            part.color = {0.8f, 0.8f, 0.8f};
            def.parts.push_back(part);
            selectedPartIdx = (int)def.parts.size() - 1;
        }
        ImGui::SameLine();
        if (selectedPartIdx >= 0 && selectedPartIdx < (int)def.parts.size()) {
            if (ImGui::Button("Duplicate", ImVec2(80, 24))) {
                MobPart copy = def.parts[selectedPartIdx];
                copy.name += " Copy";
                copy.offset.x += 0.5f;
                def.parts.push_back(copy);
                selectedPartIdx = (int)def.parts.size() - 1;
            }
        }

        ImGui::BeginChild("##PartsList", ImVec2(0, 0), true);
        for (int i = 0; i < (int)def.parts.size(); i++) {
            ImGui::PushID(300 + i);
            auto& part = def.parts[i];
            bool isSel = (selectedPartIdx == i);

            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(part.color.x * 0.4f, part.color.y * 0.4f, part.color.z * 0.4f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(part.color.x * 0.6f, part.color.y * 0.6f, part.color.z * 0.6f, 0.9f));

            char partLabel[128];
            const char* tagLeg = part.affectedByLegAnim ? " [Leg]" : "";
            const char* tagHead = part.affectedByHeadAnim ? " [Head]" : "";
            snprintf(partLabel, sizeof(partLabel), "%s%s%s  (%.1f x %.1f x %.1f)",
                     part.name.c_str(), tagLeg, tagHead, part.size.x, part.size.y, part.size.z);

            if (ImGui::Selectable(partLabel, isSel)) {
                selectedPartIdx = i;
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    // Vertical splitter between properties and 3D preview
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 1.0f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
    ImGui::Button("##mobPropsSplitter", ImVec2(4.0f, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemActive()) {
        m_propsPanelWidth += ImGui::GetIO().MouseDelta.x;
        m_propsPanelWidth = std::clamp(m_propsPanelWidth, 250.0f, 700.0f);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // ===== RIGHT PANEL: Interactive 3D Preview =====
    ImGui::BeginChild("##Mob3DPanel", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "3D PREVIEW");
        ImGui::Separator();

        // View controls (above preview so they are always visible)
        if (ImGui::Button("Reset View")) { m_yaw = 0.6f; m_pitch = 0.35f; m_dist = 5.0f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Front")) { m_yaw = 0.0f; m_pitch = 0.0f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Top")) { m_yaw = 0.0f; m_pitch = 1.3f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Side")) { m_yaw = 1.5708f; m_pitch = 0.0f; m_panX = 0.0f; m_panY = 0.0f; }
        ImGui::TextDisabled("Right Mouse Button (hold): Rotate view");
        ImGui::TextDisabled("Scroll Wheel: Zoom in (toward you) / out (away)");
        ImGui::TextDisabled("Middle Mouse Button (hold): Pan view");

        m_selectedPartForPreview = selectedPartIdx;
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
        static bool s_mobOrbit = false;
        static bool s_mobPan = false;
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                s_mobOrbit = true;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                s_mobPan = true;
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll != 0.0f) {
                m_dist += scroll * 0.4f;
                m_dist  = std::clamp(m_dist, 2.0f, 15.0f);
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
            s_mobOrbit = false;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            s_mobPan = false;
        if (s_mobOrbit) {
            ImGuiIO& pio = ImGui::GetIO();
            m_yaw   -= pio.MouseDelta.x * 0.01f;
            m_pitch += pio.MouseDelta.y * 0.01f;
            m_pitch  = std::clamp(m_pitch, -1.4f, 1.4f);
        }
        if (s_mobPan) {
            ImGuiIO& pio = ImGui::GetIO();
            m_panX -= pio.MouseDelta.x * 0.005f * m_dist;
            m_panY += pio.MouseDelta.y * 0.005f * m_dist;
        }
    }
    ImGui::EndChild();

    // --- Detect edits for undo tracking ---
    {
        bool changed = false;
        if (def.name != preEditSnapshot.name) changed = true;
        if (def.maxHp != preEditSnapshot.maxHp) changed = true;
        if (def.speed != preEditSnapshot.speed) changed = true;
        if (def.isAquatic != preEditSnapshot.isAquatic) changed = true;
        if (def.isHostile != preEditSnapshot.isHostile) changed = true;
        if (def.ambientSound != preEditSnapshot.ambientSound) changed = true;
        if (def.parts.size() != preEditSnapshot.parts.size()) changed = true;
        if (!changed) {
            for (size_t i = 0; i < def.parts.size() && i < preEditSnapshot.parts.size(); i++) {
                auto& a = def.parts[i]; auto& b = preEditSnapshot.parts[i];
                if (a.name != b.name || a.offset.x != b.offset.x || a.offset.y != b.offset.y ||
                    a.offset.z != b.offset.z || a.size.x != b.size.x || a.size.y != b.size.y ||
                    a.size.z != b.size.z || a.color.x != b.color.x || a.color.y != b.color.y ||
                    a.color.z != b.color.z || a.affectedByLegAnim != b.affectedByLegAnim ||
                    a.affectedByHeadAnim != b.affectedByHeadAnim ||
                    a.usePerFace != b.usePerFace || a.texX != b.texX || a.texY != b.texY) { changed = true; break; }
                if (!changed && a.usePerFace) {
                    for (int f = 0; f < 6; f++) {
                        if (a.faces[f].texX != b.faces[f].texX || a.faces[f].texY != b.faces[f].texY ||
                            a.faces[f].color.x != b.faces[f].color.x || a.faces[f].color.y != b.faces[f].color.y ||
                            a.faces[f].color.z != b.faces[f].color.z) { changed = true; break; }
                    }
                }
            }
        }
        if (changed) {
            m_undoStack.push_back(preEditSnapshot);
            if ((int)m_undoStack.size() > kMaxUndoSteps) m_undoStack.erase(m_undoStack.begin());
            m_redoStack.clear();
            m_dirty = true;
            saveBackup();
        }
    }

    // --- Save prompt popup ---
    if (m_showSavePrompt) {
        ImGui::OpenPopup("Save Changes?##Mob");
        m_showSavePrompt = false;
    }
    if (ImGui::BeginPopupModal("Save Changes?##Mob", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("You have unsaved changes in Mob Designer.");
        ImGui::Text("Do you want to save before closing?");
        ImGui::Spacing();
        if (ImGui::Button("Save", ImVec2(100, 30))) {
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

    // ===== AI BEHAVIOR EDITOR =====
    ImGui::Separator();
    if (ImGui::CollapsingHeader("AI Behavior Graph", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##AIBehaviorPanel", ImVec2(0, 450), true);
        m_aiEditor.show(def.aiGraph);
        ImGui::EndChild();
    }

    ImGui::EndChild(); // end ##MobRightSide
    ImGui::End();
}

void MobDesigner::renderPreview(const MobDefinition& def, unsigned int atlasID, Framebuffer* buf, Shader* shdr) {
    if (!buf || !shdr) return;

    // Save GL state
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

    float cx = 0.0f, cy = 1.0f, cz = 0.0f;
    // Auto-center camera on mob bounding box
    if (!def.parts.empty()) {
        float bminX = 1e9f, bminY = 1e9f, bminZ = 1e9f;
        float bmaxX = -1e9f, bmaxY = -1e9f, bmaxZ = -1e9f;
        for (const auto& part : def.parts) {
            bminX = std::min(bminX, part.offset.x);
            bminY = std::min(bminY, part.offset.y);
            bminZ = std::min(bminZ, part.offset.z);
            bmaxX = std::max(bmaxX, part.offset.x + part.size.x);
            bmaxY = std::max(bmaxY, part.offset.y + part.size.y);
            bmaxZ = std::max(bmaxZ, part.offset.z + part.size.z);
        }
        cx = (bminX + bmaxX) * 0.5f;
        cy = (bminY + bmaxY) * 0.5f;
        cz = (bminZ + bmaxZ) * 0.5f;
    }
    float camX = cx + m_dist * cosf(m_pitch) * sinf(m_yaw);
    float camY = cy + m_dist * sinf(m_pitch);
    float camZ = cz + m_dist * cosf(m_pitch) * cosf(m_yaw);
    // Apply pan offset in camera-local right/up directions
    Vec3 fwd = normalize(Vec3{cx - camX, cy - camY, cz - camZ});
    Vec3 right = normalize(cross(fwd, {0, 1, 0}));
    Vec3 up2 = cross(right, fwd);
    camX += right.x * m_panX + up2.x * m_panY;
    camY += right.y * m_panX + up2.y * m_panY;
    camZ += right.z * m_panX + up2.z * m_panY;
    cx   += right.x * m_panX + up2.x * m_panY;
    cy   += right.y * m_panX + up2.y * m_panY;
    cz   += right.z * m_panX + up2.z * m_panY;
    Mat4 view = lookAt({camX, camY, camZ}, {cx, cy, cz}, {0, 1, 0});
    float aspect = (float)pw / (float)ph;
    Mat4 proj = perspective(40.0f * (3.14159f / 180.0f), aspect, 0.1f, 40.0f);
    Mat4 model = Mat4::identity();

    shdr->setMat4("uProjection", proj);
    shdr->setMat4("uView", view);
    shdr->setMat4("uModel", model);
    shdr->setVec3("uLightDir", {-0.4f, -0.8f, -0.5f});
    shdr->setVec3("uColorTint", {1, 1, 1});
    shdr->setInt("uTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasID);

    // --- Floor grid ---
    {
        MeshBuilder gridMb;
        float gridY = -0.02f;
        float gridExtent = 4.0f;
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

    // --- Draw mob parts (per-part transform, per-part textures) ---
    {
        for (int partIdx = 0; partIdx < (int)def.parts.size(); partIdx++) {
            const auto& part = def.parts[partIdx];

            // Build a textured unit cube for this part
            MeshBuilder partMb;
            float s = 1.0f / 16.0f;

            auto addPartFace = [&](int fi, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n) {
                int tx = part.texX, ty = part.texY;
                Vec3 fc = part.color;
                if (part.usePerFace) {
                    tx = part.faces[fi].texX;
                    ty = part.faces[fi].texY;
                    fc = part.faces[fi].color;
                }
                float u1 = tx * s, v1 = ty * s;
                partMb.addFace(p1, p2, p3, p4, n, u1, v1, u1+s, v1+s, fc.x, fc.y, fc.z);
            };

            addPartFace(0, {1,0,0},{1,0,1},{1,1,1},{1,1,0}, {1,0,0});
            addPartFace(1, {0,0,1},{0,0,0},{0,1,0},{0,1,1}, {-1,0,0});
            addPartFace(2, {0,1,0},{1,1,0},{1,1,1},{0,1,1}, {0,1,0});
            addPartFace(3, {0,0,1},{1,0,1},{1,0,0},{0,0,0}, {0,-1,0});
            addPartFace(4, {0,0,1},{1,0,1},{1,1,1},{0,1,1}, {0,0,1});
            addPartFace(5, {1,0,0},{0,0,0},{0,1,0},{1,1,0}, {0,0,-1});

            GLMesh partMesh;
            partMesh.upload(partMb.getVertices());

            // Static preview: no animation, so pivot cancels out -> just offset + scale
            Mat4 p = translate(part.offset) * scale(part.size);
            shdr->setMat4("uModel", p);
            shdr->setVec3("uColorTint", {1, 1, 1});
            partMesh.draw();

            // Highlight selected part with yellow wireframe outline
            if (partIdx == m_selectedPartForPreview) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(3.0f);
                glDisable(GL_DEPTH_TEST);
                shdr->setVec3("uColorTint", {1.0f, 1.0f, 0.3f});
                partMesh.draw();
                glEnable(GL_DEPTH_TEST);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            partMesh.destroy();
        }

        shdr->setVec3("uColorTint", {1, 1, 1});
        shdr->setMat4("uModel", Mat4::identity());
    }

    // Restore GL state
    buf->resolve();
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glDepthFunc(GL_LEQUAL); // Restore main renderer's depth func
}

unsigned int MobDesigner::getPreviewTexture(Framebuffer* buf) {
    return buf ? buf->getTexture() : 0;
}

void MobDesigner::renderPartPreview(const MobPart& part, unsigned int atlasID, Framebuffer* buf, Shader* shdr) {
    if (!buf || !shdr) return;

    GLint prevFBO = 0, prevViewport[4] = {};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    buf->bind();
    int pw = buf->getWidth(), ph = buf->getHeight();
    glViewport(0, 0, pw, ph);
    glClearColor(0.12f, 0.12f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    shdr->use();

    // Tight camera centered on a unit cube
    float cx = 0.5f, cy = 0.5f, cz = 0.5f;
    float dist = 2.5f;
    float yaw = 0.6f, pitch = 0.35f;
    float camX = cx + dist * cosf(pitch) * sinf(yaw);
    float camY = cy + dist * sinf(pitch);
    float camZ = cz + dist * cosf(pitch) * cosf(yaw);
    Mat4 view = lookAt({camX, camY, camZ}, {cx, cy, cz}, {0, 1, 0});
    float aspect = (float)pw / (float)ph;
    Mat4 proj = perspective(40.0f * (3.14159f / 180.0f), aspect, 0.1f, 20.0f);

    shdr->setMat4("uProjection", proj);
    shdr->setMat4("uView", view);
    shdr->setMat4("uModel", Mat4::identity());
    shdr->setVec3("uLightDir", {-0.4f, -0.8f, -0.5f});
    shdr->setVec3("uColorTint", {1, 1, 1});
    shdr->setInt("uTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasID);

    // Build textured unit cube for this part
    MeshBuilder partMb;
    float s = 1.0f / 16.0f;
    auto addFace = [&](int fi, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n) {
        int tx = part.texX, ty = part.texY;
        Vec3 fc = part.color;
        if (part.usePerFace) {
            tx = part.faces[fi].texX;
            ty = part.faces[fi].texY;
            fc = part.faces[fi].color;
        }
        float u1 = tx * s, v1 = ty * s;
        partMb.addFace(p1, p2, p3, p4, n, u1, v1, u1 + s, v1 + s, fc.x, fc.y, fc.z);
    };
    addFace(0, {1,0,0},{1,0,1},{1,1,1},{1,1,0}, {1,0,0});
    addFace(1, {0,0,1},{0,0,0},{0,1,0},{0,1,1}, {-1,0,0});
    addFace(2, {0,1,0},{1,1,0},{1,1,1},{0,1,1}, {0,1,0});
    addFace(3, {0,0,1},{1,0,1},{1,0,0},{0,0,0}, {0,-1,0});
    addFace(4, {0,0,1},{1,0,1},{1,1,1},{0,1,1}, {0,0,1});
    addFace(5, {1,0,0},{0,0,0},{0,1,0},{1,1,0}, {0,0,-1});

    GLMesh mesh;
    mesh.upload(partMb.getVertices());
    mesh.draw();

    // Highlight hovered face with wireframe outline
    if (m_hoveredFace >= 0 && m_hoveredFace < 6 && part.usePerFace) {
        MeshBuilder hlMb;
        float hs = 1.0f / 16.0f;
        int htx = part.faces[m_hoveredFace].texX;
        int hty = part.faces[m_hoveredFace].texY;
        float hu1 = htx * hs, hv1 = hty * hs;

        // Build a single-face mesh for the hovered face
        auto addHlFace = [&](Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n) {
            hlMb.addFace(p1, p2, p3, p4, n, hu1, hv1, hu1 + hs, hv1 + hs, 1.0f, 1.0f, 1.0f);
        };
        switch (m_hoveredFace) {
            case 0: addHlFace({1,0,0},{1,0,1},{1,1,1},{1,1,0}, {1,0,0}); break;
            case 1: addHlFace({0,0,1},{0,0,0},{0,1,0},{0,1,1}, {-1,0,0}); break;
            case 2: addHlFace({0,1,0},{1,1,0},{1,1,1},{0,1,1}, {0,1,0}); break;
            case 3: addHlFace({0,0,1},{1,0,1},{1,0,0},{0,0,0}, {0,-1,0}); break;
            case 4: addHlFace({0,0,1},{1,0,1},{1,1,1},{0,1,1}, {0,0,1}); break;
            case 5: addHlFace({1,0,0},{0,0,0},{0,1,0},{1,1,0}, {0,0,-1}); break;
        }
        GLMesh hlMesh;
        hlMesh.upload(hlMb.getVertices());
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);
        glDisable(GL_DEPTH_TEST);
        shdr->setVec3("uColorTint", {0.4f, 0.8f, 1.0f});
        hlMesh.draw();
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        shdr->setVec3("uColorTint", {1, 1, 1});
        hlMesh.destroy();
    }

    // Highlight selected face with a brighter wireframe
    if (m_selectedFace >= 0 && m_selectedFace < 6 && part.usePerFace) {
        MeshBuilder selMb;
        float ss = 1.0f / 16.0f;
        int stx = part.faces[m_selectedFace].texX;
        int sty = part.faces[m_selectedFace].texY;
        float su1 = stx * ss, sv1 = sty * ss;

        auto addSelFace = [&](Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 n) {
            selMb.addFace(p1, p2, p3, p4, n, su1, sv1, su1 + ss, sv1 + ss, 1.0f, 1.0f, 1.0f);
        };
        switch (m_selectedFace) {
            case 0: addSelFace({1,0,0},{1,0,1},{1,1,1},{1,1,0}, {1,0,0}); break;
            case 1: addSelFace({0,0,1},{0,0,0},{0,1,0},{0,1,1}, {-1,0,0}); break;
            case 2: addSelFace({0,1,0},{1,1,0},{1,1,1},{0,1,1}, {0,1,0}); break;
            case 3: addSelFace({0,0,1},{1,0,1},{1,0,0},{0,0,0}, {0,-1,0}); break;
            case 4: addSelFace({0,0,1},{1,0,1},{1,1,1},{0,1,1}, {0,0,1}); break;
            case 5: addSelFace({1,0,0},{0,0,0},{0,1,0},{1,1,0}, {0,0,-1}); break;
        }
        GLMesh selMesh;
        selMesh.upload(selMb.getVertices());
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(3.0f);
        glDisable(GL_DEPTH_TEST);
        shdr->setVec3("uColorTint", {1.0f, 1.0f, 0.3f});
        selMesh.draw();
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        shdr->setVec3("uColorTint", {1, 1, 1});
        selMesh.destroy();
    }

    mesh.destroy();

    buf->resolve();
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glDepthFunc(GL_LEQUAL);
}

// --- Save / Undo / Redo Implementation ---

void MobDesigner::loadWorkingCopy(MobType type) {
    auto& mobs = GameRegistry::getInstance().getAllMobs();
    if (mobs.count(type)) {
        m_workingCopy = mobs[type];
    } else {
        m_workingCopy = {};
        m_workingCopy.type = type;
    }
    m_workingType = type;
    m_dirty = false;
    m_undoStack.clear();
    m_redoStack.clear();
}

void MobDesigner::pushUndo() {
    m_undoStack.push_back(m_workingCopy);
    if ((int)m_undoStack.size() > kMaxUndoSteps)
        m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();
    m_dirty = true;
}

void MobDesigner::undo() {
    if (m_undoStack.empty()) return;
    m_redoStack.push_back(m_workingCopy);
    m_workingCopy = m_undoStack.back();
    m_undoStack.pop_back();
    m_dirty = !m_undoStack.empty();
    saveBackup();
}

void MobDesigner::redo() {
    if (m_redoStack.empty()) return;
    m_undoStack.push_back(m_workingCopy);
    m_workingCopy = m_redoStack.back();
    m_redoStack.pop_back();
    m_dirty = true;
    saveBackup();
}

void MobDesigner::saveToRegistry() {
    auto& mobs = GameRegistry::getInstance().getAllMobs();
    mobs[m_workingType] = m_workingCopy;
    m_dirty = false;
    m_undoStack.clear();
    m_redoStack.clear();
    clearBackup();
}

void MobDesigner::discardChanges() {
    loadWorkingCopy(m_workingType);
    clearBackup();
}

void MobDesigner::saveBackup() const {
    std::ofstream out(kBackupFile, std::ios::binary);
    if (!out) return;
    int typeInt = (int)m_workingType;
    out.write((const char*)&typeInt, sizeof(typeInt));
    uint32_t nameLen = (uint32_t)m_workingCopy.name.size();
    out.write((const char*)&nameLen, sizeof(nameLen));
    out.write(m_workingCopy.name.data(), nameLen);
    out.write((const char*)&m_workingCopy.maxHp, sizeof(float));
    out.write((const char*)&m_workingCopy.speed, sizeof(float));
    out.write((const char*)&m_workingCopy.isAquatic, sizeof(bool));
    out.write((const char*)&m_workingCopy.isHostile, sizeof(bool));
    uint32_t partCount = (uint32_t)m_workingCopy.parts.size();
    out.write((const char*)&partCount, sizeof(partCount));
    for (const auto& p : m_workingCopy.parts) {
        uint32_t pNameLen = (uint32_t)p.name.size();
        out.write((const char*)&pNameLen, sizeof(pNameLen));
        out.write(p.name.data(), pNameLen);
        out.write((const char*)&p.offset, sizeof(Vec3));
        out.write((const char*)&p.size, sizeof(Vec3));
        out.write((const char*)&p.pivot, sizeof(Vec3));
        out.write((const char*)&p.color, sizeof(Vec3));
        out.write((const char*)&p.affectedByLegAnim, sizeof(bool));
        out.write((const char*)&p.affectedByHeadAnim, sizeof(bool));
        out.write((const char*)&p.usePerFace, sizeof(bool));
        out.write((const char*)&p.texX, sizeof(int));
        out.write((const char*)&p.texY, sizeof(int));
        for (int f = 0; f < 6; f++) {
            out.write((const char*)&p.faces[f].texX, sizeof(int));
            out.write((const char*)&p.faces[f].texY, sizeof(int));
            out.write((const char*)&p.faces[f].color, sizeof(Vec3));
        }
    }
}

bool MobDesigner::loadBackup() {
    std::ifstream in(kBackupFile, std::ios::binary);
    if (!in) return false;

    int typeInt;
    in.read((char*)&typeInt, sizeof(typeInt));
    if (!in) return false;

    uint32_t nameLen;
    in.read((char*)&nameLen, sizeof(nameLen));
    if (!in || nameLen > 256) return false;

    m_workingType = (MobType)typeInt;
    m_workingCopy.type = m_workingType;
    m_workingCopy.name.resize(nameLen);
    in.read(m_workingCopy.name.data(), nameLen);
    in.read((char*)&m_workingCopy.maxHp, sizeof(float));
    in.read((char*)&m_workingCopy.speed, sizeof(float));
    in.read((char*)&m_workingCopy.isAquatic, sizeof(bool));
    in.read((char*)&m_workingCopy.isHostile, sizeof(bool));
    uint32_t partCount;
    in.read((char*)&partCount, sizeof(partCount));
    if (!in || partCount > 100) return false;

    m_workingCopy.parts.resize(partCount);
    for (uint32_t i = 0; i < partCount; i++) {
        auto& p = m_workingCopy.parts[i];
        uint32_t pNameLen;
        in.read((char*)&pNameLen, sizeof(pNameLen));
        if (!in || pNameLen > 128) return false;
        p.name.resize(pNameLen);
        in.read(p.name.data(), pNameLen);
        in.read((char*)&p.offset, sizeof(Vec3));
        in.read((char*)&p.size, sizeof(Vec3));
        in.read((char*)&p.pivot, sizeof(Vec3));
        in.read((char*)&p.color, sizeof(Vec3));
        in.read((char*)&p.affectedByLegAnim, sizeof(bool));
        in.read((char*)&p.affectedByHeadAnim, sizeof(bool));
        in.read((char*)&p.usePerFace, sizeof(bool));
        in.read((char*)&p.texX, sizeof(int));
        in.read((char*)&p.texY, sizeof(int));
        for (int f = 0; f < 6; f++) {
            in.read((char*)&p.faces[f].texX, sizeof(int));
            in.read((char*)&p.faces[f].texY, sizeof(int));
            in.read((char*)&p.faces[f].color, sizeof(Vec3));
        }
    }
    return in.good();
}

void MobDesigner::clearBackup() const {
    std::remove(kBackupFile);
}

BlockDefinition MobDesigner::getPartAsBlock(int partIdx) const {
    BlockDefinition block{};
    if (partIdx < 0 || partIdx >= (int)m_workingCopy.parts.size()) return block;
    const auto& part = m_workingCopy.parts[partIdx];
    block.id = 255; // Temporary ID for external editing
    block.name = part.name;
    block.usePerFace = part.usePerFace;
    block.color = part.color;
    block.texX = part.texX;
    block.texY = part.texY;
    for (int i = 0; i < 6; i++) {
        block.faces[i] = part.faces[i];
    }
    return block;
}

void MobDesigner::applyBlockToPart(int partIdx, const BlockDefinition& block) {
    if (partIdx < 0 || partIdx >= (int)m_workingCopy.parts.size()) return;
    pushUndo();
    auto& part = m_workingCopy.parts[partIdx];
    part.name = block.name;
    part.usePerFace = block.usePerFace;
    part.color = block.color;
    part.texX = block.texX;
    part.texY = block.texY;
    for (int i = 0; i < 6; i++) {
        part.faces[i] = block.faces[i];
    }
    m_dirty = true;
    saveBackup();
}
