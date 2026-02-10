#pragma once

#include <cstddef>
#include <cstdint>
#include "BlockDesigner.hpp"
#include "MobDesigner.hpp"
#include "SoundEditor.hpp"
#include "EngineProfiler.hpp"

struct GLFWwindow;

class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    bool init(GLFWwindow* window);
    void setAtlasTextureID(uint32_t id) { m_atlasID = id; }
    void beginFrame();
    void endFrame();
    void shutdown();

    // Theme
    void applyTheme();

    // Menu bar
    void showMainMenuBar(bool& showProfiler, bool& showMemory, bool& showECS,
                         bool& showWorldEditor, bool& showSettings,
                         bool& showSoundEditor, bool& showBlockDesigner,
                         bool& showMobDesigner, bool& showInteractionEditor);

    // Panels that stay in GUIManager
    void showSettings(bool* open, bool& vsync, bool& wireframe,
                      bool& fullscreen, bool& backfaceCulling, class Renderer& renderer);
    void showInteractionEditor(bool* open);
    void showMemoryInspector(size_t arenaOffset, size_t arenaSize, size_t poolUsed, size_t poolTotal);
    void showECSEditor();

    // Modular panels – thin delegates
    void showBlockDesigner(bool* open)      { m_blockDesigner.show(open, m_atlasID, m_blockPreviewBuf, m_previewShader); }
    void showMobDesigner(bool* open)        { m_mobDesigner.show(open, m_atlasID, m_mobPreviewBuf, m_previewShader); }
    void showSoundEditor(bool* open)        { m_soundEditor.show(open); }
    void showProfiler(float frameTime)      { m_profiler.show(frameTime, m_vsync, m_window); }

private:
    GLFWwindow* m_window = nullptr;
    uint32_t m_atlasID = 0;
    class Framebuffer* m_blockPreviewBuf = nullptr;
    class Framebuffer* m_mobPreviewBuf   = nullptr;
    class Shader* m_previewShader = nullptr;
    bool m_initialized = false;
    bool m_vsync = true;

    // Modular UI panels
    BlockDesigner  m_blockDesigner;
    MobDesigner    m_mobDesigner;
    SoundEditor    m_soundEditor;
    EngineProfiler m_profiler;
};
