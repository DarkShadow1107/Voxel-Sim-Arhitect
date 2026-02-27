#include "SoundDesigner.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr float TWO_PI = 6.28318f;

// ---------------------------------------------------------------------------
// Default presets
// ---------------------------------------------------------------------------

void SoundDesigner::initDefaults() {
    m_presets.clear();
    m_categories = {"All", "SFX", "Music", "Ambient", "UI", "Custom"};

    // 1. Block Break - short percussive click
    {
        SoundPreset p;
        p.name = "Block Break";
        p.category = "SFX";
        p.oscillators[0].waveform = 4; // noise
        p.oscillators[0].frequency = 800.0f;
        p.oscillators[0].amplitude = 0.7f;
        p.oscillators[0].phase = 0.0f;
        p.oscillators[0].detune = 0.0f;
        p.oscillators[0].pulseWidth = 0.5f;
        p.oscillators[1].waveform = 1; // square
        p.oscillators[1].frequency = 300.0f;
        p.oscillators[1].amplitude = 0.3f;
        p.oscillators[1].phase = 0.0f;
        p.oscillators[1].detune = 0.0f;
        p.oscillators[1].pulseWidth = 0.5f;
        p.envelope.attack = 0.002f;
        p.envelope.decay = 0.04f;
        p.envelope.sustain = 0.05f;
        p.envelope.release = 0.03f;
        p.envelope.attackCurve = 1.0f;
        p.envelope.decayCurve = 1.5f;
        p.filter.filterType = 2; // highpass
        p.filter.cutoff = 500.0f;
        p.filter.resonance = 0.3f;
        p.filter.envelopeAmount = 0.0f;
        p.effects.clear();
        p.lfo.waveform = 0;
        p.lfo.frequency = 0.0f;
        p.lfo.amplitude = 0.0f;
        p.lfo.target = 0;
        p.masterVolume = 0.9f;
        p.pan = 0.0f;
        p.pitchBendRange = 2.0f;
        p.duration = 0.1f;
        p.loop = false;
        m_presets.push_back(p);
    }

    // 2. Block Place - short thud
    {
        SoundPreset p;
        p.name = "Block Place";
        p.category = "SFX";
        p.oscillators[0].waveform = 0; // sine
        p.oscillators[0].frequency = 150.0f;
        p.oscillators[0].amplitude = 0.8f;
        p.oscillators[0].phase = 0.0f;
        p.oscillators[0].detune = 0.0f;
        p.oscillators[0].pulseWidth = 0.5f;
        p.oscillators[1].waveform = 4; // noise
        p.oscillators[1].frequency = 200.0f;
        p.oscillators[1].amplitude = 0.25f;
        p.oscillators[1].phase = 0.0f;
        p.oscillators[1].detune = 0.0f;
        p.oscillators[1].pulseWidth = 0.5f;
        p.envelope.attack = 0.001f;
        p.envelope.decay = 0.06f;
        p.envelope.sustain = 0.1f;
        p.envelope.release = 0.05f;
        p.envelope.attackCurve = 1.0f;
        p.envelope.decayCurve = 2.0f;
        p.filter.filterType = 1; // lowpass
        p.filter.cutoff = 400.0f;
        p.filter.resonance = 0.4f;
        p.filter.envelopeAmount = 0.0f;
        p.effects.clear();
        p.lfo.waveform = 0;
        p.lfo.frequency = 0.0f;
        p.lfo.amplitude = 0.0f;
        p.lfo.target = 0;
        p.masterVolume = 0.85f;
        p.pan = 0.0f;
        p.pitchBendRange = 2.0f;
        p.duration = 0.12f;
        p.loop = false;
        m_presets.push_back(p);
    }

    // 3. Footstep - short muted impact
    {
        SoundPreset p;
        p.name = "Footstep";
        p.category = "SFX";
        p.oscillators[0].waveform = 4; // noise
        p.oscillators[0].frequency = 250.0f;
        p.oscillators[0].amplitude = 0.6f;
        p.oscillators[0].phase = 0.0f;
        p.oscillators[0].detune = 0.0f;
        p.oscillators[0].pulseWidth = 0.5f;
        p.oscillators[1].waveform = 0; // sine sub
        p.oscillators[1].frequency = 80.0f;
        p.oscillators[1].amplitude = 0.35f;
        p.oscillators[1].phase = 0.0f;
        p.oscillators[1].detune = 0.0f;
        p.oscillators[1].pulseWidth = 0.5f;
        p.envelope.attack = 0.003f;
        p.envelope.decay = 0.04f;
        p.envelope.sustain = 0.05f;
        p.envelope.release = 0.04f;
        p.envelope.attackCurve = 1.0f;
        p.envelope.decayCurve = 1.5f;
        p.filter.filterType = 1; // lowpass
        p.filter.cutoff = 600.0f;
        p.filter.resonance = 0.2f;
        p.filter.envelopeAmount = 0.0f;
        p.effects.clear();
        p.lfo.waveform = 0;
        p.lfo.frequency = 0.0f;
        p.lfo.amplitude = 0.0f;
        p.lfo.target = 0;
        p.masterVolume = 0.7f;
        p.pan = 0.0f;
        p.pitchBendRange = 2.0f;
        p.duration = 0.1f;
        p.loop = false;
        m_presets.push_back(p);
    }

    // 4. Explosion - noise burst with long decay
    {
        SoundPreset p;
        p.name = "Explosion";
        p.category = "SFX";
        p.oscillators[0].waveform = 4; // noise
        p.oscillators[0].frequency = 100.0f;
        p.oscillators[0].amplitude = 1.0f;
        p.oscillators[0].phase = 0.0f;
        p.oscillators[0].detune = 0.0f;
        p.oscillators[0].pulseWidth = 0.5f;
        p.oscillators[1].waveform = 0; // sine sub-bass rumble
        p.oscillators[1].frequency = 40.0f;
        p.oscillators[1].amplitude = 0.8f;
        p.oscillators[1].phase = 0.0f;
        p.oscillators[1].detune = 0.0f;
        p.oscillators[1].pulseWidth = 0.5f;
        p.envelope.attack = 0.005f;
        p.envelope.decay = 0.3f;
        p.envelope.sustain = 0.15f;
        p.envelope.release = 0.8f;
        p.envelope.attackCurve = 0.5f;
        p.envelope.decayCurve = 2.0f;
        p.filter.filterType = 1; // lowpass
        p.filter.cutoff = 500.0f;
        p.filter.resonance = 0.5f;
        p.filter.envelopeAmount = 0.3f;
        {
            SoundEffect fx;
            fx.effectType = 4; // distortion
            fx.mix = 0.4f;
            fx.parameter1 = 0.6f;
            fx.parameter2 = 0.0f;
            fx.enabled = true;
            p.effects.push_back(fx);
        }
        {
            SoundEffect fx;
            fx.effectType = 1; // reverb
            fx.mix = 0.35f;
            fx.parameter1 = 0.8f;
            fx.parameter2 = 0.5f;
            fx.enabled = true;
            p.effects.push_back(fx);
        }
        p.lfo.waveform = 0;
        p.lfo.frequency = 2.0f;
        p.lfo.amplitude = 0.1f;
        p.lfo.target = 1; // amplitude
        p.masterVolume = 1.0f;
        p.pan = 0.0f;
        p.pitchBendRange = 2.0f;
        p.duration = 1.5f;
        p.loop = false;
        m_presets.push_back(p);
    }

    // 5. Ambient Wind - filtered noise with slow LFO
    {
        SoundPreset p;
        p.name = "Ambient Wind";
        p.category = "Ambient";
        p.oscillators[0].waveform = 4; // noise
        p.oscillators[0].frequency = 200.0f;
        p.oscillators[0].amplitude = 0.5f;
        p.oscillators[0].phase = 0.0f;
        p.oscillators[0].detune = 0.0f;
        p.oscillators[0].pulseWidth = 0.5f;
        p.oscillators[1].waveform = 0; // sine
        p.oscillators[1].frequency = 60.0f;
        p.oscillators[1].amplitude = 0.1f;
        p.oscillators[1].phase = 0.0f;
        p.oscillators[1].detune = 5.0f;
        p.oscillators[1].pulseWidth = 0.5f;
        p.envelope.attack = 0.8f;
        p.envelope.decay = 0.5f;
        p.envelope.sustain = 0.6f;
        p.envelope.release = 1.0f;
        p.envelope.attackCurve = 0.7f;
        p.envelope.decayCurve = 1.0f;
        p.filter.filterType = 1; // lowpass
        p.filter.cutoff = 800.0f;
        p.filter.resonance = 0.3f;
        p.filter.envelopeAmount = 0.2f;
        p.effects.clear();
        {
            SoundEffect fx;
            fx.effectType = 1; // reverb
            fx.mix = 0.4f;
            fx.parameter1 = 0.7f;
            fx.parameter2 = 0.3f;
            fx.enabled = true;
            p.effects.push_back(fx);
        }
        p.lfo.waveform = 0; // sine
        p.lfo.frequency = 0.3f;
        p.lfo.amplitude = 0.4f;
        p.lfo.target = 1; // amplitude
        p.masterVolume = 0.5f;
        p.pan = 0.0f;
        p.pitchBendRange = 2.0f;
        p.duration = 4.0f;
        p.loop = true;
        m_presets.push_back(p);
    }

    // 6. Water Drop - sine with fast decay, high pitch
    {
        SoundPreset p;
        p.name = "Water Drop";
        p.category = "SFX";
        p.oscillators[0].waveform = 0; // sine
        p.oscillators[0].frequency = 2200.0f;
        p.oscillators[0].amplitude = 0.7f;
        p.oscillators[0].phase = 0.0f;
        p.oscillators[0].detune = 0.0f;
        p.oscillators[0].pulseWidth = 0.5f;
        p.oscillators[1].waveform = 0; // sine harmonic
        p.oscillators[1].frequency = 3300.0f;
        p.oscillators[1].amplitude = 0.3f;
        p.oscillators[1].phase = 0.0f;
        p.oscillators[1].detune = 0.0f;
        p.oscillators[1].pulseWidth = 0.5f;
        p.envelope.attack = 0.001f;
        p.envelope.decay = 0.08f;
        p.envelope.sustain = 0.0f;
        p.envelope.release = 0.15f;
        p.envelope.attackCurve = 1.0f;
        p.envelope.decayCurve = 2.5f;
        p.filter.filterType = 0; // none
        p.filter.cutoff = 5000.0f;
        p.filter.resonance = 0.5f;
        p.filter.envelopeAmount = 0.0f;
        p.effects.clear();
        {
            SoundEffect fx;
            fx.effectType = 1; // reverb
            fx.mix = 0.3f;
            fx.parameter1 = 0.5f;
            fx.parameter2 = 0.4f;
            fx.enabled = true;
            p.effects.push_back(fx);
        }
        p.lfo.waveform = 0;
        p.lfo.frequency = 0.0f;
        p.lfo.amplitude = 0.0f;
        p.lfo.target = 0;
        p.masterVolume = 0.75f;
        p.pan = 0.0f;
        p.pitchBendRange = 2.0f;
        p.duration = 0.3f;
        p.loop = false;
        m_presets.push_back(p);
    }

    // 7. Thunder - noise with medium attack, slow decay
    {
        SoundPreset p;
        p.name = "Thunder";
        p.category = "Ambient";
        p.oscillators[0].waveform = 4; // noise
        p.oscillators[0].frequency = 80.0f;
        p.oscillators[0].amplitude = 1.0f;
        p.oscillators[0].phase = 0.0f;
        p.oscillators[0].detune = 0.0f;
        p.oscillators[0].pulseWidth = 0.5f;
        p.oscillators[1].waveform = 0; // sine rumble
        p.oscillators[1].frequency = 30.0f;
        p.oscillators[1].amplitude = 0.6f;
        p.oscillators[1].phase = 0.0f;
        p.oscillators[1].detune = 0.0f;
        p.oscillators[1].pulseWidth = 0.5f;
        p.envelope.attack = 0.15f;
        p.envelope.decay = 0.4f;
        p.envelope.sustain = 0.3f;
        p.envelope.release = 1.2f;
        p.envelope.attackCurve = 0.5f;
        p.envelope.decayCurve = 1.5f;
        p.filter.filterType = 1; // lowpass
        p.filter.cutoff = 400.0f;
        p.filter.resonance = 0.3f;
        p.filter.envelopeAmount = 0.2f;
        p.effects.clear();
        {
            SoundEffect fx;
            fx.effectType = 1; // reverb
            fx.mix = 0.5f;
            fx.parameter1 = 0.9f;
            fx.parameter2 = 0.3f;
            fx.enabled = true;
            p.effects.push_back(fx);
        }
        {
            SoundEffect fx;
            fx.effectType = 2; // delay
            fx.mix = 0.25f;
            fx.parameter1 = 0.4f;
            fx.parameter2 = 0.3f;
            fx.enabled = true;
            p.effects.push_back(fx);
        }
        p.lfo.waveform = 0;
        p.lfo.frequency = 1.5f;
        p.lfo.amplitude = 0.2f;
        p.lfo.target = 1; // amplitude
        p.masterVolume = 0.95f;
        p.pan = 0.0f;
        p.pitchBendRange = 2.0f;
        p.duration = 2.5f;
        p.loop = false;
        m_presets.push_back(p);
    }

    // 8. Item Pickup - short ascending tone
    {
        SoundPreset p;
        p.name = "Item Pickup";
        p.category = "UI";
        p.oscillators[0].waveform = 0; // sine
        p.oscillators[0].frequency = 800.0f;
        p.oscillators[0].amplitude = 0.6f;
        p.oscillators[0].phase = 0.0f;
        p.oscillators[0].detune = 0.0f;
        p.oscillators[0].pulseWidth = 0.5f;
        p.oscillators[1].waveform = 3; // triangle
        p.oscillators[1].frequency = 1200.0f;
        p.oscillators[1].amplitude = 0.3f;
        p.oscillators[1].phase = 0.0f;
        p.oscillators[1].detune = 0.0f;
        p.oscillators[1].pulseWidth = 0.5f;
        p.envelope.attack = 0.005f;
        p.envelope.decay = 0.08f;
        p.envelope.sustain = 0.3f;
        p.envelope.release = 0.12f;
        p.envelope.attackCurve = 1.0f;
        p.envelope.decayCurve = 1.0f;
        p.filter.filterType = 0; // none
        p.filter.cutoff = 5000.0f;
        p.filter.resonance = 0.5f;
        p.filter.envelopeAmount = 0.0f;
        p.effects.clear();
        {
            SoundEffect fx;
            fx.effectType = 1; // reverb
            fx.mix = 0.15f;
            fx.parameter1 = 0.3f;
            fx.parameter2 = 0.5f;
            fx.enabled = true;
            p.effects.push_back(fx);
        }
        p.lfo.waveform = 0;
        p.lfo.frequency = 0.0f;
        p.lfo.amplitude = 0.0f;
        p.lfo.target = 0;
        p.masterVolume = 0.75f;
        p.pan = 0.0f;
        p.pitchBendRange = 2.0f;
        p.duration = 0.25f;
        p.loop = false;
        m_presets.push_back(p);
    }

    m_activePresetIdx = 0;
}

// ---------------------------------------------------------------------------
// Waveform generation
// ---------------------------------------------------------------------------

float SoundDesigner::generateSample(const SoundPreset& preset, float time, float envValue) {
    float sample = 0.0f;
    for (int i = 0; i < 2; i++) {
        const auto& osc = preset.oscillators[i];
        float freq = osc.frequency * (1.0f + osc.detune * 0.01f);
        float phase = time * freq * TWO_PI + osc.phase;
        float val = 0.0f;
        switch (osc.waveform) {
            case 0: // sine
                val = sinf(phase);
                break;
            case 1: // square
                val = sinf(phase) > 0 ? 1.0f : -1.0f;
                break;
            case 2: // sawtooth
                val = 2.0f * fmodf(phase / TWO_PI, 1.0f) - 1.0f;
                break;
            case 3: // triangle
                val = 4.0f * fabsf(fmodf(phase / TWO_PI, 1.0f) - 0.5f) - 1.0f;
                break;
            case 4: // noise
                val = ((float)(rand() % 2000) / 1000.0f) - 1.0f;
                break;
            case 5: // pulse
                val = fmodf(phase / TWO_PI, 1.0f) < osc.pulseWidth ? 1.0f : -1.0f;
                break;
            default:
                val = sinf(phase);
                break;
        }
        sample += val * osc.amplitude;
    }
    return sample * envValue * preset.masterVolume;
}

float SoundDesigner::computeEnvelope(const SoundEnvelope& env, float time, float duration) {
    if (time < 0.0f) return 0.0f;

    // Attack phase
    if (time < env.attack) {
        float t = (env.attack > 0.0f) ? (time / env.attack) : 1.0f;
        return std::pow(t, env.attackCurve);
    }
    time -= env.attack;

    // Decay phase
    if (time < env.decay) {
        float t = (env.decay > 0.0f) ? (time / env.decay) : 1.0f;
        float curved = std::pow(t, env.decayCurve);
        return 1.0f - (1.0f - env.sustain) * curved;
    }
    time -= env.decay;

    // Sustain phase
    float sustainTime = duration - env.attack - env.decay - env.release;
    if (sustainTime < 0.0f) sustainTime = 0.0f;
    if (time < sustainTime) {
        return env.sustain;
    }
    time -= sustainTime;

    // Release phase
    if (time < env.release) {
        float t = (env.release > 0.0f) ? (time / env.release) : 1.0f;
        return env.sustain * (1.0f - t);
    }

    return 0.0f;
}

void SoundDesigner::generatePreviewBuffer() {
    if (m_activePresetIdx < 0 || m_activePresetIdx >= (int)m_presets.size()) return;
    const auto& preset = m_presets[m_activePresetIdx];
    float dur = std::max(preset.duration, 0.01f);

    for (int i = 0; i < kPreviewSamples; ++i) {
        float time = ((float)i / (float)kPreviewSamples) * dur;
        float envVal = computeEnvelope(preset.envelope, time, dur);

        // Apply LFO modulation to envelope amplitude
        if (preset.lfo.amplitude > 0.001f && preset.lfo.frequency > 0.001f && preset.lfo.target == 1) {
            float lfoPhase = time * preset.lfo.frequency * TWO_PI;
            float lfoVal = 0.0f;
            switch (preset.lfo.waveform) {
                case 0: lfoVal = sinf(lfoPhase); break;
                case 1: lfoVal = sinf(lfoPhase) > 0 ? 1.0f : -1.0f; break;
                case 2: lfoVal = 2.0f * fmodf(lfoPhase / TWO_PI, 1.0f) - 1.0f; break;
                case 3: lfoVal = 4.0f * fabsf(fmodf(lfoPhase / TWO_PI, 1.0f) - 0.5f) - 1.0f; break;
                default: lfoVal = sinf(lfoPhase); break;
            }
            envVal *= (1.0f + lfoVal * preset.lfo.amplitude * 0.5f);
        }

        m_previewBuffer[i] = generateSample(preset, time, envVal);
        m_previewBuffer[i] = std::clamp(m_previewBuffer[i], -1.0f, 1.0f);
    }
    m_previewGenerated = true;
}

// ---------------------------------------------------------------------------
// Save / Load
// ---------------------------------------------------------------------------

void SoundDesigner::savePresets() {
    std::ofstream out("sound_presets.dat", std::ios::binary);
    if (!out.is_open()) return;

    int count = (int)m_presets.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(int));

    for (int i = 0; i < count; ++i) {
        const auto& p = m_presets[i];
        int nameLen = (int)p.name.size();
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(int));
        out.write(p.name.c_str(), nameLen);
        int catLen = (int)p.category.size();
        out.write(reinterpret_cast<const char*>(&catLen), sizeof(int));
        out.write(p.category.c_str(), catLen);
        out.write(reinterpret_cast<const char*>(&p.oscillators[0]), sizeof(SoundOscillator));
        out.write(reinterpret_cast<const char*>(&p.oscillators[1]), sizeof(SoundOscillator));
        out.write(reinterpret_cast<const char*>(&p.envelope), sizeof(SoundEnvelope));
        out.write(reinterpret_cast<const char*>(&p.filter), sizeof(SoundFilter));
        int fxCount = (int)p.effects.size();
        out.write(reinterpret_cast<const char*>(&fxCount), sizeof(int));
        for (int j = 0; j < fxCount; ++j) {
            out.write(reinterpret_cast<const char*>(&p.effects[j]), sizeof(SoundEffect));
        }
        out.write(reinterpret_cast<const char*>(&p.lfo), sizeof(SoundLFO));
        out.write(reinterpret_cast<const char*>(&p.masterVolume), sizeof(float));
        out.write(reinterpret_cast<const char*>(&p.pan), sizeof(float));
        out.write(reinterpret_cast<const char*>(&p.pitchBendRange), sizeof(float));
        out.write(reinterpret_cast<const char*>(&p.duration), sizeof(float));
        out.write(reinterpret_cast<const char*>(&p.loop), sizeof(bool));
    }
    out.close();
}

void SoundDesigner::loadPresets() {
    std::ifstream in("sound_presets.dat", std::ios::binary);
    if (!in.is_open()) return;

    int count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(int));
    if (count <= 0 || count > 1000) return;

    m_presets.clear();
    for (int i = 0; i < count; ++i) {
        SoundPreset p;
        int nameLen = 0;
        in.read(reinterpret_cast<char*>(&nameLen), sizeof(int));
        if (nameLen > 0 && nameLen < 256) {
            p.name.resize(nameLen);
            in.read(&p.name[0], nameLen);
        }
        int catLen = 0;
        in.read(reinterpret_cast<char*>(&catLen), sizeof(int));
        if (catLen > 0 && catLen < 256) {
            p.category.resize(catLen);
            in.read(&p.category[0], catLen);
        }
        in.read(reinterpret_cast<char*>(&p.oscillators[0]), sizeof(SoundOscillator));
        in.read(reinterpret_cast<char*>(&p.oscillators[1]), sizeof(SoundOscillator));
        in.read(reinterpret_cast<char*>(&p.envelope), sizeof(SoundEnvelope));
        in.read(reinterpret_cast<char*>(&p.filter), sizeof(SoundFilter));
        int fxCount = 0;
        in.read(reinterpret_cast<char*>(&fxCount), sizeof(int));
        fxCount = std::clamp(fxCount, 0, 4);
        for (int j = 0; j < fxCount; ++j) {
            SoundEffect fx;
            in.read(reinterpret_cast<char*>(&fx), sizeof(SoundEffect));
            p.effects.push_back(fx);
        }
        in.read(reinterpret_cast<char*>(&p.lfo), sizeof(SoundLFO));
        in.read(reinterpret_cast<char*>(&p.masterVolume), sizeof(float));
        in.read(reinterpret_cast<char*>(&p.pan), sizeof(float));
        in.read(reinterpret_cast<char*>(&p.pitchBendRange), sizeof(float));
        in.read(reinterpret_cast<char*>(&p.duration), sizeof(float));
        in.read(reinterpret_cast<char*>(&p.loop), sizeof(bool));
        m_presets.push_back(p);
    }
    in.close();
}

// ---------------------------------------------------------------------------
// Undo / Redo
// ---------------------------------------------------------------------------

void SoundDesigner::pushUndo() {
    UndoState state;
    state.presets = m_presets;
    state.activeIdx = m_activePresetIdx;
    m_undoStack.push_back(state);
    if ((int)m_undoStack.size() > kMaxUndoSteps)
        m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();
    m_dirty = true;
}

void SoundDesigner::undo() {
    if (m_undoStack.empty()) return;
    UndoState redo;
    redo.presets = m_presets;
    redo.activeIdx = m_activePresetIdx;
    m_redoStack.push_back(redo);

    const auto& state = m_undoStack.back();
    m_presets = state.presets;
    m_activePresetIdx = state.activeIdx;
    m_undoStack.pop_back();
    m_dirty = !m_undoStack.empty();
    m_previewGenerated = false;
}

void SoundDesigner::redo() {
    if (m_redoStack.empty()) return;
    UndoState undoState;
    undoState.presets = m_presets;
    undoState.activeIdx = m_activePresetIdx;
    m_undoStack.push_back(undoState);

    const auto& state = m_redoStack.back();
    m_presets = state.presets;
    m_activePresetIdx = state.activeIdx;
    m_redoStack.pop_back();
    m_dirty = true;
    m_previewGenerated = false;
}

// ---------------------------------------------------------------------------
// Main UI
// ---------------------------------------------------------------------------

void SoundDesigner::show(bool* open) {
    if (!*open) return;
    if (!m_initialized) {
        m_categories = {"All", "SFX", "Music", "Ambient", "UI", "Custom"};
        loadPresets();
        if (m_presets.empty()) initDefaults();
        m_initialized = true;
        generatePreviewBuffer();
    }

    ImVec2 mvCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(mvCenter, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(1200, 750), ImGuiCond_FirstUseEver);

    bool windowOpen = true;
    if (!ImGui::Begin("Sound Designer", &windowOpen)) {
        ImGui::End();
        if (!windowOpen) *open = false;
        return;
    }
    if (!windowOpen) *open = false;

    // Save notification
    if (m_saveNotifyTimer > 0.0f) {
        m_saveNotifyTimer -= ImGui::GetIO().DeltaTime;
        float alpha = std::min(1.0f, m_saveNotifyTimer * 2.0f);
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        const char* msg = "Saved!";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        float nx = winPos.x + winSize.x - ts.x - 20;
        float ny = winPos.y + 8;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(nx - 8, ny - 4), ImVec2(nx + ts.x + 8, ny + ts.y + 4),
            IM_COL32(40, 120, 60, (int)(alpha * 200)), 4.0f);
        ImGui::GetWindowDrawList()->AddText(ImVec2(nx, ny),
            IM_COL32(255, 255, 255, (int)(alpha * 255)), msg);
    }

    // Simulate playback advancement
    if (m_isPlaying && m_activePresetIdx >= 0 && m_activePresetIdx < (int)m_presets.size()) {
        m_playbackTime += ImGui::GetIO().DeltaTime;
        float dur = m_presets[m_activePresetIdx].duration;
        if (m_playbackTime >= dur) {
            if (m_presets[m_activePresetIdx].loop) {
                m_playbackTime = fmodf(m_playbackTime, dur);
            } else {
                m_isPlaying = false;
                m_playbackTime = 0.0f;
            }
        }
    }

    // Ctrl+Z / Ctrl+Y
    auto& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) redo();

    // =========================================================================
    // LEFT PANEL: Preset list with categories
    // =========================================================================
    ImGui::BeginChild("##SndLeftPanel", ImVec2(220, 0), true);
    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "SOUND PRESETS");
    ImGui::Separator();

    // Category filter buttons
    {
        float btnW = (ImGui::GetContentRegionAvail().x - 10.0f) / 3.0f;
        for (int ci = 0; ci < (int)m_categories.size(); ++ci) {
            if (ci > 0 && ci % 3 != 0) ImGui::SameLine();
            bool isSel = (m_selectedCategory == ci);
            if (isSel)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));

            if (ImGui::Button(m_categories[ci].c_str(), ImVec2(btnW, 22))) {
                m_selectedCategory = ci;
            }
            ImGui::PopStyleColor();
        }
    }

    ImGui::Spacing();

    // Search filter
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##sndSearch", "Search...", m_searchFilter, sizeof(m_searchFilter));
    ImGui::Spacing();
    ImGui::Separator();

    // Preset list
    ImGui::BeginChild("##SndPresetList", ImVec2(0, -180), false);
    for (int i = 0; i < (int)m_presets.size(); ++i) {
        const auto& preset = m_presets[i];

        // Category filter
        if (m_selectedCategory > 0 && m_selectedCategory < (int)m_categories.size()) {
            if (preset.category != m_categories[m_selectedCategory]) continue;
        }

        // Search filter
        if (m_searchFilter[0] != '\0') {
            std::string nameLower = preset.name;
            std::string filterLower = m_searchFilter;
            for (auto& c : nameLower) c = (char)tolower((unsigned char)c);
            for (auto& c : filterLower) c = (char)tolower((unsigned char)c);
            if (nameLower.find(filterLower) == std::string::npos) continue;
        }

        ImGui::PushID(i);
        bool sel = (m_activePresetIdx == i);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.5f, 0.8f, 0.5f));

        // Category color badge
        ImVec4 catCol(0.6f, 0.6f, 0.6f, 1.0f);
        if (preset.category == "SFX")     catCol = ImVec4(0.7f, 0.5f, 0.3f, 1.0f);
        if (preset.category == "Music")   catCol = ImVec4(0.8f, 0.4f, 0.8f, 1.0f);
        if (preset.category == "Ambient") catCol = ImVec4(0.3f, 0.6f, 0.8f, 1.0f);
        if (preset.category == "UI")      catCol = ImVec4(0.8f, 0.8f, 0.3f, 1.0f);
        if (preset.category == "Custom")  catCol = ImVec4(0.5f, 0.8f, 0.5f, 1.0f);
        ImGui::TextColored(catCol, "[%s]", preset.category.c_str());
        ImGui::SameLine();

        if (ImGui::Selectable(preset.name.c_str(), sel)) {
            if (m_activePresetIdx != i) {
                m_activePresetIdx = i;
                m_previewGenerated = false;
                m_isPlaying = false;
                m_playbackTime = 0.0f;
                generatePreviewBuffer();
            }
        }

        if (sel) ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Preset management buttons
    if (ImGui::Button("+ Add", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, 26))) {
        pushUndo();
        SoundPreset p;
        p.name = "New Sound";
        p.category = (m_selectedCategory > 0 && m_selectedCategory < (int)m_categories.size())
            ? m_categories[m_selectedCategory] : "SFX";
        p.effects.clear();
        m_presets.push_back(p);
        m_activePresetIdx = (int)m_presets.size() - 1;
        m_previewGenerated = false;
        generatePreviewBuffer();
    }
    ImGui::SameLine();
    if (m_activePresetIdx >= 0 && m_activePresetIdx < (int)m_presets.size() && (int)m_presets.size() > 1) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("- Delete", ImVec2(-1, 26))) {
            pushUndo();
            m_presets.erase(m_presets.begin() + m_activePresetIdx);
            if (m_activePresetIdx >= (int)m_presets.size())
                m_activePresetIdx = (int)m_presets.size() - 1;
            m_previewGenerated = false;
            if (m_activePresetIdx >= 0) generatePreviewBuffer();
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::InvisibleButton("##noDel", ImVec2(-1, 26));
    }

    // Copy / Paste
    {
        float halfW = ImGui::GetContentRegionAvail().x * 0.5f - 2;
        if (m_activePresetIdx >= 0 && m_activePresetIdx < (int)m_presets.size()) {
            if (ImGui::Button("Copy", ImVec2(halfW, 24))) {
                m_clipboard = m_presets[m_activePresetIdx];
                m_hasClipboard = true;
            }
        } else {
            ImGui::InvisibleButton("##noCopy", ImVec2(halfW, 24));
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, m_hasClipboard ? ImVec4(0.3f, 0.6f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        if (ImGui::Button("Paste", ImVec2(-1, 24)) && m_hasClipboard && m_activePresetIdx >= 0 && m_activePresetIdx < (int)m_presets.size()) {
            pushUndo();
            std::string keepName = m_presets[m_activePresetIdx].name;
            m_presets[m_activePresetIdx] = m_clipboard;
            m_presets[m_activePresetIdx].name = keepName;
            m_previewGenerated = false;
            generatePreviewBuffer();
        }
        ImGui::PopStyleColor();
    }

    // Duplicate
    if (m_activePresetIdx >= 0 && m_activePresetIdx < (int)m_presets.size()) {
        if (ImGui::Button("Duplicate", ImVec2(-1, 24))) {
            pushUndo();
            SoundPreset copy = m_presets[m_activePresetIdx];
            copy.name += " Copy";
            m_presets.push_back(copy);
            m_activePresetIdx = (int)m_presets.size() - 1;
            m_previewGenerated = false;
            generatePreviewBuffer();
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    // Guard: no selection
    if (m_activePresetIdx < 0 || m_activePresetIdx >= (int)m_presets.size()) {
        ImGui::TextDisabled("Select a preset or add a new one.");
        ImGui::End();
        return;
    }

    // =========================================================================
    // RIGHT PANEL: Editor with tabs
    // =========================================================================
    ImGui::BeginChild("##SndRightPanel", ImVec2(0, 0), true);
    {
        auto& preset = m_presets[m_activePresetIdx];
        SoundPreset preEdit = preset;

        // Toolbar
        {
            ImGui::PushStyleColor(ImGuiCol_Button, m_dirty ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            if (ImGui::Button("Save", ImVec2(70, 28)) && m_dirty) {
                savePresets();
                m_dirty = false;
                m_saveNotifyTimer = 2.0f;
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, m_undoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
            if (ImGui::Button("Undo", ImVec2(55, 28)) && !m_undoStack.empty()) undo();
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, m_redoStack.empty() ? ImVec4(0.3f,0.3f,0.3f,0.5f) : ImVec4(0.3f,0.5f,0.8f,1.0f));
            if (ImGui::Button("Redo", ImVec2(55, 28)) && !m_redoStack.empty()) redo();
            ImGui::PopStyleColor();

            if (m_dirty) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " * Modified");
            }
        }
        ImGui::Separator();

        // Name and category
        {
            char nameBuf[64];
            strncpy(nameBuf, preset.name.c_str(), 63); nameBuf[63] = '\0';
            ImGui::SetNextItemWidth(300);
            if (ImGui::InputText("Name", nameBuf, 64)) preset.name = nameBuf;
        }

        // Tab bar
        if (ImGui::BeginTabBar("##SndTabs", ImGuiTabBarFlags_None)) {

            // =================================================================
            // TAB: Oscillators
            // =================================================================
            if (ImGui::BeginTabItem("Oscillators")) {
                ImGui::Spacing();
                const char* waveformNames[] = {"Sine", "Square", "Sawtooth", "Triangle", "Noise", "Pulse"};

                for (int oscIdx = 0; oscIdx < 2; ++oscIdx) {
                    ImGui::PushID(200 + oscIdx);
                    auto& osc = preset.oscillators[oscIdx];

                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "OSCILLATOR %d", oscIdx + 1);
                    ImGui::Separator();

                    ImGui::SetNextItemWidth(200);
                    ImGui::Combo("Waveform", &osc.waveform, waveformNames, 6);
                    ImGui::DragFloat("Frequency", &osc.frequency, 1.0f, 20.0f, 20000.0f, "%.1f Hz");
                    ImGui::DragFloat("Amplitude", &osc.amplitude, 0.005f, 0.0f, 1.0f, "%.3f");
                    ImGui::DragFloat("Detune", &osc.detune, 0.1f, -100.0f, 100.0f, "%.1f cents");
                    ImGui::DragFloat("Phase", &osc.phase, 0.01f, 0.0f, TWO_PI, "%.3f rad");
                    if (osc.waveform == 5) {
                        ImGui::DragFloat("Pulse Width", &osc.pulseWidth, 0.005f, 0.01f, 0.99f, "%.3f");
                    }

                    // Visual waveform preview for this oscillator
                    {
                        float oscPreview[128];
                        float singleCycleDur = (osc.frequency > 0.0f) ? (1.0f / osc.frequency) : 0.01f;
                        for (int s = 0; s < 128; ++s) {
                            float t = ((float)s / 128.0f) * singleCycleDur * 2.0f;
                            float freq = osc.frequency * (1.0f + osc.detune * 0.01f);
                            float ph = t * freq * TWO_PI + osc.phase;
                            float val = 0.0f;
                            switch (osc.waveform) {
                                case 0: val = sinf(ph); break;
                                case 1: val = sinf(ph) > 0 ? 1.0f : -1.0f; break;
                                case 2: val = 2.0f * fmodf(ph / TWO_PI, 1.0f) - 1.0f; break;
                                case 3: val = 4.0f * fabsf(fmodf(ph / TWO_PI, 1.0f) - 0.5f) - 1.0f; break;
                                case 4: val = ((float)(rand() % 2000) / 1000.0f) - 1.0f; break;
                                case 5: val = fmodf(ph / TWO_PI, 1.0f) < osc.pulseWidth ? 1.0f : -1.0f; break;
                                default: val = sinf(ph); break;
                            }
                            oscPreview[s] = val * osc.amplitude;
                        }

                        char overlayBuf[32];
                        snprintf(overlayBuf, sizeof(overlayBuf), "%.0f Hz", osc.frequency);
                        ImGui::PlotLines("##oscWave", oscPreview, 128, 0, overlayBuf, -1.0f, 1.0f, ImVec2(ImGui::GetContentRegionAvail().x, 60));
                    }

                    ImGui::Spacing();
                    ImGui::PopID();
                }

                ImGui::EndTabItem();
            }

            // =================================================================
            // TAB: Envelope
            // =================================================================
            if (ImGui::BeginTabItem("Envelope")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "ADSR ENVELOPE");
                ImGui::Separator();

                ImGui::DragFloat("Attack", &preset.envelope.attack, 0.001f, 0.0f, 5.0f, "%.3f s");
                ImGui::DragFloat("Decay", &preset.envelope.decay, 0.001f, 0.0f, 5.0f, "%.3f s");
                ImGui::DragFloat("Sustain", &preset.envelope.sustain, 0.005f, 0.0f, 1.0f, "%.3f");
                ImGui::DragFloat("Release", &preset.envelope.release, 0.005f, 0.0f, 10.0f, "%.3f s");

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "CURVE SHAPE");
                ImGui::Separator();
                ImGui::DragFloat("Attack Curve", &preset.envelope.attackCurve, 0.01f, 0.1f, 5.0f, "%.2f");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("1.0 = linear, <1 = logarithmic, >1 = exponential");
                ImGui::DragFloat("Decay Curve", &preset.envelope.decayCurve, 0.01f, 0.1f, 5.0f, "%.2f");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("1.0 = linear, <1 = logarithmic, >1 = exponential");

                // Visual ADSR curve drawn using ImDrawList
                ImGui::Spacing();
                {
                    float graphW = ImGui::GetContentRegionAvail().x - 8;
                    if (graphW < 100.0f) graphW = 100.0f;
                    float graphH = 150.0f;

                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImDrawList* dl = ImGui::GetWindowDrawList();

                    // Background
                    dl->AddRectFilled(pos, ImVec2(pos.x + graphW, pos.y + graphH), IM_COL32(25, 25, 35, 255));
                    dl->AddRect(pos, ImVec2(pos.x + graphW, pos.y + graphH), IM_COL32(80, 80, 100, 200));

                    float totalTime = preset.envelope.attack + preset.envelope.decay + preset.duration + preset.envelope.release;
                    if (totalTime <= 0.0f) totalTime = 1.0f;

                    float mx = 10.0f, my = 10.0f;
                    float gw = graphW - mx * 2.0f;
                    float gh = graphH - my * 2.0f;

                    auto timeToX = [&](float t) -> float { return pos.x + mx + (t / totalTime) * gw; };
                    auto ampToY = [&](float a) -> float { return pos.y + my + (1.0f - std::clamp(a, 0.0f, 1.0f)) * gh; };

                    // Grid lines
                    for (int gi = 1; gi < 4; ++gi) {
                        float gy = ampToY((float)gi / 4.0f);
                        dl->AddLine(ImVec2(pos.x + mx, gy), ImVec2(pos.x + mx + gw, gy),
                                    IM_COL32(50, 50, 65, 150), 1.0f);
                    }

                    // Draw the envelope using line segments with curve sampling
                    const int segmentsPerPhase = 32;
                    ImVec2 prevPt(timeToX(0.0f), ampToY(0.0f));

                    // Attack - draw curved segments
                    for (int s = 1; s <= segmentsPerPhase; ++s) {
                        float t = preset.envelope.attack * (float)s / (float)segmentsPerPhase;
                        float norm = (preset.envelope.attack > 0.0f) ? (t / preset.envelope.attack) : 1.0f;
                        float val = std::pow(norm, preset.envelope.attackCurve);
                        ImVec2 curPt(timeToX(t), ampToY(val));
                        dl->AddLine(prevPt, curPt, IM_COL32(60, 200, 120, 255), 2.5f);
                        prevPt = curPt;
                    }

                    // Decay
                    float decayStart = preset.envelope.attack;
                    for (int s = 1; s <= segmentsPerPhase; ++s) {
                        float dt = preset.envelope.decay * (float)s / (float)segmentsPerPhase;
                        float norm = (preset.envelope.decay > 0.0f) ? (dt / preset.envelope.decay) : 1.0f;
                        float curved = std::pow(norm, preset.envelope.decayCurve);
                        float val = 1.0f - (1.0f - preset.envelope.sustain) * curved;
                        ImVec2 curPt(timeToX(decayStart + dt), ampToY(val));
                        dl->AddLine(prevPt, curPt, IM_COL32(220, 180, 50, 255), 2.5f);
                        prevPt = curPt;
                    }

                    // Sustain
                    float sustainStart = decayStart + preset.envelope.decay;
                    float sustainEnd = totalTime - preset.envelope.release;
                    ImVec2 sustEndPt(timeToX(sustainEnd), ampToY(preset.envelope.sustain));
                    dl->AddLine(prevPt, sustEndPt, IM_COL32(60, 150, 220, 255), 2.5f);
                    prevPt = sustEndPt;

                    // Release
                    for (int s = 1; s <= segmentsPerPhase; ++s) {
                        float rt = preset.envelope.release * (float)s / (float)segmentsPerPhase;
                        float norm = (preset.envelope.release > 0.0f) ? (rt / preset.envelope.release) : 1.0f;
                        float val = preset.envelope.sustain * (1.0f - norm);
                        ImVec2 curPt(timeToX(sustainEnd + rt), ampToY(val));
                        dl->AddLine(prevPt, curPt, IM_COL32(220, 80, 80, 255), 2.5f);
                        prevPt = curPt;
                    }

                    // Key points
                    dl->AddCircleFilled(ImVec2(timeToX(0.0f), ampToY(0.0f)), 4.0f, IM_COL32(180, 180, 180, 255));
                    dl->AddCircleFilled(ImVec2(timeToX(preset.envelope.attack), ampToY(1.0f)), 4.0f, IM_COL32(60, 200, 120, 255));
                    dl->AddCircleFilled(ImVec2(timeToX(sustainStart), ampToY(preset.envelope.sustain)), 4.0f, IM_COL32(220, 180, 50, 255));
                    dl->AddCircleFilled(ImVec2(timeToX(sustainEnd), ampToY(preset.envelope.sustain)), 4.0f, IM_COL32(60, 150, 220, 255));
                    dl->AddCircleFilled(ImVec2(timeToX(totalTime), ampToY(0.0f)), 4.0f, IM_COL32(220, 80, 80, 255));

                    // Labels
                    dl->AddText(ImVec2(timeToX(preset.envelope.attack) - 4, ampToY(1.0f) - 16),
                                IM_COL32(60, 200, 120, 220), "A");
                    dl->AddText(ImVec2(timeToX(sustainStart) - 4, ampToY(preset.envelope.sustain) - 16),
                                IM_COL32(220, 180, 50, 220), "D");
                    dl->AddText(ImVec2((timeToX(sustainStart) + timeToX(sustainEnd)) * 0.5f - 4,
                                ampToY(preset.envelope.sustain) - 16),
                                IM_COL32(60, 150, 220, 220), "S");
                    dl->AddText(ImVec2(timeToX(totalTime) - 8, ampToY(0.0f) - 16),
                                IM_COL32(220, 80, 80, 220), "R");

                    ImGui::Dummy(ImVec2(graphW, graphH));
                }

                // Envelope info
                float envTotal = preset.envelope.attack + preset.envelope.decay + preset.duration + preset.envelope.release;
                ImGui::TextDisabled("Total envelope time: %.3f s", envTotal);

                ImGui::EndTabItem();
            }

            // =================================================================
            // TAB: Filter
            // =================================================================
            if (ImGui::BeginTabItem("Filter")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "FILTER");
                ImGui::Separator();

                const char* filterNames[] = {"None", "Low-Pass", "High-Pass", "Band-Pass", "Notch"};
                ImGui::SetNextItemWidth(200);
                ImGui::Combo("Filter Type", &preset.filter.filterType, filterNames, 5);
                ImGui::DragFloat("Cutoff Frequency", &preset.filter.cutoff, 5.0f, 20.0f, 20000.0f, "%.0f Hz");
                ImGui::DragFloat("Resonance", &preset.filter.resonance, 0.005f, 0.0f, 1.0f, "%.3f");
                ImGui::DragFloat("Envelope Amount", &preset.filter.envelopeAmount, 0.005f, -1.0f, 1.0f, "%.3f");

                // Visual frequency response curve
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "FREQUENCY RESPONSE");
                {
                    float respPreview[256];
                    float cutoffNorm = preset.filter.cutoff / 20000.0f;
                    float res = preset.filter.resonance;
                    int fType = preset.filter.filterType;

                    for (int s = 0; s < 256; ++s) {
                        float freqNorm = (float)s / 256.0f; // 0 to 1, representing 20Hz to 20kHz
                        float logFreq = 20.0f * std::pow(1000.0f, freqNorm); // log scale
                        float logCutoff = preset.filter.cutoff;
                        float ratio = logFreq / std::max(logCutoff, 1.0f);
                        float response = 1.0f;

                        switch (fType) {
                            case 0: // none
                                response = 1.0f;
                                break;
                            case 1: // lowpass
                            {
                                float r2 = ratio * ratio;
                                float resBoost = 1.0f + res * 4.0f;
                                response = 1.0f / std::sqrt(1.0f + r2 * r2 - 2.0f * r2 * (2.0f * resBoost - 1.0f) / (resBoost * resBoost));
                                response = std::clamp(response, 0.0f, 2.0f);
                                break;
                            }
                            case 2: // highpass
                            {
                                float invRatio = 1.0f / std::max(ratio, 0.001f);
                                float r2 = invRatio * invRatio;
                                float resBoost = 1.0f + res * 4.0f;
                                response = 1.0f / std::sqrt(1.0f + r2 * r2 - 2.0f * r2 * (2.0f * resBoost - 1.0f) / (resBoost * resBoost));
                                response = std::clamp(response, 0.0f, 2.0f);
                                break;
                            }
                            case 3: // bandpass
                            {
                                float bw = 0.5f * (1.0f - res * 0.9f);
                                float diff = std::abs(ratio - 1.0f);
                                response = std::exp(-diff * diff / (2.0f * bw * bw));
                                response = std::clamp(response, 0.0f, 1.0f);
                                break;
                            }
                            case 4: // notch
                            {
                                float bw = 0.3f * (1.0f - res * 0.9f);
                                float diff = std::abs(ratio - 1.0f);
                                float notch = std::exp(-diff * diff / (2.0f * bw * bw));
                                response = 1.0f - notch;
                                response = std::clamp(response, 0.0f, 1.0f);
                                break;
                            }
                        }
                        respPreview[s] = response;
                    }

                    char overlayBuf[64];
                    snprintf(overlayBuf, sizeof(overlayBuf), "%s  Cutoff: %.0f Hz",
                             filterNames[std::clamp(preset.filter.filterType, 0, 4)], preset.filter.cutoff);
                    ImGui::PlotLines("##filterResp", respPreview, 256, 0, overlayBuf,
                                     0.0f, 2.0f, ImVec2(ImGui::GetContentRegionAvail().x, 120));
                }

                ImGui::EndTabItem();
            }

            // =================================================================
            // TAB: Effects
            // =================================================================
            if (ImGui::BeginTabItem("Effects")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "EFFECTS CHAIN (up to 4)");
                ImGui::Separator();

                const char* effectNames[] = {
                    "None", "Reverb", "Delay", "Chorus", "Distortion",
                    "Flanger", "Phaser", "Tremolo", "Vibrato"
                };

                // Param display names per effect type
                auto getParam1Name = [](int type) -> const char* {
                    switch (type) {
                        case 1: return "Room Size";
                        case 2: return "Delay Time";
                        case 3: return "Rate";
                        case 4: return "Drive";
                        case 5: return "Rate";
                        case 6: return "Rate";
                        case 7: return "Rate";
                        case 8: return "Rate";
                        default: return "Param 1";
                    }
                };
                auto getParam2Name = [](int type) -> const char* {
                    switch (type) {
                        case 1: return "Damping";
                        case 2: return "Feedback";
                        case 3: return "Depth";
                        case 4: return "Tone";
                        case 5: return "Depth";
                        case 6: return "Depth";
                        case 7: return "Depth";
                        case 8: return "Depth";
                        default: return "Param 2";
                    }
                };

                // Ensure we have up to 4 effect slots
                while ((int)preset.effects.size() < 4) {
                    SoundEffect fx;
                    fx.effectType = 0;
                    fx.mix = 0.3f;
                    fx.parameter1 = 0.5f;
                    fx.parameter2 = 0.5f;
                    fx.enabled = false;
                    preset.effects.push_back(fx);
                }

                for (int fi = 0; fi < 4; ++fi) {
                    ImGui::PushID(300 + fi);
                    auto& fx = preset.effects[fi];

                    ImVec4 hdrCol = fx.enabled ? ImVec4(0.3f, 0.7f, 0.9f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 0.6f);
                    ImGui::TextColored(hdrCol, "Effect Slot %d", fi + 1);
                    ImGui::SameLine();
                    ImGui::Checkbox("##fxEn", &fx.enabled);

                    if (fx.enabled) {
                        ImGui::Indent(16.0f);
                        ImGui::SetNextItemWidth(200);
                        ImGui::Combo("Type", &fx.effectType, effectNames, 9);
                        if (fx.effectType > 0) {
                            ImGui::DragFloat("Mix (Wet/Dry)", &fx.mix, 0.005f, 0.0f, 1.0f, "%.3f");
                            ImGui::DragFloat(getParam1Name(fx.effectType), &fx.parameter1, 0.005f, 0.0f, 1.0f, "%.3f");
                            ImGui::DragFloat(getParam2Name(fx.effectType), &fx.parameter2, 0.005f, 0.0f, 1.0f, "%.3f");
                        }
                        ImGui::Unindent(16.0f);
                    }

                    ImGui::Spacing();
                    ImGui::PopID();
                }

                // Active effects count
                int activeCount = 0;
                for (int fi = 0; fi < (int)preset.effects.size(); ++fi) {
                    if (preset.effects[fi].enabled && preset.effects[fi].effectType > 0) activeCount++;
                }
                ImGui::TextDisabled("Active effects: %d / 4", activeCount);

                ImGui::EndTabItem();
            }

            // =================================================================
            // TAB: LFO
            // =================================================================
            if (ImGui::BeginTabItem("LFO")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "LOW FREQUENCY OSCILLATOR");
                ImGui::Separator();

                const char* lfoWaveNames[] = {"Sine", "Square", "Sawtooth", "Triangle"};
                const char* lfoTargetNames[] = {"Pitch", "Amplitude", "Filter Cutoff", "Pan"};

                ImGui::SetNextItemWidth(200);
                ImGui::Combo("Waveform##lfo", &preset.lfo.waveform, lfoWaveNames, 4);
                ImGui::DragFloat("Frequency##lfo", &preset.lfo.frequency, 0.01f, 0.0f, 50.0f, "%.2f Hz");
                ImGui::DragFloat("Amplitude##lfo", &preset.lfo.amplitude, 0.005f, 0.0f, 1.0f, "%.3f");
                ImGui::SetNextItemWidth(200);
                ImGui::Combo("Target##lfo", &preset.lfo.target, lfoTargetNames, 4);

                // Visual LFO waveform preview
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "LFO WAVEFORM PREVIEW");
                {
                    float lfoPreview[256];
                    float lfoFreq = std::max(preset.lfo.frequency, 0.01f);
                    float cycleTime = 2.0f / lfoFreq; // show ~2 cycles

                    for (int s = 0; s < 256; ++s) {
                        float t = ((float)s / 256.0f) * cycleTime;
                        float ph = t * lfoFreq * TWO_PI;
                        float val = 0.0f;
                        switch (preset.lfo.waveform) {
                            case 0: val = sinf(ph); break;
                            case 1: val = sinf(ph) > 0 ? 1.0f : -1.0f; break;
                            case 2: val = 2.0f * fmodf(ph / TWO_PI, 1.0f) - 1.0f; break;
                            case 3: val = 4.0f * fabsf(fmodf(ph / TWO_PI, 1.0f) - 0.5f) - 1.0f; break;
                            default: val = sinf(ph); break;
                        }
                        lfoPreview[s] = val * preset.lfo.amplitude;
                    }

                    char lfoOverlay[64];
                    snprintf(lfoOverlay, sizeof(lfoOverlay), "%.2f Hz -> %s",
                             preset.lfo.frequency, lfoTargetNames[std::clamp(preset.lfo.target, 0, 3)]);
                    ImGui::PlotLines("##lfoWave", lfoPreview, 256, 0, lfoOverlay,
                                     -1.0f, 1.0f, ImVec2(ImGui::GetContentRegionAvail().x, 80));
                }

                ImGui::EndTabItem();
            }

            // =================================================================
            // TAB: Master
            // =================================================================
            if (ImGui::BeginTabItem("Master")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "MASTER SETTINGS");
                ImGui::Separator();

                ImGui::DragFloat("Volume", &preset.masterVolume, 0.005f, 0.0f, 1.0f, "%.3f");

                // Volume bar
                {
                    ImVec2 barPos = ImGui::GetCursorScreenPos();
                    float barW = std::min(ImGui::GetContentRegionAvail().x - 8, 300.0f);
                    float barH = 14.0f;
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddRectFilled(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(30, 30, 40, 255));
                    float fillW = preset.masterVolume * barW;
                    ImU32 volCol = preset.masterVolume < 0.7f ? IM_COL32(60, 200, 120, 220) :
                                   preset.masterVolume < 0.9f ? IM_COL32(220, 180, 50, 220) :
                                   IM_COL32(220, 80, 80, 220);
                    dl->AddRectFilled(barPos, ImVec2(barPos.x + fillW, barPos.y + barH), volCol);
                    dl->AddRect(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(80, 80, 100, 200));
                    ImGui::Dummy(ImVec2(barW, barH + 4));
                }

                ImGui::DragFloat("Pan", &preset.pan, 0.005f, -1.0f, 1.0f, "%.3f");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("-1.0 = Left, 0.0 = Center, +1.0 = Right");

                // Pan indicator
                {
                    ImVec2 panPos = ImGui::GetCursorScreenPos();
                    float panW = std::min(ImGui::GetContentRegionAvail().x - 8, 200.0f);
                    float panH = 14.0f;
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddRectFilled(panPos, ImVec2(panPos.x + panW, panPos.y + panH), IM_COL32(30, 30, 40, 255));
                    float centerX = panPos.x + panW * 0.5f;
                    dl->AddLine(ImVec2(centerX, panPos.y), ImVec2(centerX, panPos.y + panH),
                                IM_COL32(100, 100, 120, 150), 1.0f);
                    float markerX = panPos.x + (preset.pan * 0.5f + 0.5f) * panW;
                    dl->AddRectFilled(ImVec2(markerX - 3, panPos.y), ImVec2(markerX + 3, panPos.y + panH),
                                      IM_COL32(80, 180, 255, 220));
                    dl->AddRect(panPos, ImVec2(panPos.x + panW, panPos.y + panH), IM_COL32(80, 80, 100, 200));
                    dl->AddText(ImVec2(panPos.x + 2, panPos.y - 1), IM_COL32(120, 120, 140, 180), "L");
                    dl->AddText(ImVec2(panPos.x + panW - 10, panPos.y - 1), IM_COL32(120, 120, 140, 180), "R");
                    ImGui::Dummy(ImVec2(panW, panH + 4));
                }

                ImGui::Spacing();
                ImGui::DragFloat("Pitch Bend Range", &preset.pitchBendRange, 0.1f, 0.0f, 24.0f, "%.1f semitones");
                ImGui::DragFloat("Duration", &preset.duration, 0.01f, 0.01f, 30.0f, "%.2f s");
                ImGui::Checkbox("Loop", &preset.loop);

                ImGui::Spacing();
                ImGui::Separator();

                // Category
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "CATEGORY");
                {
                    const char* catNames[] = {"SFX", "Music", "Ambient", "UI", "Custom"};
                    int catIdx = 0;
                    for (int i = 0; i < 5; i++) {
                        if (preset.category == catNames[i]) { catIdx = i; break; }
                    }
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::Combo("Category", &catIdx, catNames, 5)) {
                        preset.category = catNames[catIdx];
                    }
                }

                ImGui::EndTabItem();
            }

            // =================================================================
            // TAB: Preview
            // =================================================================
            if (ImGui::BeginTabItem("Preview")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "COMBINED OUTPUT PREVIEW");
                ImGui::Separator();

                // Transport controls
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, m_isPlaying ? ImVec4(0.7f, 0.3f, 0.2f, 1.0f) : ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_isPlaying ? ImVec4(0.8f, 0.4f, 0.3f, 1.0f) : ImVec4(0.3f, 0.7f, 0.4f, 1.0f));
                    if (ImGui::Button(m_isPlaying ? "Stop" : "Play", ImVec2(80, 30))) {
                        if (m_isPlaying) {
                            m_isPlaying = false;
                            m_playbackTime = 0.0f;
                        } else {
                            m_isPlaying = true;
                            m_playbackTime = 0.0f;
                            if (!m_previewGenerated) generatePreviewBuffer();
                        }
                    }
                    ImGui::PopStyleColor(2);

                    ImGui::SameLine();
                    if (ImGui::Button("Generate Preview", ImVec2(150, 30))) {
                        generatePreviewBuffer();
                    }

                    if (m_isPlaying) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "  %.2f / %.2f s",
                                           m_playbackTime, preset.duration);
                    }
                }

                ImGui::Spacing();

                // Large waveform display
                {
                    float pvW = ImGui::GetContentRegionAvail().x - 8;
                    if (pvW < 100.0f) pvW = 100.0f;
                    float pvH = 200.0f;

                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImDrawList* dl = ImGui::GetWindowDrawList();

                    // Background
                    dl->AddRectFilled(pos, ImVec2(pos.x + pvW, pos.y + pvH), IM_COL32(20, 22, 30, 255));
                    dl->AddRect(pos, ImVec2(pos.x + pvW, pos.y + pvH), IM_COL32(80, 80, 100, 200));

                    // Center line
                    float centerY = pos.y + pvH * 0.5f;
                    dl->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + pvW, centerY),
                                IM_COL32(50, 50, 70, 180), 1.0f);

                    // Quarter lines
                    dl->AddLine(ImVec2(pos.x, pos.y + pvH * 0.25f), ImVec2(pos.x + pvW, pos.y + pvH * 0.25f),
                                IM_COL32(40, 40, 55, 120), 1.0f);
                    dl->AddLine(ImVec2(pos.x, pos.y + pvH * 0.75f), ImVec2(pos.x + pvW, pos.y + pvH * 0.75f),
                                IM_COL32(40, 40, 55, 120), 1.0f);

                    if (m_previewGenerated) {
                        float halfH = pvH * 0.45f;
                        float drawW = pvW - 8.0f;
                        float mx = 4.0f;

                        // Draw filled waveform
                        ImVec2 prevPt(pos.x + mx, centerY);
                        for (int i = 0; i < kPreviewSamples; ++i) {
                            float x = pos.x + mx + ((float)i / (float)(kPreviewSamples - 1)) * drawW;
                            float y = centerY - m_previewBuffer[i] * halfH;
                            y = std::clamp(y, pos.y + 2.0f, pos.y + pvH - 2.0f);

                            // Filled from center
                            ImU32 fillCol = (m_previewBuffer[i] >= 0.0f)
                                ? IM_COL32(50, 160, 255, 40)
                                : IM_COL32(255, 100, 60, 40);
                            dl->AddLine(ImVec2(x, centerY), ImVec2(x, y), fillCol, 1.0f);

                            // Waveform line
                            ImVec2 curPt(x, y);
                            if (i > 0) {
                                dl->AddLine(prevPt, curPt, IM_COL32(80, 180, 255, 220), 1.5f);
                            }
                            prevPt = curPt;
                        }

                        // Playback cursor
                        if (m_isPlaying && preset.duration > 0.0f) {
                            float ratio = m_playbackTime / preset.duration;
                            if (ratio >= 0.0f && ratio <= 1.0f) {
                                float cx = pos.x + mx + ratio * drawW;
                                dl->AddLine(ImVec2(cx, pos.y + 2), ImVec2(cx, pos.y + pvH - 2),
                                            IM_COL32(255, 220, 50, 200), 2.0f);
                            }
                        }
                    } else {
                        dl->AddText(ImVec2(pos.x + pvW * 0.5f - 80, centerY - 6),
                                    IM_COL32(120, 120, 120, 200),
                                    "Click 'Generate Preview'");
                    }

                    // Amplitude labels
                    dl->AddText(ImVec2(pos.x + 4, pos.y + 2), IM_COL32(100, 100, 120, 180), "+1.0");
                    dl->AddText(ImVec2(pos.x + 4, centerY - 6), IM_COL32(100, 100, 120, 180), " 0.0");
                    dl->AddText(ImVec2(pos.x + 4, pos.y + pvH - 14), IM_COL32(100, 100, 120, 180), "-1.0");

                    ImGui::Dummy(ImVec2(pvW, pvH));
                }

                ImGui::Spacing();
                ImGui::Separator();

                // Sound summary
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "SOUND SUMMARY");
                ImGui::TextDisabled("Name: %s", preset.name.c_str());
                ImGui::TextDisabled("Category: %s", preset.category.c_str());
                ImGui::TextDisabled("Duration: %.2f s | Volume: %.0f%% | Pan: %.2f",
                                    preset.duration, preset.masterVolume * 100.0f, preset.pan);
                ImGui::TextDisabled("Loop: %s", preset.loop ? "Yes" : "No");

                // Oscillator summary
                const char* waveShort[] = {"SIN", "SQR", "SAW", "TRI", "NSE", "PLS"};
                for (int oi = 0; oi < 2; ++oi) {
                    const auto& osc = preset.oscillators[oi];
                    int wfIdx = std::clamp(osc.waveform, 0, 5);
                    ImGui::TextDisabled("  OSC %d: %s %.0f Hz  amp=%.2f  dt=%.1f",
                                        oi + 1, waveShort[wfIdx],
                                        osc.frequency, osc.amplitude, osc.detune);
                }
                ImGui::TextDisabled("  Env: A=%.3f D=%.3f S=%.2f R=%.3f",
                                    preset.envelope.attack, preset.envelope.decay,
                                    preset.envelope.sustain, preset.envelope.release);

                const char* filterShort[] = {"OFF", "LPF", "HPF", "BPF", "NCH"};
                int ftIdx = std::clamp(preset.filter.filterType, 0, 4);
                ImGui::TextDisabled("  Filter: %s  Cutoff=%.0f Hz  Res=%.2f",
                                    filterShort[ftIdx], preset.filter.cutoff, preset.filter.resonance);

                int activeEffects = 0;
                for (int fi = 0; fi < (int)preset.effects.size(); ++fi) {
                    if (preset.effects[fi].enabled && preset.effects[fi].effectType > 0) activeEffects++;
                }
                ImGui::TextDisabled("  Active effects: %d", activeEffects);

                if (preset.lfo.amplitude > 0.001f && preset.lfo.frequency > 0.001f) {
                    const char* lfoTargets[] = {"Pitch", "Amplitude", "Filter", "Pan"};
                    int ltIdx = std::clamp(preset.lfo.target, 0, 3);
                    ImGui::TextDisabled("  LFO: %.2f Hz -> %s  amp=%.2f",
                                        preset.lfo.frequency, lfoTargets[ltIdx], preset.lfo.amplitude);
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // Detect changes and push undo
        {
            bool changed = false;

            if (preset.name != preEdit.name) changed = true;
            if (preset.category != preEdit.category) changed = true;
            if (preset.masterVolume != preEdit.masterVolume) changed = true;
            if (preset.pan != preEdit.pan) changed = true;
            if (preset.pitchBendRange != preEdit.pitchBendRange) changed = true;
            if (preset.duration != preEdit.duration) changed = true;
            if (preset.loop != preEdit.loop) changed = true;

            // Check oscillators
            if (!changed) {
                for (int i = 0; i < 2; ++i) {
                    const auto& a = preset.oscillators[i];
                    const auto& b = preEdit.oscillators[i];
                    if (a.waveform != b.waveform || a.frequency != b.frequency ||
                        a.amplitude != b.amplitude || a.phase != b.phase ||
                        a.detune != b.detune || a.pulseWidth != b.pulseWidth) {
                        changed = true;
                        break;
                    }
                }
            }

            // Check envelope
            if (!changed) {
                const auto& a = preset.envelope;
                const auto& b = preEdit.envelope;
                if (a.attack != b.attack || a.decay != b.decay || a.sustain != b.sustain ||
                    a.release != b.release || a.attackCurve != b.attackCurve || a.decayCurve != b.decayCurve)
                    changed = true;
            }

            // Check filter
            if (!changed) {
                const auto& a = preset.filter;
                const auto& b = preEdit.filter;
                if (a.filterType != b.filterType || a.cutoff != b.cutoff ||
                    a.resonance != b.resonance || a.envelopeAmount != b.envelopeAmount)
                    changed = true;
            }

            // Check effects
            if (!changed) {
                if (preset.effects.size() != preEdit.effects.size()) {
                    changed = true;
                } else {
                    for (int i = 0; i < (int)preset.effects.size(); ++i) {
                        const auto& a = preset.effects[i];
                        const auto& b = preEdit.effects[i];
                        if (a.effectType != b.effectType || a.mix != b.mix ||
                            a.parameter1 != b.parameter1 || a.parameter2 != b.parameter2 ||
                            a.enabled != b.enabled) {
                            changed = true;
                            break;
                        }
                    }
                }
            }

            // Check LFO
            if (!changed) {
                const auto& a = preset.lfo;
                const auto& b = preEdit.lfo;
                if (a.waveform != b.waveform || a.frequency != b.frequency ||
                    a.amplitude != b.amplitude || a.target != b.target)
                    changed = true;
            }

            if (changed) {
                pushUndo();
                m_previewGenerated = false;
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}
