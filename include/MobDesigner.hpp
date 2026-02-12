#pragma once

#include "Registry.hpp"
#include "AINodeEditor.hpp"
#include <vector>
#include <fstream>

struct GLFWwindow;
class Framebuffer;
class Shader;

class MobDesigner {
public:
    void show(bool* open, unsigned int atlasID, Framebuffer* previewBuffer, Shader* previewShader, Framebuffer* partPreviewBuf);

    // External block editing coordination
    bool wantsBlockDesigner() const { return m_wantsBlockDesigner; }
    void clearWantsBlockDesigner() { m_wantsBlockDesigner = false; }
    bool isEditingPartExternally() const { return m_editingPartExternally; }
    int getEditingPartIndex() const { return m_editingPartIdx; }
    BlockDefinition getPartAsBlock(int partIdx) const;
    void applyBlockToPart(int partIdx, const BlockDefinition& block);
    void finishExternalEdit() { m_editingPartExternally = false; m_editingPartIdx = -1; }
    const std::vector<MobPart>& getMobParts() const { return m_workingCopy.parts; }

    float m_yaw = 0.6f;
    float m_pitch = 0.35f;
    float m_dist = 5.0f;
    float m_panX = 0.0f;
    float m_panY = 0.0f;

private:
    void renderPreview(const MobDefinition& def, unsigned int atlasID, Framebuffer* buf, Shader* shdr);
    void renderPartPreview(const MobPart& part, unsigned int atlasID, Framebuffer* buf, Shader* shdr);
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

    // Preview selection highlight
    int m_selectedPartForPreview = -1;

    // Inline face editor
    int m_selectedFace = -1;
    int m_hoveredFace = -1;

    // External block editing state
    bool m_wantsBlockDesigner = false;
    bool m_editingPartExternally = false;
    int m_editingPartIdx = -1;

    // AI Node Editor
    AINodeEditor m_aiEditor;

    // Resizable panel widths
    float m_listPanelWidth = 180.0f;
    float m_propsPanelWidth = 420.0f;
};
