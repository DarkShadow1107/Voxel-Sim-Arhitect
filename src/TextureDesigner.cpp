#include "TextureDesigner.hpp"
#include "Texture.hpp"
#include "imgui.h"
#include <cstring>
#include <queue>
#include <algorithm>

void TextureDesigner::show(bool* open, Texture* atlas) {
    if (!open || !*open || !atlas) return;

    if (!m_initialized) {
        if (!loadBackup()) {
            loadFromAtlas(atlas);
        }
        m_initialized = true;
    }

    ImVec2 mvCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(mvCenter, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Texture Designer", open)) {
        ImGui::End();
        return;
    }

    // Save notification
    if (m_saveNotifyTimer > 0.0f) {
        m_saveNotifyTimer -= ImGui::GetIO().DeltaTime;
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Saved to atlas!");
    }

    // ============ Toolbar ============
    if (ImGui::Button("Save to Atlas")) {
        saveToAtlas(atlas);
        m_dirty = false;
        m_saveNotifyTimer = 2.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard")) {
        loadFromAtlas(atlas);
        m_dirty = false;
        m_undoStack.clear();
        m_redoStack.clear();
    }
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    if (ImGui::Button("Undo") || (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))) {
        undo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo") || (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))) {
        redo();
    }
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        pushUndo();
        std::memset(m_pixels, 0, sizeof(m_pixels));
        m_dirty = true;
    }

    if (m_dirty) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(unsaved)");
    }

    ImGui::Separator();

    // ============ Layout: Left panel (slot picker) | Center (canvas) | Right (tools/color) ============
    float availW = ImGui::GetContentRegionAvail().x;
    float slotPanelW = 180.0f;
    float toolPanelW = 200.0f;
    float canvasW = availW - slotPanelW - toolPanelW - 16.0f;
    if (canvasW < 200.0f) canvasW = 200.0f;

    // --- Left: Tile Slot Picker ---
    ImGui::BeginChild("SlotPicker", ImVec2(slotPanelW, 0), true);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Tile Slot");
    ImGui::Separator();

    ImGui::Text("Row: %d  Col: %d", m_tileY, m_tileX);

    // Grid of available custom slots (rows 4-15 = 192 slots)
    ImGui::TextWrapped("Custom tiles: rows 4-15");
    ImGui::Separator();

    float slotBtnSize = 20.0f;
    ImTextureID texId = (ImTextureID)(uintptr_t)atlas->getID();

    for (int row = 4; row < 16; ++row) {
        for (int col = 0; col < 16; ++col) {
            ImGui::PushID(row * 16 + col);

            float u0 = col / 16.0f;
            float v0 = row / 16.0f;
            float u1 = (col + 1) / 16.0f;
            float v1 = (row + 1) / 16.0f;

            bool isSelected = (m_tileX == col && m_tileY == row);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
            }

            if (ImGui::ImageButton("##slot", texId, ImVec2(slotBtnSize, slotBtnSize), ImVec2(u0, v0), ImVec2(u1, v1))) {
                // Save current before switching
                if (m_dirty) {
                    saveToAtlas(atlas);
                    m_dirty = false;
                }
                m_tileX = col;
                m_tileY = row;
                loadFromAtlas(atlas);
                m_undoStack.clear();
                m_redoStack.clear();
            }

            if (isSelected) {
                ImGui::PopStyleColor();
            }

            ImGui::PopID();

            if (col < 15) ImGui::SameLine(0, 2);
        }
    }

    // Also allow editing existing tiles (rows 0-3) in read-only-ish manner
    ImGui::Separator();
    ImGui::TextWrapped("Built-in tiles (rows 0-3):");
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 16; ++col) {
            ImGui::PushID(1000 + row * 16 + col);

            float u0 = col / 16.0f;
            float v0 = row / 16.0f;
            float u1 = (col + 1) / 16.0f;
            float v1 = (row + 1) / 16.0f;

            bool isSelected = (m_tileX == col && m_tileY == row);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.5f, 0.2f, 1.0f));
            }

            if (ImGui::ImageButton("##builtinslot", texId, ImVec2(slotBtnSize, slotBtnSize), ImVec2(u0, v0), ImVec2(u1, v1))) {
                if (m_dirty) {
                    saveToAtlas(atlas);
                    m_dirty = false;
                }
                m_tileX = col;
                m_tileY = row;
                loadFromAtlas(atlas);
                m_undoStack.clear();
                m_redoStack.clear();
            }

            if (isSelected) {
                ImGui::PopStyleColor();
            }

            ImGui::PopID();

            if (col < 15) ImGui::SameLine(0, 2);
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // --- Center: Canvas ---
    ImGui::BeginChild("Canvas", ImVec2(canvasW, 0), true);
    ImGui::Text("Tile (%d, %d) - 16x16 pixels", m_tileX, m_tileY);

    // Zoom control
    ImGui::SliderFloat("Zoom", &m_pixelSize, 8.0f, 40.0f, "%.0f px");
    ImGui::Separator();

    ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    float totalSize = m_pixelSize * 16.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Draw checkerboard background (for transparency)
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            ImVec2 p0(canvasOrigin.x + x * m_pixelSize, canvasOrigin.y + y * m_pixelSize);
            ImVec2 p1(p0.x + m_pixelSize, p0.y + m_pixelSize);

            bool dark = ((x + y) % 2 == 0);
            ImU32 bgCol = dark ? IM_COL32(40, 40, 40, 255) : IM_COL32(60, 60, 60, 255);
            drawList->AddRectFilled(p0, p1, bgCol);
        }
    }

    // Draw pixels
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            int idx = (y * 16 + x) * 4;
            uint8_t r = m_pixels[idx];
            uint8_t g = m_pixels[idx + 1];
            uint8_t b = m_pixels[idx + 2];
            uint8_t a = m_pixels[idx + 3];

            if (a > 0) {
                ImVec2 p0(canvasOrigin.x + x * m_pixelSize, canvasOrigin.y + y * m_pixelSize);
                ImVec2 p1(p0.x + m_pixelSize, p0.y + m_pixelSize);
                drawList->AddRectFilled(p0, p1, IM_COL32(r, g, b, a));
            }
        }
    }

    // Draw grid lines
    ImU32 gridCol = IM_COL32(100, 100, 100, 80);
    for (int i = 0; i <= 16; ++i) {
        float xPos = canvasOrigin.x + i * m_pixelSize;
        float yPos = canvasOrigin.y + i * m_pixelSize;
        drawList->AddLine(ImVec2(xPos, canvasOrigin.y), ImVec2(xPos, canvasOrigin.y + totalSize), gridCol);
        drawList->AddLine(ImVec2(canvasOrigin.x, yPos), ImVec2(canvasOrigin.x + totalSize, yPos), gridCol);
    }

    // Draw border
    drawList->AddRect(canvasOrigin, ImVec2(canvasOrigin.x + totalSize, canvasOrigin.y + totalSize), IM_COL32(200, 200, 200, 255));

    // Invisible button for mouse interaction
    ImGui::SetCursorScreenPos(canvasOrigin);
    ImGui::InvisibleButton("##canvas_interact", ImVec2(totalSize, totalSize));

    if (ImGui::IsItemHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        int px = (int)((mousePos.x - canvasOrigin.x) / m_pixelSize);
        int py = (int)((mousePos.y - canvasOrigin.y) / m_pixelSize);

        if (px >= 0 && px < 16 && py >= 0 && py < 16) {
            // Hover highlight
            ImVec2 hp0(canvasOrigin.x + px * m_pixelSize, canvasOrigin.y + py * m_pixelSize);
            ImVec2 hp1(hp0.x + m_pixelSize, hp0.y + m_pixelSize);
            drawList->AddRect(hp0, hp1, IM_COL32(255, 255, 255, 200), 0, 0, 2.0f);

            // Show pixel coords
            ImGui::SetTooltip("(%d, %d)", px, py);

            // Left click: paint
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                static bool paintStarted = false;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    pushUndo();
                    paintStarted = true;
                }

                switch (m_currentTool) {
                    case TOOL_PENCIL:
                        setPixel(px, py,
                            (uint8_t)(m_brushColor[0] * 255),
                            (uint8_t)(m_brushColor[1] * 255),
                            (uint8_t)(m_brushColor[2] * 255),
                            (uint8_t)(m_brushColor[3] * 255));
                        m_dirty = true;
                        break;
                    case TOOL_ERASER:
                        setPixel(px, py, 0, 0, 0, 0);
                        m_dirty = true;
                        break;
                    case TOOL_FILL:
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            floodFill(px, py,
                                (uint8_t)(m_brushColor[0] * 255),
                                (uint8_t)(m_brushColor[1] * 255),
                                (uint8_t)(m_brushColor[2] * 255),
                                (uint8_t)(m_brushColor[3] * 255));
                            m_dirty = true;
                        }
                        break;
                    case TOOL_EYEDROPPER:
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            uint8_t er, eg, eb, ea;
                            getPixel(px, py, er, eg, eb, ea);
                            m_brushColor[0] = er / 255.0f;
                            m_brushColor[1] = eg / 255.0f;
                            m_brushColor[2] = eb / 255.0f;
                            m_brushColor[3] = ea / 255.0f;
                            m_currentTool = TOOL_PENCIL;
                            // Remove the undo we just pushed
                            if (!m_undoStack.empty()) m_undoStack.pop_back();
                        }
                        break;
                }
            }

            // Right click: erase
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    pushUndo();
                }
                setPixel(px, py, 0, 0, 0, 0);
                m_dirty = true;
            }
        }
    }

    // Preview of current tile at actual size
    ImGui::SetCursorScreenPos(ImVec2(canvasOrigin.x, canvasOrigin.y + totalSize + 8));
    ImGui::Text("Preview (actual size):");
    float u0 = m_tileX / 16.0f;
    float v0 = m_tileY / 16.0f;
    float u1 = (m_tileX + 1) / 16.0f;
    float v1 = (m_tileY + 1) / 16.0f;
    ImGui::Image((ImTextureID)(uintptr_t)atlas->getID(), ImVec2(64, 64), ImVec2(u0, v0), ImVec2(u1, v1));

    ImGui::EndChild();

    ImGui::SameLine();

    // --- Right: Tools & Color ---
    ImGui::BeginChild("ToolPanel", ImVec2(toolPanelW, 0), true);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Tools");
    ImGui::Separator();

    const char* toolNames[] = { "Pencil", "Eraser", "Fill", "Eyedropper" };
    for (int i = 0; i < 4; ++i) {
        if (i > 0) ImGui::SameLine();
        bool selected = (m_currentTool == (Tool)i);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
        if (ImGui::Button(toolNames[i])) {
            m_currentTool = (Tool)i;
        }
        if (selected) ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Brush Color");
    ImGui::ColorPicker4("##brushcolor", m_brushColor,
        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);

    // Quick color presets
    ImGui::Separator();
    ImGui::Text("Quick Colors:");
    struct QuickColor { const char* name; float r, g, b, a; };
    QuickColor presets[] = {
        {"White", 1, 1, 1, 1}, {"Black", 0, 0, 0, 1}, {"Red", 1, 0, 0, 1},
        {"Green", 0, 1, 0, 1}, {"Blue", 0, 0, 1, 1}, {"Yellow", 1, 1, 0, 1},
        {"Brown", 0.55f, 0.35f, 0.2f, 1}, {"Gray", 0.5f, 0.5f, 0.5f, 1},
    };
    int qi = 0;
    for (auto& qc : presets) {
        ImVec4 col(qc.r, qc.g, qc.b, qc.a);
        ImGui::PushID(qi++);
        if (ImGui::ColorButton(qc.name, col, 0, ImVec2(24, 24))) {
            m_brushColor[0] = qc.r;
            m_brushColor[1] = qc.g;
            m_brushColor[2] = qc.b;
            m_brushColor[3] = qc.a;
        }
        ImGui::PopID();
        if (qi % 4 != 0) ImGui::SameLine();
    }

    ImGui::Separator();
    ImGui::TextWrapped("Keyboard shortcuts:\nCtrl+Z: Undo\nCtrl+Y: Redo\nLeft click: Paint\nRight click: Erase");

    ImGui::EndChild();

    // Auto-backup on changes
    if (m_dirty) {
        saveBackup();
    }

    ImGui::End();
}

void TextureDesigner::loadFromAtlas(Texture* atlas) {
    if (atlas) {
        atlas->readTile(m_tileX, m_tileY, m_pixels);
    }
}

void TextureDesigner::saveToAtlas(Texture* atlas) {
    if (atlas) {
        atlas->updateTile(m_tileX, m_tileY, m_pixels);
        clearBackup();
    }
}

void TextureDesigner::pushUndo() {
    CanvasState state;
    std::memcpy(state.pixels, m_pixels, sizeof(m_pixels));
    m_undoStack.push_back(state);
    if ((int)m_undoStack.size() > kMaxUndo) {
        m_undoStack.erase(m_undoStack.begin());
    }
    m_redoStack.clear();
}

void TextureDesigner::undo() {
    if (m_undoStack.empty()) return;
    CanvasState redo;
    std::memcpy(redo.pixels, m_pixels, sizeof(m_pixels));
    m_redoStack.push_back(redo);

    std::memcpy(m_pixels, m_undoStack.back().pixels, sizeof(m_pixels));
    m_undoStack.pop_back();
    m_dirty = true;
}

void TextureDesigner::redo() {
    if (m_redoStack.empty()) return;
    CanvasState undo;
    std::memcpy(undo.pixels, m_pixels, sizeof(m_pixels));
    m_undoStack.push_back(undo);

    std::memcpy(m_pixels, m_redoStack.back().pixels, sizeof(m_pixels));
    m_redoStack.pop_back();
    m_dirty = true;
}

void TextureDesigner::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= 16 || y < 0 || y >= 16) return;
    int idx = (y * 16 + x) * 4;
    m_pixels[idx] = r;
    m_pixels[idx + 1] = g;
    m_pixels[idx + 2] = b;
    m_pixels[idx + 3] = a;
}

void TextureDesigner::getPixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const {
    if (x < 0 || x >= 16 || y < 0 || y >= 16) { r = g = b = a = 0; return; }
    int idx = (y * 16 + x) * 4;
    r = m_pixels[idx];
    g = m_pixels[idx + 1];
    b = m_pixels[idx + 2];
    a = m_pixels[idx + 3];
}

void TextureDesigner::floodFill(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= 16 || y < 0 || y >= 16) return;

    uint8_t tr, tg, tb, ta;
    getPixel(x, y, tr, tg, tb, ta);

    // Don't fill if target color matches fill color
    if (tr == r && tg == g && tb == b && ta == a) return;

    std::queue<std::pair<int, int>> q;
    q.push({x, y});

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        if (cx < 0 || cx >= 16 || cy < 0 || cy >= 16) continue;

        uint8_t cr, cg, cb, ca;
        getPixel(cx, cy, cr, cg, cb, ca);
        if (cr != tr || cg != tg || cb != tb || ca != ta) continue;

        setPixel(cx, cy, r, g, b, a);

        q.push({cx + 1, cy});
        q.push({cx - 1, cy});
        q.push({cx, cy + 1});
        q.push({cx, cy - 1});
    }
}

void TextureDesigner::saveBackup() const {
    std::ofstream file(kBackupFile, std::ios::binary);
    if (!file) return;
    file.write((const char*)&m_tileX, sizeof(m_tileX));
    file.write((const char*)&m_tileY, sizeof(m_tileY));
    file.write((const char*)m_pixels, sizeof(m_pixels));
    file.write((const char*)m_brushColor, sizeof(m_brushColor));
}

bool TextureDesigner::loadBackup() {
    std::ifstream file(kBackupFile, std::ios::binary);
    if (!file) return false;
    file.read((char*)&m_tileX, sizeof(m_tileX));
    file.read((char*)&m_tileY, sizeof(m_tileY));
    file.read((char*)m_pixels, sizeof(m_pixels));
    file.read((char*)m_brushColor, sizeof(m_brushColor));
    m_dirty = true;
    return true;
}

void TextureDesigner::clearBackup() const {
    std::remove(kBackupFile);
}
