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
    ImGui::SetNextWindowSize(ImVec2(1050, 700), ImGuiCond_FirstUseEver);

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

    // Initialize working copy on first use or when selection changes
    if (!m_initialized || m_workingId != selectedId) {
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

    // --- Ctrl+Z / Ctrl+Y ---
    auto& io = ImGui::GetIO();
    bool ctrl = io.KeyCtrl;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undo();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) redo();

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

    // ===== FAR LEFT: Block List =====
    ImGui::BeginChild("##BlockListPanel", ImVec2(160, 0), true);
    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "BLOCKS");
    ImGui::Separator();

    for (auto& [id, def] : blocks) {
        if (id == 0) continue;
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
    ImGui::EndChild();

    ImGui::SameLine();

    if (!blocks.count(selectedId)) { ImGui::End(); return; }

    // Wrap all right-side content in a child to keep layout after SameLine
    ImGui::BeginChild("##BlockRightSide", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    auto& def = m_workingCopy;

    // ===== TOOLBAR: Save / Discard / Undo / Redo =====
    {
        ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Save", ImVec2(70, 28)) && m_dirty) {
            saveToRegistry();
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.7f, 0.3f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Discard", ImVec2(70, 28)) && m_dirty) {
            discardChanges();
        }
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

    // Snapshot before any edit for undo tracking
    BlockDefinition preEditSnapshot = def;

    // ===== LEFT-CENTER: 2D Unfolded Cube + Properties =====
    ImGui::BeginChild("##Block2DPanel", ImVec2(440, 0), false);
    {
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "PROPERTIES");
        ImGui::Separator();
        char nameBuf[64];
        strncpy(nameBuf, def.name.c_str(), 63); nameBuf[63] = '\0';
        if (ImGui::InputText("Name", nameBuf, 64)) def.name = nameBuf;
        ImGui::Text("Block ID: %d", def.id);
        ImGui::Checkbox("Per-Face Mode", &def.usePerFace);
        ImGui::SameLine(); ImGui::Checkbox("Transparent", &def.isTransparent);
        ImGui::SameLine(); ImGui::Checkbox("Liquid", &def.isLiquid);

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

            ImU32 bgCol = isSel
                ? IM_COL32((int)(fc.x * 200), (int)(fc.y * 200), (int)(fc.z * 200), 220)
                : IM_COL32((int)(fc.x * 80), (int)(fc.y * 80), (int)(fc.z * 80), 180);
            drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + cellW, y + cellH), bgCol, 4.0f);

            ImU32 borderCol = isSel
                ? IM_COL32(255, 255, 255, 255)
                : IM_COL32((int)(fc.x * 255), (int)(fc.y * 255), (int)(fc.z * 255), 160);
            drawList->AddRect(ImVec2(x, y), ImVec2(x + cellW, y + cellH), borderCol, 4.0f, 0, isSel ? 3.0f : 1.5f);

            char txt[64];
            snprintf(txt, sizeof(txt), "%s", faceShort[fi]);
            ImVec2 textSize = ImGui::CalcTextSize(txt);
            drawList->AddText(ImVec2(x + (cellW - textSize.x) * 0.5f, y + 6), IM_COL32(255, 255, 255, 255), txt);

            char coordTxt[32];
            snprintf(coordTxt, sizeof(coordTxt), "[%d, %d]", def.faces[fi].texX, def.faces[fi].texY);
            ImVec2 coordSize = ImGui::CalcTextSize(coordTxt);
            drawList->AddText(ImVec2(x + (cellW - coordSize.x) * 0.5f, y + 24), IM_COL32(200, 200, 200, 200), coordTxt);

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
                ImGui::SliderInt("Texture X", &face.texX, 0, 15);
                ImGui::SliderInt("Texture Y", &face.texY, 0, 15);
                ImGui::Spacing();
                if (ImGui::Button("Copy to All Faces")) {
                    for (int j = 0; j < 6; j++) {
                        if (j != selectedFace) def.faces[j] = face;
                    }
                }
            } else {
                ImGui::TextDisabled("Click a face on the unfolded cube to edit it.");
            }
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Global Settings");
            ImGui::ColorEdit3("Color", &def.color.x, ImGuiColorEditFlags_Float);
            ImGui::SliderInt("Texture X", &def.texX, 0, 15);
            ImGui::SliderInt("Texture Y", &def.texY, 0, 15);
            if (ImGui::Button("Apply to All Faces")) {
                for (int i = 0; i < 6; i++) {
                    def.faces[i].color = def.color;
                    def.faces[i].texX = def.texX;
                    def.faces[i].texY = def.texY;
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
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
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // ===== RIGHT: Interactive 3D Preview =====
    ImGui::BeginChild("##Block3DPanel", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "3D PREVIEW");
        ImGui::Separator();

        // View controls (above preview so they are always visible)
        if (ImGui::Button("Reset View")) { m_yaw = 0.78f; m_pitch = 0.4f; m_dist = 3.5f; }
        ImGui::SameLine();
        if (ImGui::Button("Front")) { m_yaw = 0.0f; m_pitch = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button("Top")) { m_yaw = 0.0f; m_pitch = 1.3f; }
        ImGui::SameLine();
        if (ImGui::Button("Side")) { m_yaw = 1.5708f; m_pitch = 0.0f; }
        ImGui::TextDisabled("LMB drag: rotate | Scroll: zoom");

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
        static bool s_blockDrag = false;
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                s_blockDrag = true;
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll != 0.0f) {
                m_dist -= scroll * 0.3f;
                m_dist  = std::clamp(m_dist, 1.5f, 8.0f);
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            s_blockDrag = false;
        if (s_blockDrag) {
            ImGuiIO& pio = ImGui::GetIO();
            m_yaw   += pio.MouseDelta.x * 0.01f;
            m_pitch += pio.MouseDelta.y * 0.01f;
            m_pitch  = std::clamp(m_pitch, -1.4f, 1.4f);
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
        ImGui::Text("You have unsaved changes in Block Designer.");
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
    glClearColor(0.08f, 0.08f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    // Simple gradient via glClear is enough; use darker bottom
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    shdr->use();

    float cx = 0.5f, cy = 0.5f, cz = 0.5f;
    float camX = cx + m_dist * cosf(m_pitch) * sinf(m_yaw);
    float camY = cy + m_dist * sinf(m_pitch);
    float camZ = cz + m_dist * cosf(m_pitch) * cosf(m_yaw);
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

    addBlockFace(0, {1,0,0}, {1,0,1}, {1,1,1}, {1,1,0}, {1,0,0});
    addBlockFace(1, {0,0,1}, {0,0,0}, {0,1,0}, {0,1,1}, {-1,0,0});
    addBlockFace(2, {0,1,0}, {1,1,0}, {1,1,1}, {0,1,1}, {0,1,0});
    addBlockFace(3, {0,0,1}, {1,0,1}, {1,0,0}, {0,0,0}, {0,-1,0});
    addBlockFace(4, {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}, {0,0,1});
    addBlockFace(5, {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0}, {0,0,-1});

    GLMesh mesh;
    mesh.upload(mb.getVertices());
    mesh.draw();

    mesh.destroy();

    // Restore GL state
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
    auto& blocks = GameRegistry::getInstance().getAllBlocks();
    blocks[m_workingId] = m_workingCopy;
    m_dirty = false;
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
