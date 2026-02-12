#pragma once

#include "Registry.hpp"
#include <vector>
#include <string>

class WeatherDesigner {
public:
    void show(bool* open);
    void update(float dt, float worldTime);

    const WeatherPreset& getActivePreset() const;
    bool isTransitioning() const { return m_activeTransitionIdx >= 0; }
    float getTransitionProgress() const { return m_transitionProgress; }

private:
    void initDefaults();
    WeatherPreset lerpPresets(const WeatherPreset& a, const WeatherPreset& b, float t) const;
    float applyEasing(float t, int easingType) const;
    void pushUndo();
    void undo();
    void redo();

    std::vector<WeatherPreset> m_presets;
    int m_activePresetIdx = 0;
    int m_selectedPresetIdx = 0;
    bool m_inited = false;

    // Transition
    std::vector<WeatherTransition> m_transitions;
    int m_activeTransitionIdx = -1;
    float m_transitionProgress = 0.0f;
    WeatherPreset m_blendedPreset;

    // Undo
    std::vector<WeatherPreset> m_undoStack;
    std::vector<WeatherPreset> m_redoStack;
    bool m_dirty = false;
    static constexpr int kMaxUndo = 40;

    // Preview animation
    float m_previewTime = 0.0f;

    // Clipboard for copy/paste
    bool m_hasClipboard = false;
    WeatherPreset m_clipboard;
};
