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

    float m_yaw = 0.78f;
    float m_pitch = 0.5f;
    float m_dist = 3.0f;

private:
    void renderPreview(const BlockDefinition& def, unsigned int atlasID, Framebuffer* buf, Shader* shdr);
    unsigned int getPreviewTexture(Framebuffer* buf);

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
};
