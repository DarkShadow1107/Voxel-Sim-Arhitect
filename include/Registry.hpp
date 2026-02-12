#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "Math.hpp"
#include "Chunk.hpp"
#include "MobAI.hpp"
#include "AINodeEditor.hpp"

struct BlockFace {
    int texX = 0;
    int texY = 0;
    Vec3 color = {1.0f, 1.0f, 1.0f};
};

struct BlockDrop {
    uint8_t blockId = 0;
    int minCount = 1;
    int maxCount = 1;
    float chance = 1.0f;
};

struct BlockInteraction {
    uint8_t neighborBlockId = 0;
    uint8_t resultBlockId = 0;
    std::string condition = "adjacent"; // "adjacent", "above", "below"
};

struct BlockDefinition {
    uint8_t id;
    std::string name;
    bool usePerFace = false;
    BlockFace faces[6]; // Order: +X, -X, +Y, -Y, +Z, -Z

    // Legacy support (fallback)
    Vec3 color = {1.0f, 1.0f, 1.0f};
    int texX = 0;
    int texY = 0;

    bool isTransparent = false;
    bool isLiquid = false;
    std::string breakSound = "";
    std::string stepSound = "";

    // Breaking mechanics
    float breakTime = 0.5f;
    std::string requiredToolType = "";  // "pickaxe","axe","shovel","hoe","sword","" = any/hand
    int requiredToolTier = 0;           // 0=hand, 1=wood, 2=stone, 3=iron, 4=diamond
    bool dropsItself = true;
    std::vector<BlockDrop> drops;

    // Physics properties
    float blastResistance = 1.0f;
    int lightEmission = 0;              // 0-15
    bool hasGravity = false;
    float friction = 0.6f;
    float slipperiness = 0.0f;
    int opacity = 15;
    bool flammable = false;
    int burnTime = 0;
    bool replaceable = false;
    int redstonePower = 0;

    // Block interactions
    std::vector<BlockInteraction> interactions;

    // State machine
    bool hasStates = false;
    int stateCount = 1;
    std::string stateNames[4] = {"default", "", "", ""};
};

struct MobPart {
    std::string name;
    Vec3 offset;
    Vec3 size;
    Vec3 pivot;
    Vec3 color = {1.0f, 1.0f, 1.0f};
    bool affectedByLegAnim = false;
    bool affectedByHeadAnim = false;
    // Per-face texture support (same order as BlockDefinition: +X,-X,+Y,-Y,+Z,-Z)
    bool usePerFace = false;
    BlockFace faces[6] = {};
    int texX = 0;
    int texY = 3; // Row 3 = mob textures in atlas
};

struct MobDefinition {
    MobType type;
    std::string name;
    float maxHp = 10.0f;
    float speed = 2.0f;
    std::string ambientSound = "";
    bool isAquatic = false;
    bool isHostile = false;
    std::vector<MobPart> parts;

    // AI behavior graph (visual node editor)
    AIGraph aiGraph;
};

struct ToolPiece {
    std::string name = "Piece";
    Vec3 color = {1.0f, 1.0f, 1.0f};
    bool usePerFace = false;
    BlockFace faces[6] = {};
    int texX = 0;
    int texY = 0;
};

struct ToolDefinition {
    int id = 0;
    std::string name;
    std::string toolType;           // "pickaxe","axe","shovel","hoe","sword","bow","shield","fishing_rod"
    int tier = 0;                   // 0=wood, 1=stone, 2=iron, 3=diamond
    float speedMultiplier = 1.0f;
    float damage = 1.0f;
    Vec3 color = {1.0f, 1.0f, 1.0f};
    int durability = 100;
    float attackSpeed = 1.0f;
    float knockback = 0.0f;
    std::string specialEffect = ""; // "","fire_aspect","silk_touch","fortune","efficiency","unbreaking"
    uint8_t craftingRecipe[9] = {0};
    std::vector<ToolPiece> customPieces; // Per-piece appearance (optional, sized to match procedural pieces)
};

struct WeatherPreset {
    std::string name = "Clear";
    // Particles
    int particleCount = 0;
    float particleSpeed = 800.0f;
    float particleSize = 1.5f;
    Vec3 particleColor = {0.3f, 0.5f, 1.0f};
    float particleAlpha = 0.7f;
    bool snowStyle = false;
    // Precipitation type
    int precipitationType = 0; // 0=rain, 1=snow, 2=hail, 3=sleet, 4=ash, 5=sandstorm
    float particleSpread = 1.0f; // horizontal spread multiplier
    // Wind
    float windStrength = 0.0f;
    float windDirection = 0.0f;
    float windGusts = 0.0f;     // gust intensity (0 = no gusts)
    float gustFrequency = 0.5f; // gusts per second
    // Fog
    float fogDensity = 0.0f;
    Vec3 fogColor = {0.5f, 0.5f, 0.6f};
    float fogStartDistance = 10.0f;
    float fogEndDistance = 100.0f;
    float fogHeight = 50.0f;
    // Sky
    Vec3 skyColorDay = {0.4f, 0.6f, 0.9f};
    Vec3 skyColorNight = {0.05f, 0.05f, 0.1f};
    float skyBrightness = 1.0f;
    // Clouds
    float cloudCoverage = 0.0f;
    float cloudHeight = 128.0f;
    float cloudSpeed = 1.0f;
    Vec3 cloudColor = {1.0f, 1.0f, 1.0f};
    float cloudDarkness = 0.0f;
    // Thunder & Lightning
    bool hasThunder = false;
    float thunderFrequency = 0.0005f;
    float thunderVolume = 0.5f;
    float lightningFrequency = 0.001f;
    // Environment
    float temperature = 20.0f;    // Celsius
    float visibility = 1000.0f;   // meters
    float wetness = 0.0f;         // 0-1
    // Sound
    std::string ambientSound = "";
    float ambientVolume = 0.4f;
    std::string extraSound = "";
    float extraSoundVolume = 0.0f;
    // Seasonal
    std::string season = "";
};

struct WeatherTransition {
    int fromIdx = 0;
    int toIdx = 0;
    float duration = 5.0f;
    float triggerWorldTime = -1.0f;
    int easingType = 0; // 0=linear, 1=ease-in, 2=ease-out, 3=ease-in-out
};

class GameRegistry {
public:
    static GameRegistry& getInstance() {
        static GameRegistry instance;
        return instance;
    }

    void init();

    const BlockDefinition& getBlock(uint8_t id) const {
        auto it = m_blocks.find(id);
        if (it != m_blocks.end()) return it->second;
        return m_blocks.at(0); // Return Air
    }

    const MobDefinition& getMob(MobType type) const {
        auto it = m_mobs.find(type);
        if (it != m_mobs.end()) return it->second;
        return m_mobs.at(MOB_COW);
    }

    void registerBlock(const BlockDefinition& def) { m_blocks[def.id] = def; }
    void registerMob(const MobDefinition& def) { m_mobs[def.type] = def; }
    void registerTool(const ToolDefinition& def) { m_tools[def.id] = def; }

    std::unordered_map<uint8_t, BlockDefinition>& getAllBlocks() { return m_blocks; }
    std::unordered_map<MobType, MobDefinition>& getAllMobs() { return m_mobs; }
    std::unordered_map<int, ToolDefinition>& getAllTools() { return m_tools; }

    const ToolDefinition* getTool(int id) const {
        auto it = m_tools.find(id);
        return (it != m_tools.end()) ? &it->second : nullptr;
    }

private:
    GameRegistry() = default;
    std::unordered_map<uint8_t, BlockDefinition> m_blocks;
    std::unordered_map<MobType, MobDefinition> m_mobs;
    std::unordered_map<int, ToolDefinition> m_tools;
};
