#pragma once

#include <string>
#include <vector>

struct SoundOscillator {
    int waveform = 0;         // 0=sine, 1=square, 2=sawtooth, 3=triangle, 4=noise, 5=pulse
    float frequency = 440.0f;
    float amplitude = 0.5f;
    float phase = 0.0f;
    float detune = 0.0f;      // cents
    float pulseWidth = 0.5f;  // for pulse waveform
};

struct SoundEnvelope {
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.2f;
    float attackCurve = 1.0f;  // 1.0 = linear, <1 = logarithmic, >1 = exponential
    float decayCurve = 1.0f;
};

struct SoundFilter {
    int filterType = 0;        // 0=none, 1=lowpass, 2=highpass, 3=bandpass, 4=notch
    float cutoff = 1000.0f;
    float resonance = 0.5f;
    float envelopeAmount = 0.0f;
};

struct SoundEffect {
    int effectType = 0;        // 0=none,1=reverb,2=delay,3=chorus,4=distortion,5=flanger,6=phaser,7=tremolo,8=vibrato
    float mix = 0.3f;          // wet/dry
    float parameter1 = 0.5f;
    float parameter2 = 0.5f;
    bool enabled = false;
};

struct SoundLFO {
    int waveform = 0;          // 0=sine, 1=square, 2=sawtooth, 3=triangle
    float frequency = 5.0f;
    float amplitude = 0.5f;
    int target = 0;            // 0=pitch, 1=amplitude, 2=filter_cutoff, 3=pan
};

struct SoundPreset {
    std::string name = "New Sound";
    std::string category = "SFX";
    SoundOscillator oscillators[2];
    SoundEnvelope envelope;
    SoundFilter filter;
    std::vector<SoundEffect> effects;
    SoundLFO lfo;
    float masterVolume = 0.8f;
    float pan = 0.0f;           // -1 left, 0 center, +1 right
    float pitchBendRange = 2.0f; // semitones
    float duration = 0.5f;      // seconds
    bool loop = false;
};

class SoundDesigner {
public:
    void show(bool* open);

private:
    void initDefaults();

    // Audio preview generation
    float generateSample(const SoundPreset& preset, float time, float envValue);
    float computeEnvelope(const SoundEnvelope& env, float time, float duration);
    void generatePreviewBuffer();

    // Save / Load
    void savePresets();
    void loadPresets();

    // Undo / Redo
    void pushUndo();
    void undo();
    void redo();

    // Presets
    std::vector<SoundPreset> m_presets;
    int m_activePresetIdx = 0;
    bool m_initialized = false;
    bool m_dirty = false;

    // Waveform preview buffer
    static constexpr int kPreviewSamples = 512;
    float m_previewBuffer[kPreviewSamples] = {};
    bool m_previewGenerated = false;

    // Search and category filter
    char m_searchFilter[64] = "";
    std::vector<std::string> m_categories;
    int m_selectedCategory = 0; // 0 = All

    // Playback preview state
    bool m_isPlaying = false;
    float m_playbackTime = 0.0f;

    // Undo/Redo stacks
    struct UndoState {
        std::vector<SoundPreset> presets;
        int activeIdx;
    };
    std::vector<UndoState> m_undoStack;
    std::vector<UndoState> m_redoStack;
    static constexpr int kMaxUndoSteps = 40;

    // Clipboard for copy/paste
    bool m_hasClipboard = false;
    SoundPreset m_clipboard;

    // Save notification
    float m_saveNotifyTimer = 0.0f;
};
