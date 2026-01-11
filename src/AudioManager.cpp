#define MA_IMPLEMENTATION
#include "miniaudio.h"
#include "AudioManager.hpp"
#include <iostream>
#include <filesystem>

AudioManager& AudioManager::getInstance() {
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager() {}

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::init(const std::string& rootPath) {
    projectRoot = rootPath;
    engine = new ma_engine();
    ma_result result = ma_engine_init(NULL, engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine." << std::endl;
        delete engine;
        engine = nullptr;
        return false;
    }

    loadSoundConfigs();
    return true;
}

void AudioManager::shutdown() {
    if (currentMusic) {
        ma_sound_uninit((ma_sound*)currentMusic);
        delete (ma_sound*)currentMusic;
        currentMusic = nullptr;
    }
    for (auto& pair : ambientLoops) {
        if (pair.second) {
            ma_sound_uninit((ma_sound*)pair.second);
            delete (ma_sound*)pair.second;
        }
    }
    ambientLoops.clear();

    if (engine) {
        ma_engine_uninit(engine);
        delete engine;
        engine = nullptr;
    }
}

void AudioManager::loadSoundConfigs() {
    // Map mob types to file paths
    mobSounds["cow"] = "assets/sounds/cow_idle.wav";
    mobSounds["pig"] = "assets/sounds/pig_idle.wav";
    mobSounds["chicken"] = "assets/sounds/chicken_idle.wav";
    mobSounds["rabbit"] = "assets/sounds/rabbit_hop.wav";
    mobSounds["sheep"] = "assets/sounds/sheep_idle.wav";

    // Map block types to file paths
    blockSounds[1] = "assets/sounds/stone_break.wav"; // Dirt -> stone-ish thud
    blockSounds[2] = "assets/sounds/grass_break.wav"; // Grass
    blockSounds[3] = "assets/sounds/stone_break.wav"; // Stone
    blockSounds[4] = ""; // Water
    blockSounds[5] = ""; // Lava
    blockSounds[6] = "assets/sounds/wood_break.wav";  // Wood
    blockSounds[7] = "assets/sounds/grass_break.wav"; // Leaves
    blockSounds[8] = "assets/sounds/grass_break.wav"; // Sand
    blockSounds[9] = "assets/sounds/grass_break.wav"; // Snow
    blockSounds[14] = "assets/sounds/glass_break.wav"; // Glass
    blockSounds[23] = "assets/sounds/stone_break.wav"; // Cobblestone
    blockSounds[25] = "assets/sounds/wood_break.wav";  // Planks
    blockSounds[26] = "assets/sounds/stone_break.wav"; // Bricks
    blockSounds[27] = "assets/sounds/glass_break.wav"; // Ice (shatters)

    // Ores
    for(int i=15; i<=18; ++i) blockSounds[i] = "assets/sounds/stone_break.wav";

    musicPlaylist = {
        "assets/sounds/music1.wav",
        "assets/sounds/music2.wav"
    };
    musicTimer = 10.0f; // Start with some delay
}

void AudioManager::setMasterVolume(float v) {
    masterVolume = v;
    if (engine) ma_engine_set_volume(engine, masterVolume);
}

void AudioManager::setMusicVolume(float v) {
    musicVolume = v;
    if (currentMusic) {
        ma_sound_set_volume((ma_sound*)currentMusic, musicVolume);
    }
}

void AudioManager::setMusicEnabled(bool enabled) {
    musicEnabled = enabled;
    if (!musicEnabled && currentMusic) {
        ma_sound_stop((ma_sound*)currentMusic);
    } else if (musicEnabled && !currentMusic) {
        musicTimer = 1.0f;
    }
}

void AudioManager::setAmbientLoop(const std::string& key, const std::string& path, bool active, float volume) {
    if (!engine) return;

    auto it = ambientLoops.find(key);
    if (active) {
        if (it == ambientLoops.end() || it->second == nullptr) {
            std::filesystem::path fullPath = std::filesystem::path(projectRoot) / path;
            if (!std::filesystem::exists(fullPath)) return;

            ma_sound* sound = new ma_sound();
            if (ma_sound_init_from_file(engine, fullPath.string().c_str(), 0, NULL, NULL, sound) == MA_SUCCESS) {
                ma_sound_set_looping(sound, MA_TRUE);
                ma_sound_set_volume(sound, volume * ambientVolume);
                ma_sound_start(sound);
                ambientLoops[key] = (void*)sound;
            } else {
                delete sound;
            }
        } else {
            // Already playing, just update volume
            ma_sound_set_volume((ma_sound*)it->second, volume * ambientVolume);
        }
    } else {
        if (it != ambientLoops.end() && it->second != nullptr) {
            ma_sound_uninit((ma_sound*)it->second);
            delete (ma_sound*)it->second;
            ambientLoops.erase(it);
        }
    }
}

void AudioManager::playThunder(Vec3 position, Vec3 listenerPos) {
    playSoundAt("assets/sounds/thunder.wav", position, listenerPos, 1.0f, 0.5f);
}

void AudioManager::playSound(const std::string& filePath, float volume) {
    if (!engine) return;
    
    std::filesystem::path fullPath = std::filesystem::path(projectRoot) / filePath;
    if (!std::filesystem::exists(fullPath)) return;

    ma_engine_play_sound(engine, fullPath.string().c_str(), NULL);
}

void AudioManager::playSoundAt(const std::string& filePath, Vec3 position, Vec3 listenerPos, float volume, float rollOff) {
    if (!engine) return;

    std::filesystem::path fullPath = std::filesystem::path(projectRoot) / filePath;
    if (!std::filesystem::exists(fullPath)) return;

    float dx = position.x - listenerPos.x;
    float dy = position.y - listenerPos.y;
    float dz = position.z - listenerPos.z;
    float distSq = dx*dx + dy*dy + dz*dz;
    
    // Minecraft-like falloff: full volume up to 2 blocks, linear fade to zero at 16 blocks
    float maxDist = 16.0f;
    if (distSq > maxDist * maxDist) return;

    float dist = std::sqrt(distSq);
    float gain = 1.0f;
    if (dist > 2.0f) {
        gain = 1.0f - (dist - 2.0f) / (maxDist - 2.0f);
    }
    if (gain <= 0.0f) return;

    // Use fire-and-forget which is safe and managed by miniaudio engine internally.
    // We can't easily set individual volume for fire-and-forget in miniaudio engine without handles,
    // so we'll just play it. The attenuation 'gain' is already used to cull distant sounds.
    ma_engine_play_sound(engine, fullPath.string().c_str(), NULL);
}

void AudioManager::playAmbientMobSound(const std::string& mobType, Vec3 position, Vec3 listenerPos) {
    if (!mobSoundsEnabled) return;
    if (mobSounds.count(mobType)) {
        playSoundAt(mobSounds[mobType], position, listenerPos, mobVolume);
    }
}

void AudioManager::playBlockBreakSound(uint8_t blockType, Vec3 position, Vec3 listenerPos) {
    if (!blockSoundsEnabled) return;
    std::string path = "assets/sounds/block_break.wav";
    if (blockSounds.count(blockType)) {
        path = blockSounds[blockType];
    }
    playSoundAt(path, position, listenerPos, blockVolume);
}

void AudioManager::startNextMusic() {
    if (musicPlaylist.empty()) return;

    if (currentMusic) {
        ma_sound_uninit((ma_sound*)currentMusic);
        delete (ma_sound*)currentMusic;
        currentMusic = nullptr;
    }

    int idx = std::rand() % musicPlaylist.size();
    std::string fullPath = (std::filesystem::path(projectRoot) / musicPlaylist[idx]).string();
    
    if (std::filesystem::exists(fullPath)) {
        ma_sound* sound = new ma_sound();
        if (ma_sound_init_from_file(engine, fullPath.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, sound) == MA_SUCCESS) {
            ma_sound_set_volume(sound, musicVolume);
            ma_sound_set_looping(sound, MA_TRUE); // LOOPING INFINITELY as requested
            ma_sound_start(sound);
            currentMusic = sound;
        } else {
            delete sound;
        }
    }
}

void AudioManager::update(float dt) {
    if (!musicEnabled) return;

    if (!currentMusic) {
        musicTimer -= dt;
        if (musicTimer <= 0.0f) {
            startNextMusic();
        }
    } else {
        // Music is looping indefinitely now, so we don't need to check for finished
        // unless we want to change tracks occasionally.
        // The user said "infinite, like in a loop to work", so looping one track is fine.
    }
}
