#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "Math.hpp"

// Forward declaration for miniaudio types to avoid including it in the header
struct ma_engine;

class AudioManager {
public:
    static AudioManager& getInstance();

    bool init(const std::string& rootPath = "");
    void shutdown();

    void playSound(const std::string& filePath, float volume = 1.0f);
    void playSoundAt(const std::string& filePath, Vec3 position, Vec3 listenerPos, float volume = 1.0f, float rollOff = 1.0f);
    void playAmbientMobSound(const std::string& mobType, Vec3 position, Vec3 listenerPos);
    void playBlockBreakSound(uint8_t blockType, Vec3 position, Vec3 listenerPos);
    void update(float dt);

    float getMasterVolume() const { return masterVolume; }
    void setMasterVolume(float v);
    float getMusicVolume() const { return musicVolume; }
    void setMusicVolume(float v);
    float getMobVolume() const { return mobVolume; }
    void setMobVolume(float v) { mobVolume = v; }
    float getBlockVolume() const { return blockVolume; }
    void setBlockVolume(float v) { blockVolume = v; }

    bool isMobSoundsEnabled() const { return mobSoundsEnabled; }
    void setMobSoundsEnabled(bool enabled) { mobSoundsEnabled = enabled; }
    bool isBlockSoundsEnabled() const { return blockSoundsEnabled; }
    void setBlockSoundsEnabled(bool enabled) { blockSoundsEnabled = enabled; }
    bool isMusicEnabled() const { return musicEnabled; }
    void setMusicEnabled(bool enabled);

    // Environmental Ambient Loops (Water, Lava, Fire, Rain)
    void setAmbientLoop(const std::string& key, const std::string& path, bool active, float volume = 1.0f);
    void playThunder(Vec3 position, Vec3 listenerPos);

private:
    AudioManager();
    ~AudioManager();

    ma_engine* engine = nullptr;
    void* currentMusic = nullptr; // ma_sound*
    std::unordered_map<std::string, void*> ambientLoops; // ma_sound*
    std::string projectRoot;
    std::unordered_map<std::string, std::string> mobSounds;
    std::unordered_map<uint8_t, std::string> blockSounds;

    float masterVolume = 0.8f;
    float musicVolume = 0.5f;
    float mobVolume = 0.7f;
    float blockVolume = 0.8f;
    float ambientVolume = 0.6f;
    bool mobSoundsEnabled = true;
    bool blockSoundsEnabled = true;
    bool musicEnabled = true;
    bool ambientEnabled = true;
    float musicTimer = 0.0f;
    std::vector<std::string> musicPlaylist;

    void loadSoundConfigs();
    void startNextMusic();
};
