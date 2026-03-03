#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
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
#include "UIToolbar.hpp"
#include "HelperWindow.hpp"

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

    // Window tab bar — thin secondary row showing every open panel as a closeable chip.
    // Call each frame immediately after showMainMenuBar().
    void showWindowTabBar(bool& showProfiler, bool& showMemory, bool& showECS,
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
    // Show the standalone help / reference window; also called internally by the Help menu.
    void showHelperWindow()                 { m_helperWindow.show(); }
    void openHelperWindow()                 { m_helperWindow.open(); }

    // Supply live world stats once per frame (call before showMainMenuBar).
    // These values are displayed in the right-side status bar of the menu bar.
    //   px/py/pz  — player world position
    //   biome     — display name of the biome at player position (may be nullptr)
    //   chunks    — number of currently loaded chunks
    //   day       — world time in ticks (used to show day/night badge)
    void setWorldStats(float px, float py, float pz,
                       const char* biome, int chunks, float worldTime = 0.0f) {
        m_playerX      = px;
        m_playerY      = py;
        m_playerZ      = pz;
        m_biomeName    = biome ? biome : "";
        m_loadedChunks = chunks;
        m_worldTime    = worldTime;
    }
    // Supply per-frame gameplay stats for the navbar status bar.
    //   isCreative — true if in Creative mode
    //   level      — player XP level
    //   hp         — current player health (0-20)
    //   oxygen     — breath meter (0-20; used to show drowning state)
    void setPlayerStats(bool isCreative, uint32_t level, float hp, float oxygen = 20.0f) {
        m_isCreativeMode = isCreative;
        m_playerLevel    = level;
        m_playerHp       = hp;
        m_playerOxygen   = oxygen;
    }
    // Supply the current world name so the navbar can display it.
    void setWorldName(const char* name) { m_worldName = name ? name : ""; }
    WorldEditor& getWorldEditor()           { return m_worldEditor; }
    WeatherDesigner& getWeatherDesigner()   { return m_weatherDesigner; }

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
    bool m_vsync = false;

    // Live world stats (updated via setWorldStats once per frame)
    float       m_playerX = 0.0f, m_playerY = 0.0f, m_playerZ = 0.0f;
    std::string m_biomeName;
    std::string m_worldName;       // current world save name shown in navbar
    int         m_loadedChunks = 0;
    float       m_worldTime    = 0.0f;
    // Live gameplay stats (updated via setPlayerStats once per frame)
    bool        m_isCreativeMode = true;
    uint32_t    m_playerLevel    = 0;
    float       m_playerHp       = 20.0f;
    float       m_playerOxygen   = 20.0f;

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
    HelperWindow   m_helperWindow;
};
