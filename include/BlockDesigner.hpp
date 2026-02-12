#pragma once

#include "Registry.hpp"
#include <vector>
#include <fstream>

struct GLFWwindow;
class Framebuffer;
class Shader;

class BlockDesigner {
public:
    void show(bool* open, unsigned int atlasID, Framebuffer* previewBuffer, Shader* previewShader);

    // External edit mode (for mob part / tool piece editing)
    void loadExternalBlock(const BlockDefinition& def);
    void loadExternalBlock(const BlockDefinition& def, Vec3 shapeSize);
    bool isExternalMode() const { return m_externalMode; }
    bool isExternalDone() const { return m_externalDone; }
    BlockDefinition getExternalResult() const { return m_workingCopy; }
    void exitExternalMode();

    float m_yaw = 0.78f;
    float m_pitch = 0.5f;
    float m_dist = 3.0f;
    float m_panX = 0.0f;
    float m_panY = 0.0f;

private:
    void renderPreview(const BlockDefinition& def, unsigned int atlasID, Framebuffer* buf, Shader* shdr);
    unsigned int getPreviewTexture(Framebuffer* buf);

    // Face highlight in 3D preview
    int m_selectedFaceForPreview = -1;

    // External edit mode
    bool m_externalMode = false;
    bool m_externalSaved = false;
    bool m_externalDone = false;
    Vec3 m_externalShape = {1.0f, 1.0f, 1.0f}; // Shape dimensions for external part

    // --- Save / Undo / Redo ---
    // Working copy (edits go here; only applied to registry on Save)
    BlockDefinition m_workingCopy{};
    uint8_t m_workingId = 0;
    bool m_dirty = false;
    bool m_initialized = false;

    // Undo/Redo stacks
    std::vector<BlockDefinition> m_undoStack;
    std::vector<BlockDefinition> m_redoStack;
    static constexpr int kMaxUndoSteps = 50;

    void loadWorkingCopy(uint8_t id);
    void pushUndo();
    void undo();
    void redo();
    void saveToRegistry();
    void discardChanges();

    // Crash backup
    void saveBackup() const;
    bool loadBackup();
    void clearBackup() const;
    static constexpr const char* kBackupFile = "block_designer_backup.dat";

    // Close confirmation
    bool m_showSavePrompt = false;
    bool* m_pendingClose = nullptr;

    // Clipboard for copy/paste blocks
    bool m_hasClipboard = false;
    BlockDefinition m_clipboard;

    // Search filter
    char m_searchFilter[64] = "";

    // Save notification
    float m_saveNotifyTimer = 0.0f;

    // Resizable panel widths
    float m_listPanelWidth = 160.0f;
    float m_propsPanelWidth = 440.0f;
};
