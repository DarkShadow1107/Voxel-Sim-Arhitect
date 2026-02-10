#pragma once

#include "Registry.hpp"
#include <vector>
#include <fstream>

struct GLFWwindow;
class Framebuffer;
class Shader;

class MobDesigner {
public:
    void show(bool* open, unsigned int atlasID, Framebuffer* previewBuffer, Shader* previewShader);

    float m_yaw = 0.6f;
    float m_pitch = 0.35f;
    float m_dist = 5.0f;

private:
    void renderPreview(const MobDefinition& def, unsigned int atlasID, Framebuffer* buf, Shader* shdr);
    unsigned int getPreviewTexture(Framebuffer* buf);

    // --- Save / Undo / Redo ---
    MobDefinition m_workingCopy{};
    MobType m_workingType = MOB_COW;
    bool m_dirty = false;
    bool m_initialized = false;

    std::vector<MobDefinition> m_undoStack;
    std::vector<MobDefinition> m_redoStack;
    static constexpr int kMaxUndoSteps = 50;

    void loadWorkingCopy(MobType type);
    void pushUndo();
    void undo();
    void redo();
    void saveToRegistry();
    void discardChanges();

    // Crash backup
    void saveBackup() const;
    bool loadBackup();
    void clearBackup() const;
    static constexpr const char* kBackupFile = "mob_designer_backup.dat";

    // Close confirmation
    bool m_showSavePrompt = false;
    bool* m_pendingClose = nullptr;
};
