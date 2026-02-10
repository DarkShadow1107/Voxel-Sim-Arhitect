#include "MobDesigner.hpp"
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

void MobDesigner::show(bool* open, unsigned int atlasID, Framebuffer* previewBuffer, Shader* previewShader) {
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
    ImGui::BeginChild("##MobListPanel", ImVec2(180, 0), true);
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

    ImGui::SameLine();

    if (!mobs.count(selectedType)) { ImGui::End(); return; }

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
    ImGui::BeginChild("##MobCenterPanel", ImVec2(420, 0), false);
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

    ImGui::SameLine();

    // ===== RIGHT PANEL: Interactive 3D Preview =====
    ImGui::BeginChild("##Mob3DPanel", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "3D PREVIEW");
        ImGui::Separator();

        // View controls (above preview so they are always visible)
        if (ImGui::Button("Reset View")) { m_yaw = 0.6f; m_pitch = 0.35f; m_dist = 5.0f; }
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
        static bool s_mobDrag = false;
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                s_mobDrag = true;
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll != 0.0f) {
                m_dist -= scroll * 0.4f;
                m_dist  = std::clamp(m_dist, 2.0f, 15.0f);
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            s_mobDrag = false;
        if (s_mobDrag) {
            ImGuiIO& pio = ImGui::GetIO();
            m_yaw   += pio.MouseDelta.x * 0.01f;
            m_pitch += pio.MouseDelta.y * 0.01f;
            m_pitch  = std::clamp(m_pitch, -1.4f, 1.4f);
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
                    a.affectedByHeadAnim != b.affectedByHeadAnim) { changed = true; break; }
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
    glClearColor(0.08f, 0.08f, 0.14f, 1.0f);
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

    // --- Draw mob parts ---
    MeshBuilder mb;
    float s = 1.0f / 16.0f;
    // Use an untouched white atlas tile (15,15) so vertex color alone determines part color
    float u = 15 * s, v = 15 * s;

    for (const auto& part : def.parts) {
        Vec3 o = part.offset;
        Vec3 sz = part.size;
        Vec3 c = part.color;

        mb.addFace({o.x+sz.x, o.y, o.z}, {o.x+sz.x, o.y, o.z+sz.z}, {o.x+sz.x, o.y+sz.y, o.z+sz.z}, {o.x+sz.x, o.y+sz.y, o.z}, {1,0,0}, u,v,u+s,v+s, c.x, c.y, c.z);
        mb.addFace({o.x, o.y, o.z+sz.z}, {o.x, o.y, o.z}, {o.x, o.y+sz.y, o.z}, {o.x, o.y+sz.y, o.z+sz.z}, {-1,0,0}, u,v,u+s,v+s, c.x, c.y, c.z);
        mb.addFace({o.x, o.y+sz.y, o.z}, {o.x+sz.x, o.y+sz.y, o.z}, {o.x+sz.x, o.y+sz.y, o.z+sz.z}, {o.x, o.y+sz.y, o.z+sz.z}, {0,1,0}, u,v,u+s,v+s, c.x, c.y, c.z);
        mb.addFace({o.x, o.y, o.z+sz.z}, {o.x+sz.x, o.y, o.z+sz.z}, {o.x+sz.x, o.y, o.z}, {o.x, o.y, o.z}, {0,-1,0}, u,v,u+s,v+s, c.x, c.y, c.z);
        mb.addFace({o.x, o.y, o.z+sz.z}, {o.x+sz.x, o.y, o.z+sz.z}, {o.x+sz.x, o.y+sz.y, o.z+sz.z}, {o.x, o.y+sz.y, o.z+sz.z}, {0,0,1}, u,v,u+s,v+s, c.x, c.y, c.z);
        mb.addFace({o.x+sz.x, o.y, o.z}, {o.x, o.y, o.z}, {o.x, o.y+sz.y, o.z}, {o.x+sz.x, o.y+sz.y, o.z}, {0,0,-1}, u,v,u+s,v+s, c.x, c.y, c.z);
    }

    GLMesh mesh;
    mesh.upload(mb.getVertices());
    mesh.draw();

    mesh.destroy();

    // Restore GL state
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glDepthFunc(GL_LEQUAL); // Restore main renderer's depth func
}

unsigned int MobDesigner::getPreviewTexture(Framebuffer* buf) {
    return buf ? buf->getTexture() : 0;
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
    }
    return in.good();
}

void MobDesigner::clearBackup() const {
    std::remove(kBackupFile);
}
