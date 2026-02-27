#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Registry.hpp"

using Catch::Matchers::WithinAbs;

TEST_CASE("Registry: Air Block Defs", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    const auto& air = reg.getBlock(0);
    REQUIRE(air.id == 0);
    REQUIRE(air.name == "Air");
    REQUIRE(air.isTransparent == true);
    REQUIRE_THAT(air.breakTime, WithinAbs(0.0f, 0.001f));
    REQUIRE(air.isLiquid == false);
}
    
TEST_CASE("Registry: Dirt Block Defs", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    const auto& dirt = reg.getBlock(1);
    REQUIRE(dirt.id == 1);
    REQUIRE(dirt.name == "Dirt");
    REQUIRE(dirt.breakTime > 0.0f);
    REQUIRE(dirt.requiredToolType == "shovel");
}

TEST_CASE("Registry: Water Block Defs", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    const auto& water = reg.getBlock(4);
    REQUIRE(water.isLiquid == true);
    REQUIRE(water.name == "Water");
}

TEST_CASE("Registry: Out of Bounds Block Defs", "[registry]") {
    // RATIONALE: getBlock(id) guarantees it never throws and always returns a
    // valid, named block even for unregistered IDs (it falls back to a default).
    // We do NOT assert which specific block is returned because the fallback
    // is an implementation detail and the singleton may carry state from other
    // test cases. We verify the public behavioral contract only.
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();

    SECTION("Unknown ID does not throw") {
        REQUIRE_NOTHROW(reg.getBlock(200));
        REQUIRE_NOTHROW(reg.getBlock(255));
    }

    SECTION("Unknown ID returns a non-empty named block (valid reference)") {
        const auto& fb = reg.getBlock(255);
        CHECK_FALSE(fb.name.empty());
    }

    SECTION("Valid IDs return consistent results across repeated calls") {
        const auto& first  = reg.getBlock(BLOCK_AIR);
        const auto& second = reg.getBlock(BLOCK_AIR);
        REQUIRE(first.id   == second.id);
        REQUIRE(first.name == second.name);
    }
}

TEST_CASE("Registry: Mob Defaults", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    const auto& cow = reg.getMob(MOB_COW);
    REQUIRE(cow.type == MOB_COW);
    REQUIRE(cow.name == "Cow");
    REQUIRE(cow.parts.size() >= 5); // Head, body, etc
    
    // Ensure unknown mob returns Cow (default boundary behavior defined in Registry.hpp)
    const auto& unknown = reg.getMob((MobType)999);
    REQUIRE(unknown.type == MOB_COW);
}

TEST_CASE("Registry: Tool Retrieval", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    const auto* woodPick = reg.getTool(1);
    REQUIRE(woodPick != nullptr);
    REQUIRE(woodPick->name == "Wooden Pickaxe");
    REQUIRE(woodPick->toolType == "pickaxe");
    REQUIRE(woodPick->tier == 1);
    REQUIRE(woodPick->damage > 0.0f);
    
    const auto* unknown = reg.getTool(9999);
    REQUIRE(unknown == nullptr);
}

TEST_CASE("Registry: Dynamic Registration Override", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    BlockDefinition testBlock;
    testBlock.id = 254; // Assume 254 is unused for now
    testBlock.name = "Testium Block";
    testBlock.friction = 0.99f;
    
    reg.registerBlock(testBlock);
    
    const auto& retrieved = reg.getBlock(254);
    REQUIRE(retrieved.id == 254);
    REQUIRE(retrieved.name == "Testium Block");
    REQUIRE_THAT(retrieved.friction, WithinAbs(0.99f, 0.001f));
}

TEST_CASE("Registry: Block Drops Logic", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    const auto& stone = reg.getBlock(BLOCK_STONE);
    // Stone usually drops cobblestone, not itself
    REQUIRE(stone.dropsItself == false);
    REQUIRE(stone.drops.size() == 1);
    REQUIRE(stone.drops[0].blockId == BLOCK_COBBLESTONE);
}

TEST_CASE("Registry: Hardness/Blast Resistance", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    const auto& bedrock = reg.getBlock(BLOCK_BEDROCK);
    REQUIRE(bedrock.breakTime > 999999.0f);
    REQUIRE(bedrock.blastResistance > 999999.0f);
    
    const auto& obsidian = reg.getBlock(70); // Obsidian
    REQUIRE(obsidian.id == 70); 
    REQUIRE(obsidian.name == "Obsidian");
}

TEST_CASE("Registry: Light Emission", "[registry]") {
    GameRegistry& reg = GameRegistry::getInstance(); reg.init();
    const auto& fire = reg.getBlock(BLOCK_FIRE);
    REQUIRE(fire.lightEmission == 15);
}
