#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "Math.hpp"
#include "Chunk.hpp"
#include "MobAI.hpp"

struct BlockDefinition {
    uint8_t id;
    std::string name;
    Vec3 color = {1.0f, 1.0f, 1.0f};
    int texX = 0;
    int texY = 0;
    bool isTransparent = false;
    bool isLiquid = false;
    std::string breakSound = "";
    std::string stepSound = "";
};

struct MobDefinition {
    MobType type;
    std::string name;
    float maxHp = 10.0f;
    float speed = 2.0f;
    std::string ambientSound = "";
    bool isAquatic = false;
    bool isHostile = false;
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

    std::unordered_map<uint8_t, BlockDefinition>& getAllBlocks() { return m_blocks; }
    std::unordered_map<MobType, MobDefinition>& getAllMobs() { return m_mobs; }

private:
    GameRegistry() = default;
    std::unordered_map<uint8_t, BlockDefinition> m_blocks;
    std::unordered_map<MobType, MobDefinition> m_mobs;
};
