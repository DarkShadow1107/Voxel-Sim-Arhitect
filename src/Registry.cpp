#include "Registry.hpp"

void GameRegistry::init() {
    // Register default blocks
    registerBlock({BLOCK_AIR, "Air", {0,0,0}, 0, 0, true, false});
    registerBlock({BLOCK_DIRT, "Dirt", {0.55f, 0.40f, 0.25f}, 0, 0});
    registerBlock({BLOCK_GRASS, "Grass", {0.30f, 0.75f, 0.30f}, 1, 0});
    registerBlock({BLOCK_STONE, "Stone", {0.60f, 0.60f, 0.65f}, 3, 0});
    registerBlock({BLOCK_WATER, "Water", {0.20f, 0.40f, 0.90f}, 4, 0, true, true});
    registerBlock({BLOCK_LAVA, "Lava", {1.00f, 0.30f, 0.00f}, 5, 0, false, true});
    registerBlock({BLOCK_WOOD, "Wood", {0.45f, 0.30f, 0.15f}, 6, 0});
    registerBlock({BLOCK_LEAVES, "Leaves", {0.20f, 0.60f, 0.20f}, 7, 0, true});
    registerBlock({BLOCK_SAND, "Sand", {0.95f, 0.90f, 0.60f}, 8, 0});
    registerBlock({BLOCK_SNOW, "Snow", {1.00f, 1.00f, 1.00f}, 9, 0});
    registerBlock({BLOCK_GLASS, "Glass", {1.00f, 1.00f, 1.00f}, 14, 0, true});

    // Register default mobs
    registerMob({MOB_COW, "Cow", 10.0f, 2.0f, "cow"});
    registerMob({MOB_PIG, "Pig", 10.0f, 2.0f, "pig"});
    registerMob({MOB_SHEEP, "Sheep", 8.0f, 2.0f, "sheep"});
    registerMob({MOB_CHICKEN, "Chicken", 4.0f, 2.2f, "chicken"});
    registerMob({MOB_RABBIT, "Rabbit", 3.0f, 3.5f, "rabbit"});
    registerMob({MOB_DOG, "Dog", 20.0f, 3.0f, "dog"});
    registerMob({MOB_CAT, "Cat", 10.0f, 2.8f, "cat"});
    registerMob({MOB_BIRD, "Bird", 2.0f, 4.0f, "bird"});
    registerMob({MOB_FISH, "Fish", 2.0f, 2.5f, "fish", true});
}
