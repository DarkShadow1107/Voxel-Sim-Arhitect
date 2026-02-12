#pragma once

#include <cstddef>
#include <cstdint>
#include "BlockDesigner.hpp"
#include "MobDesigner.hpp"
#include "ToolDesigner.hpp"
#include "SoundEditor.hpp"
#include "EngineProfiler.hpp"
#include "AINodeEditor.hpp"
#include "WeatherDesigner.hpp"
#include "WorldEditor.hpp"
#include "SoundDesigner.hpp"
#include "TextureDesigner.hpp"

struct GLFWwindow;
class Texture;

class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    bool init(GLFWwindow* window);
    void setAtlasTextureID(uint32_t id) { m_atlasID = id; }
    void setAtlasPointer(Texture* atlas) { m_atlasPtr = atlas; }
    void beginFrame();
    void endFrame();
    void shutdown();

    // Theme
    void applyTheme();

    // Menu bar
    void showMainMenuBar(bool& showProfiler, bool& showMemory, bool& showECS,
                         bool& showWorldEditor, bool& showSettings,
                         bool& showSoundEditor, bool& showBlockDesigner,
                         bool& showMobDesigner, bool& showInteractionEditor,
                         bool& showToolDesigner, bool& showWeatherDesigner,
                         bool& showSoundDesigner, bool& showAdvWorldEditor,
                         bool& showTextureDesigner);

    // Panels that stay in GUIManager
    void showSettings(bool* open, bool& vsync, bool& wireframe,
                      bool& fullscreen, bool& backfaceCulling, class Renderer& renderer);
    void showInteractionEditor(bool* open);
    void showMemoryInspector(size_t arenaOffset, size_t arenaSize, size_t poolUsed, size_t poolTotal);
    void showECSEditor();

    // Modular panels – thin delegates
    void showBlockDesigner(bool* open)      { m_blockDesigner.show(open, m_atlasID, m_blockPreviewBuf, m_previewShader); }
    void showMobDesigner(bool* open)        { m_mobDesigner.show(open, m_atlasID, m_mobPreviewBuf, m_previewShader, m_partPreviewBuf); }
    void showToolDesigner(bool* open)       { m_toolDesigner.show(open, m_atlasID, m_toolPreviewBuf, m_previewShader); }
    void showSoundEditor(bool* open)        { m_soundEditor.show(open); }
    void showWeatherDesigner(bool* open)    { m_weatherDesigner.show(open); }
    void showAdvWorldEditor(bool* open)     { m_worldEditor.show(open); }
    void showSoundDesigner(bool* open)      { m_soundDesigner.show(open); }
    void showTextureDesigner(bool* open)    { m_textureDesigner.show(open, m_atlasPtr); }
    void showProfiler(float frameTime)      { m_profiler.show(frameTime, m_vsync, m_window); }

    WeatherDesigner& getWeatherDesigner()   { return m_weatherDesigner; }
    WorldEditor& getWorldEditor()           { return m_worldEditor; }

    // MobDesigner / ToolDesigner <-> BlockDesigner coordination
    void coordinateMobBlockEdit(bool& showBlockDesigner);

    // Access to designers for coordination
    BlockDesigner& getBlockDesigner() { return m_blockDesigner; }
    MobDesigner& getMobDesigner() { return m_mobDesigner; }
    ToolDesigner& getToolDesigner() { return m_toolDesigner; }

private:
    GLFWwindow* m_window = nullptr;
    uint32_t m_atlasID = 0;
    Texture* m_atlasPtr = nullptr;
    class Framebuffer* m_blockPreviewBuf = nullptr;
    class Framebuffer* m_mobPreviewBuf   = nullptr;
    class Framebuffer* m_toolPreviewBuf  = nullptr;
    class Framebuffer* m_partPreviewBuf  = nullptr;
    class Shader* m_previewShader = nullptr;
    bool m_initialized = false;
    bool m_vsync = true;

    // Modular UI panels
    BlockDesigner  m_blockDesigner;
    MobDesigner    m_mobDesigner;
    ToolDesigner   m_toolDesigner;
    SoundEditor    m_soundEditor;
    EngineProfiler m_profiler;
    AINodeEditor   m_interactionAIEditor;
    WeatherDesigner m_weatherDesigner;
    WorldEditor    m_worldEditor;
    SoundDesigner  m_soundDesigner;
    TextureDesigner m_textureDesigner;
};
