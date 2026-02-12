#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

class Texture;

class TextureDesigner {
public:
    void show(bool* open, Texture* atlas);

private:
    enum Tool { TOOL_PENCIL, TOOL_ERASER, TOOL_FILL, TOOL_EYEDROPPER };

    // Current tile slot
    int m_tileX = 0;
    int m_tileY = 4; // Start at row 4 (first custom row)
    bool m_initialized = false;

    // Canvas: 16x16 RGBA pixels
    uint8_t m_pixels[16 * 16 * 4] = {};

    // Brush
    float m_brushColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    Tool m_currentTool = TOOL_PENCIL;

    // Undo/Redo
    struct CanvasState {
        uint8_t pixels[16 * 16 * 4];
    };
    std::vector<CanvasState> m_undoStack;
    std::vector<CanvasState> m_redoStack;
    static constexpr int kMaxUndo = 50;

    // Dirty tracking
    bool m_dirty = false;
    float m_saveNotifyTimer = 0.0f;

    // Canvas zoom
    float m_pixelSize = 20.0f;

    void loadFromAtlas(Texture* atlas);
    void saveToAtlas(Texture* atlas);
    void pushUndo();
    void undo();
    void redo();

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void getPixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;
    void floodFill(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    // Backup
    void saveBackup() const;
    bool loadBackup();
    void clearBackup() const;
    static constexpr const char* kBackupFile = "texture_designer_backup.dat";
};
