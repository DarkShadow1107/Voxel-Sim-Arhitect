#pragma once

#include "Registry.hpp"
#include <vector>

class Framebuffer;
class Shader;

class ToolDesigner {
public:
    void show(bool* open, unsigned int atlasID, Framebuffer* previewBuffer, Shader* previewShader);

    // External block editing coordination
    bool wantsBlockDesigner() const { return m_wantsBlockDesigner; }
    void clearWantsBlockDesigner() { m_wantsBlockDesigner = false; }
    bool isEditingPieceExternally() const { return m_editingPieceExternally; }
    int getEditingPieceIndex() const { return m_editingPieceIdx; }
    BlockDefinition getPieceAsBlock(int pieceIdx) const;
    void applyBlockToPiece(int pieceIdx, const BlockDefinition& block);
    Vec3 getPieceShape(int pieceIdx) const;
    void finishExternalEdit() { m_editingPieceExternally = false; m_editingPieceIdx = -1; }

    float m_yaw = 0.6f;
    float m_pitch = 0.35f;
    float m_dist = 6.0f;
    float m_panX = 0.0f;
    float m_panY = 0.0f;

private:
    void renderPreview(const ToolDefinition& def, unsigned int atlasID, Framebuffer* buf, Shader* shdr);
    unsigned int getPreviewTexture(Framebuffer* buf);

    // Working copy
    ToolDefinition m_workingCopy{};
    int m_workingId = -1;
    bool m_dirty = false;
    bool m_initialized = false;

    // Undo/Redo
    std::vector<ToolDefinition> m_undoStack;
    std::vector<ToolDefinition> m_redoStack;
    static constexpr int kMaxUndoSteps = 30;

    void loadWorkingCopy(int id);
    void pushUndo();
    void undo();
    void redo();
    void saveToRegistry();
    void discardChanges();

    // External block editing state
    bool m_wantsBlockDesigner = false;
    bool m_editingPieceExternally = false;
    int m_editingPieceIdx = -1;

    // 3D preview selection
    int m_selectedPieceForPreview = -1;

    // Ensures customPieces vector matches the procedural piece count
    void ensureCustomPieces();
    int getProceduralPieceCount() const;

    // Comparison
    int m_compareToolId = -1;

    // Search filter
    char m_searchFilter[64] = "";

    // Save notification
    float m_saveNotifyTimer = 0.0f;

    // Resizable panel widths
    float m_listPanelWidth = 200.0f;
    float m_propsPanelWidth = 440.0f;
};
