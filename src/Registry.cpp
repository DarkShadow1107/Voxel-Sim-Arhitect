#include "Registry.hpp"

void GameRegistry::init() {
    auto registerDefBlock = [&](uint8_t id, std::string name, Vec3 col, int tx, int ty, bool trans = false, bool liq = false) {
        BlockDefinition b;
        b.id = id; b.name = name; b.color = col; b.texX = tx; b.texY = ty;
        b.isTransparent = trans; b.isLiquid = liq;
        b.usePerFace = false;
        for(int i=0; i<6; ++i) { b.faces[i] = {tx, ty, col}; }
        registerBlock(b);
    };

    // Register default blocks — vertex colors are lightened since they multiply
    // with the already-colored texture atlas. Closer to {1,1,1} = less tinting.
    registerDefBlock(0, "Air", {0,0,0}, 0, 0, true, false);
    registerDefBlock(1, "Dirt", {0.85f, 0.75f, 0.65f}, 0, 0);
    registerDefBlock(3, "Stone", {0.80f, 0.80f, 0.82f}, 3, 0);
    registerDefBlock(4, "Water", {0.45f, 0.65f, 1.00f}, 4, 0, true, true);
    registerDefBlock(5, "Lava", {1.00f, 0.60f, 0.20f}, 5, 0, false, true);
    registerDefBlock(6, "Wood", {0.75f, 0.60f, 0.45f}, 6, 0);
    registerDefBlock(7, "Leaves", {0.50f, 0.80f, 0.50f}, 7, 0, true);
    registerDefBlock(8, "Sand", {1.00f, 0.98f, 0.80f}, 8, 0);
    registerDefBlock(9, "Snow", {1.00f, 1.00f, 1.00f}, 9, 0);
    registerDefBlock(10, "Bedrock", {0.30f, 0.30f, 0.30f}, 10, 0);
    registerDefBlock(11, "Red Flower", {1.00f, 0.50f, 0.45f}, 11, 0, true);
    registerDefBlock(12, "Blue Flower", {0.50f, 0.60f, 1.00f}, 12, 0, true);
    registerDefBlock(13, "Tall Grass", {0.55f, 0.80f, 0.50f}, 13, 0, true);
    registerDefBlock(14, "Glass", {1.00f, 1.00f, 1.00f}, 14, 0, true);
    registerDefBlock(15, "Coal Ore", {0.70f, 0.70f, 0.70f}, 0, 1);
    registerDefBlock(16, "Iron Ore", {0.82f, 0.78f, 0.75f}, 1, 1);
    registerDefBlock(17, "Gold Ore", {0.95f, 0.90f, 0.60f}, 2, 1);
    registerDefBlock(18, "Diamond Ore", {0.65f, 0.95f, 0.95f}, 3, 1);
    registerDefBlock(19, "Birch Wood", {0.92f, 0.90f, 0.88f}, 4, 1);
    registerDefBlock(20, "Birch Leaves", {0.55f, 0.82f, 0.50f}, 5, 1, true);
    registerDefBlock(21, "Cherry Wood", {0.98f, 0.95f, 0.96f}, 6, 1);
    registerDefBlock(22, "Cherry Leaves", {0.95f, 0.70f, 0.80f}, 7, 1, true);
    registerDefBlock(23, "Cobblestone", {0.70f, 0.70f, 0.70f}, 8, 1);
    registerDefBlock(24, "Mossy Stone", {0.65f, 0.72f, 0.65f}, 9, 1);
    registerDefBlock(25, "Oak Planks", {0.82f, 0.72f, 0.58f}, 10, 1);
    registerDefBlock(26, "Bricks", {0.80f, 0.60f, 0.50f}, 11, 1);
    registerDefBlock(27, "Ice", {0.85f, 0.92f, 1.00f}, 12, 1, true);
    registerDefBlock(28, "Fire", {1.00f, 0.60f, 0.15f}, 5, 0, true);

    // Grass: per-face textures (top=grass, sides=grass side, bottom=dirt)
    {
        BlockDefinition b;
        b.id = BLOCK_GRASS; b.name = "Grass";
        b.color = {0.55f, 0.85f, 0.55f}; b.texX = 1; b.texY = 0;
        b.usePerFace = true;
        b.faces[0] = {2, 0, {0.75f, 0.80f, 0.60f}}; // +X side
        b.faces[1] = {2, 0, {0.75f, 0.80f, 0.60f}}; // -X side
        b.faces[2] = {1, 0, {0.55f, 0.85f, 0.55f}}; // +Y top
        b.faces[3] = {0, 0, {0.85f, 0.75f, 0.65f}}; // -Y bottom (dirt)
        b.faces[4] = {2, 0, {0.75f, 0.80f, 0.60f}}; // +Z side
        b.faces[5] = {2, 0, {0.75f, 0.80f, 0.60f}}; // -Z side
        registerBlock(b);
    }

    // Register default mobs with Minecraft-accurate part models.
    // Dimensions in voxel-world units (roughly 1 pixel = 1/16 block).
    // Minecraft pixel sizes are converted: px / 16.0 = world units.
    //
    // Convention:
    //   offset = bottom-center of the part relative to mob origin (feet at Y=0)
    //   size   = (width, height, depth) in world units
    //   pivot  = rotation pivot relative to part
    //   color  = approximate Minecraft skin color
    //   affectedByLegAnim / affectedByHeadAnim flags

    // ========== COW ==========
    // Minecraft cow: body 12w×10h×18d px, head 8×8×6, legs 4×12×4
    // 1px = 0.0625 world units. Feet at Y=0, body bottom at Y=0.75 (12px).
    {
        MobDefinition m;
        m.type = MOB_COW; m.name = "Cow"; m.maxHp = 10; m.speed = 2.0f;
        // Body
        m.parts.push_back({"Body",    {-0.375f, 0.75f, -0.5625f}, {0.75f, 0.625f, 1.125f}, {0,0,0}, {0.44f, 0.31f, 0.20f}});
        // Head (attached flush to front of body)
        m.parts.push_back({"Head",    {-0.25f, 0.875f, 0.5625f}, {0.5f, 0.5f, 0.375f}, {0, -0.25f, -0.1875f}, {0.44f, 0.31f, 0.20f}, false, true});
        // Muzzle (lighter snout)
        m.parts.push_back({"Muzzle",  {-0.1875f, 0.875f, 0.9375f}, {0.375f, 0.25f, 0.0625f}, {0,0,0}, {0.75f, 0.70f, 0.62f}});
        // Horns
        m.parts.push_back({"Horn L",  {-0.3125f, 1.375f, 0.6875f}, {0.0625f, 0.1875f, 0.0625f}, {0,0,0}, {0.85f, 0.85f, 0.80f}});
        m.parts.push_back({"Horn R",  { 0.25f,   1.375f, 0.6875f}, {0.0625f, 0.1875f, 0.0625f}, {0,0,0}, {0.85f, 0.85f, 0.80f}});
        // Legs: 4×12×4 px, flush with body bottom (Y=0.75)
        m.parts.push_back({"Leg FL",  {-0.3125f, 0.0f,  0.3125f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.44f, 0.31f, 0.20f}, true});
        m.parts.push_back({"Leg FR",  { 0.0625f, 0.0f,  0.3125f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.44f, 0.31f, 0.20f}, true});
        m.parts.push_back({"Leg BL",  {-0.3125f, 0.0f, -0.5625f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.44f, 0.31f, 0.20f}, true});
        m.parts.push_back({"Leg BR",  { 0.0625f, 0.0f, -0.5625f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.44f, 0.31f, 0.20f}, true});
        // Udder
        m.parts.push_back({"Udder",   {-0.125f, 0.625f, -0.1875f}, {0.25f, 0.125f, 0.25f}, {0,0,0}, {0.92f, 0.78f, 0.72f}});
        registerMob(m);
    }

    // ========== PIG ==========
    // Minecraft pig: body 10w×8h×16d px, head 8×8×8, legs 4×6×4
    // Body bottom at Y=0.375 (6px).
    {
        MobDefinition m;
        m.type = MOB_PIG; m.name = "Pig"; m.maxHp = 10; m.speed = 2.0f;
        // Body
        m.parts.push_back({"Body",    {-0.3125f, 0.375f, -0.5f}, {0.625f, 0.5f, 1.0f}, {0,0,0}, {0.90f, 0.63f, 0.55f}});
        // Head (flush with body front)
        m.parts.push_back({"Head",    {-0.25f, 0.375f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0, -0.25f, -0.25f}, {0.90f, 0.63f, 0.55f}, false, true});
        // Snout
        m.parts.push_back({"Snout",   {-0.125f, 0.4375f, 1.0f}, {0.25f, 0.1875f, 0.0625f}, {0,0,0}, {0.82f, 0.52f, 0.45f}});
        // Legs: 4×6×4 px, flush with body bottom (Y=0.375)
        m.parts.push_back({"Leg FL",  {-0.25f,   0.0f,  0.25f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.90f, 0.63f, 0.55f}, true});
        m.parts.push_back({"Leg FR",  { 0.0f,    0.0f,  0.25f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.90f, 0.63f, 0.55f}, true});
        m.parts.push_back({"Leg BL",  {-0.25f,   0.0f, -0.5f},  {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.90f, 0.63f, 0.55f}, true});
        m.parts.push_back({"Leg BR",  { 0.0f,    0.0f, -0.5f},  {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.90f, 0.63f, 0.55f}, true});
        registerMob(m);
    }

    // ========== SHEEP ==========
    // Minecraft sheep: wool body 12w×10h×16d px, head 6×6×8, legs 4×12×4
    // Body bottom at Y=0.75 (12px). Head is dark, body is woolly white.
    {
        MobDefinition m;
        m.type = MOB_SHEEP; m.name = "Sheep"; m.maxHp = 8; m.speed = 2.0f;
        // Woolly body
        m.parts.push_back({"Body",    {-0.375f, 0.75f, -0.5f}, {0.75f, 0.625f, 1.0f}, {0,0,0}, {0.93f, 0.93f, 0.90f}});
        // Head (dark fur, not wool)
        m.parts.push_back({"Head",    {-0.1875f, 0.875f, 0.5f}, {0.375f, 0.375f, 0.5f}, {0, -0.1875f, -0.25f}, {0.62f, 0.58f, 0.54f}, false, true});
        // Legs: 4×12×4 px, flush with body bottom (Y=0.75)
        m.parts.push_back({"Leg FL",  {-0.3125f, 0.0f,  0.25f},  {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.62f, 0.58f, 0.54f}, true});
        m.parts.push_back({"Leg FR",  { 0.0625f, 0.0f,  0.25f},  {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.62f, 0.58f, 0.54f}, true});
        m.parts.push_back({"Leg BL",  {-0.3125f, 0.0f, -0.5f},   {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.62f, 0.58f, 0.54f}, true});
        m.parts.push_back({"Leg BR",  { 0.0625f, 0.0f, -0.5f},   {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.62f, 0.58f, 0.54f}, true});
        registerMob(m);
    }

    // ========== CHICKEN ==========
    // Minecraft chicken: body 6w×6h×8d px, head 4×6×3, legs 3×5×3, wings 1×4×6
    // Body bottom at Y=0.3125 (5px).
    {
        MobDefinition m;
        m.type = MOB_CHICKEN; m.name = "Chicken"; m.maxHp = 4; m.speed = 2.2f;
        // Body
        m.parts.push_back({"Body",    {-0.1875f, 0.3125f, -0.25f}, {0.375f, 0.375f, 0.5f}, {0,0,0}, {0.95f, 0.95f, 0.95f}});
        // Head
        m.parts.push_back({"Head",    {-0.125f, 0.5625f, 0.25f}, {0.25f, 0.375f, 0.1875f}, {0, -0.1875f, -0.09375f}, {0.95f, 0.95f, 0.95f}, false, true});
        // Beak
        m.parts.push_back({"Beak",    {-0.0625f, 0.625f, 0.4375f}, {0.125f, 0.0625f, 0.125f}, {0,0,0}, {0.95f, 0.65f, 0.15f}});
        // Wattle (red thing under beak)
        m.parts.push_back({"Wattle",  {-0.0625f, 0.5625f, 0.4375f}, {0.125f, 0.125f, 0.0625f}, {0,0,0}, {0.90f, 0.20f, 0.15f}});
        // Wings
        m.parts.push_back({"Wing L",  {-0.25f,   0.3125f, -0.1875f}, {0.0625f, 0.25f, 0.375f}, {0, 0.25f, 0}, {0.92f, 0.92f, 0.90f}});
        m.parts.push_back({"Wing R",  { 0.1875f, 0.3125f, -0.1875f}, {0.0625f, 0.25f, 0.375f}, {0, 0.25f, 0}, {0.92f, 0.92f, 0.90f}});
        // Legs: flush with body bottom (Y=0.3125)
        m.parts.push_back({"Leg L",   {-0.125f, 0.0f, 0.0f}, {0.125f, 0.3125f, 0.125f}, {0.0625f, 0.3125f, 0.0625f}, {0.95f, 0.70f, 0.20f}, true});
        m.parts.push_back({"Leg R",   { 0.0f,   0.0f, 0.0f}, {0.125f, 0.3125f, 0.125f}, {0.0625f, 0.3125f, 0.0625f}, {0.95f, 0.70f, 0.20f}, true});
        registerMob(m);
    }

    // ========== RABBIT ==========
    // Minecraft rabbit: body 5w×4h×7d px, head 5×4×5, ears 1×5×2
    // Body bottom at Y=0.25 (4px).
    {
        MobDefinition m;
        m.type = MOB_RABBIT; m.name = "Rabbit"; m.maxHp = 3; m.speed = 3.5f;
        // Body
        m.parts.push_back({"Body",    {-0.15625f, 0.25f, -0.21875f}, {0.3125f, 0.25f, 0.4375f}, {0,0,0}, {0.60f, 0.45f, 0.30f}});
        // Head (flush at body top-front)
        m.parts.push_back({"Head",    {-0.15625f, 0.375f, 0.21875f}, {0.3125f, 0.25f, 0.3125f}, {0, -0.125f, -0.15625f}, {0.60f, 0.45f, 0.30f}, false, true});
        // Ears
        m.parts.push_back({"Ear L",   {-0.125f,   0.625f, 0.28125f}, {0.0625f, 0.3125f, 0.125f}, {0,0,0}, {0.55f, 0.40f, 0.28f}});
        m.parts.push_back({"Ear R",   { 0.0625f,  0.625f, 0.28125f}, {0.0625f, 0.3125f, 0.125f}, {0,0,0}, {0.55f, 0.40f, 0.28f}});
        // Tail
        m.parts.push_back({"Tail",    {-0.09375f, 0.3125f, -0.34375f}, {0.1875f, 0.125f, 0.125f}, {0,0,0}, {0.95f, 0.95f, 0.92f}});
        // Front legs: 2×4×2 px, flush with body bottom (Y=0.25)
        m.parts.push_back({"Leg FL",  {-0.125f, 0.0f,  0.125f}, {0.125f, 0.25f, 0.125f}, {0.0625f, 0.25f, 0.0625f}, {0.60f, 0.45f, 0.30f}, true});
        m.parts.push_back({"Leg FR",  { 0.0f,   0.0f,  0.125f}, {0.125f, 0.25f, 0.125f}, {0.0625f, 0.25f, 0.0625f}, {0.60f, 0.45f, 0.30f}, true});
        // Hind legs: 3×4×3 px (larger, same height)
        m.parts.push_back({"Leg BL",  {-0.15625f, 0.0f, -0.21875f}, {0.1875f, 0.25f, 0.1875f}, {0.09375f, 0.25f, 0.09375f}, {0.60f, 0.45f, 0.30f}, true});
        m.parts.push_back({"Leg BR",  { 0.0f,     0.0f, -0.21875f}, {0.1875f, 0.25f, 0.1875f}, {0.09375f, 0.25f, 0.09375f}, {0.60f, 0.45f, 0.30f}, true});
        registerMob(m);
    }

    // ========== DOG (Wolf) ==========
    // Minecraft wolf: body 6w×8h×8d px, head 6×6×4, legs 2×8×2, tail 2×8×2
    // Body bottom at Y=0.5 (8px).
    {
        MobDefinition m;
        m.type = MOB_DOG; m.name = "Dog"; m.maxHp = 20; m.speed = 3.0f;
        // Body
        m.parts.push_back({"Body",    {-0.1875f, 0.5f, -0.25f}, {0.375f, 0.5f, 0.5f}, {0,0,0}, {0.82f, 0.82f, 0.80f}});
        // Head (flush with body front)
        m.parts.push_back({"Head",    {-0.1875f, 0.625f, 0.25f}, {0.375f, 0.375f, 0.25f}, {0, -0.1875f, -0.125f}, {0.82f, 0.82f, 0.80f}, false, true});
        // Snout
        m.parts.push_back({"Snout",   {-0.125f, 0.625f, 0.5f}, {0.25f, 0.1875f, 0.1875f}, {0,0,0}, {0.78f, 0.78f, 0.75f}});
        // Nose
        m.parts.push_back({"Nose",    {-0.0625f, 0.75f, 0.6875f}, {0.125f, 0.0625f, 0.03125f}, {0,0,0}, {0.15f, 0.12f, 0.12f}});
        // Ears
        m.parts.push_back({"Ear L",   {-0.1875f, 1.0f, 0.3125f}, {0.125f, 0.1875f, 0.125f}, {0,0,0}, {0.72f, 0.72f, 0.70f}});
        m.parts.push_back({"Ear R",   { 0.0625f, 1.0f, 0.3125f}, {0.125f, 0.1875f, 0.125f}, {0,0,0}, {0.72f, 0.72f, 0.70f}});
        // Collar (tamed)
        m.parts.push_back({"Collar",  {-0.21875f, 0.875f, 0.1875f}, {0.4375f, 0.0625f, 0.125f}, {0,0,0}, {0.85f, 0.18f, 0.12f}});
        // Tail
        m.parts.push_back({"Tail",    {-0.0625f, 0.75f, -0.375f}, {0.125f, 0.5f, 0.125f}, {0.0625f, 0, 0.0625f}, {0.82f, 0.82f, 0.80f}});
        // Legs: 2×8×2 px, flush with body bottom (Y=0.5)
        m.parts.push_back({"Leg FL",  {-0.1875f, 0.0f,  0.125f},  {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.82f, 0.82f, 0.80f}, true});
        m.parts.push_back({"Leg FR",  { 0.0625f, 0.0f,  0.125f},  {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.82f, 0.82f, 0.80f}, true});
        m.parts.push_back({"Leg BL",  {-0.1875f, 0.0f, -0.25f},   {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.82f, 0.82f, 0.80f}, true});
        m.parts.push_back({"Leg BR",  { 0.0625f, 0.0f, -0.25f},   {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.82f, 0.82f, 0.80f}, true});
        registerMob(m);
    }

    // ========== CAT ==========
    // Minecraft cat: body 6w×4h×10d px, head 5×4×5, legs 2×6×2, tail 1×1×8
    // Body bottom at Y=0.375 (6px).
    {
        MobDefinition m;
        m.type = MOB_CAT; m.name = "Cat"; m.maxHp = 10; m.speed = 2.8f;
        // Body
        m.parts.push_back({"Body",    {-0.1875f, 0.375f, -0.3125f}, {0.375f, 0.25f, 0.625f}, {0,0,0}, {0.85f, 0.55f, 0.22f}});
        // Head (flush with body front)
        m.parts.push_back({"Head",    {-0.15625f, 0.375f, 0.3125f}, {0.3125f, 0.25f, 0.3125f}, {0, -0.125f, -0.15625f}, {0.85f, 0.55f, 0.22f}, false, true});
        // Nose
        m.parts.push_back({"Nose",    {-0.03125f, 0.4375f, 0.625f}, {0.0625f, 0.0625f, 0.03125f}, {0,0,0}, {0.92f, 0.48f, 0.48f}});
        // Ears (pointed)
        m.parts.push_back({"Ear L",   {-0.125f,   0.625f, 0.4375f}, {0.0625f, 0.125f, 0.125f}, {0,0,0}, {0.80f, 0.50f, 0.18f}});
        m.parts.push_back({"Ear R",   { 0.0625f,  0.625f, 0.4375f}, {0.0625f, 0.125f, 0.125f}, {0,0,0}, {0.80f, 0.50f, 0.18f}});
        // Tail
        m.parts.push_back({"Tail",    {-0.03125f, 0.5f, -0.8125f}, {0.0625f, 0.0625f, 0.5f}, {0,0,0.5f}, {0.80f, 0.50f, 0.18f}});
        // Legs: 2×6×2 px, flush with body bottom (Y=0.375)
        m.parts.push_back({"Leg FL",  {-0.15625f, 0.0f,  0.1875f}, {0.125f, 0.375f, 0.125f}, {0.0625f, 0.375f, 0.0625f}, {0.85f, 0.55f, 0.22f}, true});
        m.parts.push_back({"Leg FR",  { 0.03125f, 0.0f,  0.1875f}, {0.125f, 0.375f, 0.125f}, {0.0625f, 0.375f, 0.0625f}, {0.85f, 0.55f, 0.22f}, true});
        m.parts.push_back({"Leg BL",  {-0.15625f, 0.0f, -0.25f},   {0.125f, 0.375f, 0.125f}, {0.0625f, 0.375f, 0.0625f}, {0.85f, 0.55f, 0.22f}, true});
        m.parts.push_back({"Leg BR",  { 0.03125f, 0.0f, -0.25f},   {0.125f, 0.375f, 0.125f}, {0.0625f, 0.375f, 0.0625f}, {0.85f, 0.55f, 0.22f}, true});
        registerMob(m);
    }

    // ========== BIRD (Parrot) ==========
    // Minecraft parrot: body 4w×5h×3d px, head 4×3×3, wings 1×4×5, legs 1×2×1
    // Body bottom at Y=0.25 (4px).
    {
        MobDefinition m;
        m.type = MOB_BIRD; m.name = "Bird"; m.maxHp = 2; m.speed = 4.0f;
        // Body
        m.parts.push_back({"Body",    {-0.125f, 0.25f, -0.09375f}, {0.25f, 0.3125f, 0.1875f}, {0,0,0}, {0.20f, 0.75f, 0.20f}});
        // Head (flush with body top)
        m.parts.push_back({"Head",    {-0.125f, 0.5625f, -0.09375f}, {0.25f, 0.1875f, 0.1875f}, {0, -0.09375f, -0.09375f}, {0.20f, 0.75f, 0.20f}, false, true});
        // Beak
        m.parts.push_back({"Beak",    {-0.0625f, 0.5625f, 0.09375f}, {0.125f, 0.0625f, 0.125f}, {0,0,0}, {0.95f, 0.70f, 0.10f}});
        // Wings
        m.parts.push_back({"Wing L",  {-0.1875f, 0.3125f, -0.15625f}, {0.0625f, 0.25f, 0.3125f}, {0, 0.25f, 0}, {0.85f, 0.15f, 0.15f}});
        m.parts.push_back({"Wing R",  { 0.125f,  0.3125f, -0.15625f}, {0.0625f, 0.25f, 0.3125f}, {0, 0.25f, 0}, {0.15f, 0.15f, 0.85f}});
        // Tail feathers
        m.parts.push_back({"Tail",    {-0.125f, 0.3125f, -0.28125f}, {0.25f, 0.125f, 0.1875f}, {0,0,0}, {0.85f, 0.15f, 0.15f}});
        // Legs: flush with body bottom (Y=0.25)
        m.parts.push_back({"Leg L",   {-0.09375f, 0.0f, 0.0f}, {0.0625f, 0.25f, 0.0625f}, {0, 0.25f, 0}, {0.40f, 0.40f, 0.40f}, true});
        m.parts.push_back({"Leg R",   { 0.03125f, 0.0f, 0.0f}, {0.0625f, 0.25f, 0.0625f}, {0, 0.25f, 0}, {0.40f, 0.40f, 0.40f}, true});
        registerMob(m);
    }

    // ========== FISH (Cod) ==========
    // Minecraft cod: body 6w×4h×2d px, head 4×4×2, fins
    {
        MobDefinition m;
        m.type = MOB_FISH; m.name = "Fish"; m.maxHp = 2; m.speed = 2.5f;
        m.isAquatic = true;
        // Body
        m.parts.push_back({"Body",    {-0.1875f, 0.0625f, -0.0625f}, {0.375f, 0.25f, 0.125f}, {0,0,0}, {0.65f, 0.55f, 0.35f}});
        // Head
        m.parts.push_back({"Head",    {-0.125f, 0.0625f, 0.0625f}, {0.25f, 0.25f, 0.125f}, {0,0,0}, {0.70f, 0.60f, 0.38f}, false, true});
        // Tail fin
        m.parts.push_back({"Tail Fin",{-0.125f, 0.0625f, -0.1875f}, {0.25f, 0.25f, 0.0625f}, {0.125f, 0.125f, 0.0625f}, {0.65f, 0.55f, 0.35f}});
        // Top fin
        m.parts.push_back({"Top Fin", {-0.1875f, 0.3125f, -0.03125f}, {0.375f, 0.125f, 0.0625f}, {0,0,0}, {0.60f, 0.50f, 0.30f}});
        registerMob(m);
    }

    // ========== SALMON ==========
    // Minecraft salmon: longer body 8w×4h×2d px
    {
        MobDefinition m;
        m.type = MOB_SALMON; m.name = "Salmon"; m.maxHp = 3; m.speed = 3.0f;
        m.isAquatic = true;
        // Body
        m.parts.push_back({"Body",    {-0.25f, 0.0625f, -0.0625f}, {0.5f, 0.25f, 0.125f}, {0,0,0}, {0.70f, 0.25f, 0.25f}});
        // Head
        m.parts.push_back({"Head",    {-0.125f, 0.0625f, 0.0625f}, {0.25f, 0.25f, 0.125f}, {0,0,0}, {0.75f, 0.30f, 0.28f}, false, true});
        // Tail
        m.parts.push_back({"Tail Fin",{-0.125f, 0.0625f, -0.25f}, {0.25f, 0.25f, 0.0625f}, {0.125f, 0.125f, 0.0625f}, {0.65f, 0.20f, 0.20f}});
        // Back (darker segment)
        m.parts.push_back({"Back",    {-0.125f, 0.0625f, -0.1875f}, {0.25f, 0.25f, 0.125f}, {0,0,0}, {0.50f, 0.18f, 0.18f}});
        // Top fin
        m.parts.push_back({"Top Fin", {-0.125f, 0.3125f, -0.03125f}, {0.25f, 0.0625f, 0.0625f}, {0,0,0}, {0.60f, 0.22f, 0.22f}});
        registerMob(m);
    }

    // ========== OCTOPUS (Squid) ==========
    // Minecraft squid: body 8w×8h×8d px, 8 tentacles 2×18×2
    {
        MobDefinition m;
        m.type = MOB_OCTOPUS; m.name = "Octopus"; m.maxHp = 10; m.speed = 1.5f;
        m.isAquatic = true;
        // Body (mantle)
        m.parts.push_back({"Body",    {-0.25f, 0.5f, -0.25f}, {0.5f, 0.5f, 0.5f}, {0,0,0}, {0.25f, 0.35f, 0.55f}});
        // Eyes
        m.parts.push_back({"Eye L",   {-0.26f, 0.7f, 0.1f}, {0.06f, 0.1f, 0.1f}, {0,0,0}, {0.95f, 0.95f, 0.95f}});
        m.parts.push_back({"Eye R",   { 0.2f,  0.7f, 0.1f}, {0.06f, 0.1f, 0.1f}, {0,0,0}, {0.95f, 0.95f, 0.95f}});
        m.parts.push_back({"Pupil L", {-0.265f, 0.72f, 0.14f}, {0.04f, 0.06f, 0.06f}, {0,0,0}, {0.05f, 0.05f, 0.08f}});
        m.parts.push_back({"Pupil R", { 0.225f, 0.72f, 0.14f}, {0.04f, 0.06f, 0.06f}, {0,0,0}, {0.05f, 0.05f, 0.08f}});
        // 8 tentacles: 2×8×2 px, flush with body bottom (Y=0.5)
        m.parts.push_back({"Tent. 1", { 0.125f, 0.0f,  0.125f},  {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.22f, 0.32f, 0.52f}, true});
        m.parts.push_back({"Tent. 2", {-0.25f,  0.0f,  0.125f},  {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.22f, 0.32f, 0.52f}, true});
        m.parts.push_back({"Tent. 3", { 0.125f, 0.0f, -0.25f},   {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.22f, 0.32f, 0.52f}, true});
        m.parts.push_back({"Tent. 4", {-0.25f,  0.0f, -0.25f},   {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.22f, 0.32f, 0.52f}, true});
        m.parts.push_back({"Tent. 5", { 0.0f,   0.0f,  0.1875f}, {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.22f, 0.32f, 0.52f}, true});
        m.parts.push_back({"Tent. 6", { 0.0f,   0.0f, -0.3125f}, {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.22f, 0.32f, 0.52f}, true});
        m.parts.push_back({"Tent. 7", { 0.1875f,0.0f,  0.0f},    {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.22f, 0.32f, 0.52f}, true});
        m.parts.push_back({"Tent. 8", {-0.3125f,0.0f,  0.0f},    {0.125f, 0.5f, 0.125f}, {0.0625f, 0.5f, 0.0625f}, {0.22f, 0.32f, 0.52f}, true});
        registerMob(m);
    }

    // --- Block breaking properties ---
    auto setBreakProps = [&](uint8_t id, float breakTime, const std::string& toolType, int toolTier,
                             float blastRes = 1.0f, int light = 0, bool gravity = false) {
        auto it = m_blocks.find(id);
        if (it == m_blocks.end()) return;
        it->second.breakTime = breakTime;
        it->second.requiredToolType = toolType;
        it->second.requiredToolTier = toolTier;
        it->second.blastResistance = blastRes;
        it->second.lightEmission = light;
        it->second.hasGravity = gravity;
    };

    // Air/Water/Lava are unbreakable or special
    setBreakProps(BLOCK_AIR, 0.0f, "", 0);
    setBreakProps(BLOCK_DIRT, 0.5f, "shovel", 0, 0.5f);
    setBreakProps(BLOCK_GRASS, 0.6f, "shovel", 0, 0.6f);
    setBreakProps(BLOCK_STONE, 1.5f, "pickaxe", 1, 6.0f);
    setBreakProps(BLOCK_WATER, 0.0f, "", 0, 100.0f);
    setBreakProps(BLOCK_LAVA, 0.0f, "", 0, 100.0f, 15);
    setBreakProps(BLOCK_WOOD, 2.0f, "axe", 0, 2.0f);
    setBreakProps(BLOCK_LEAVES, 0.2f, "", 0, 0.2f);
    setBreakProps(BLOCK_SAND, 0.5f, "shovel", 0, 0.5f, 0, true);
    setBreakProps(BLOCK_SNOW, 0.2f, "shovel", 0, 0.1f);
    setBreakProps(BLOCK_BEDROCK, 1e9f, "", 0, 1e9f);
    setBreakProps(BLOCK_FLOWER_RED, 0.0f, "", 0, 0.0f);
    setBreakProps(BLOCK_FLOWER_BLUE, 0.0f, "", 0, 0.0f);
    setBreakProps(BLOCK_TALL_GRASS, 0.0f, "", 0, 0.0f);
    setBreakProps(BLOCK_GLASS, 0.3f, "", 0, 0.3f);
    setBreakProps(BLOCK_COAL_ORE, 3.0f, "pickaxe", 1, 3.0f);
    setBreakProps(BLOCK_IRON_ORE, 3.0f, "pickaxe", 2, 3.0f);
    setBreakProps(BLOCK_GOLD_ORE, 3.0f, "pickaxe", 3, 3.0f);
    setBreakProps(BLOCK_DIAMOND_ORE, 3.0f, "pickaxe", 3, 3.0f);
    setBreakProps(BLOCK_BIRCH_WOOD, 2.0f, "axe", 0, 2.0f);
    setBreakProps(BLOCK_BIRCH_LEAVES, 0.2f, "", 0, 0.2f);
    setBreakProps(BLOCK_CHERRY_WOOD, 2.0f, "axe", 0, 2.0f);
    setBreakProps(BLOCK_CHERRY_LEAVES, 0.2f, "", 0, 0.2f);
    setBreakProps(BLOCK_COBBLESTONE, 2.0f, "pickaxe", 1, 6.0f);
    setBreakProps(BLOCK_MOSSY_STONE, 2.0f, "pickaxe", 1, 6.0f);
    setBreakProps(BLOCK_OAK_PLANKS, 2.0f, "axe", 0, 3.0f);
    setBreakProps(BLOCK_BRICKS, 2.0f, "pickaxe", 1, 6.0f);
    setBreakProps(BLOCK_ICE, 0.5f, "pickaxe", 0, 0.5f);
    setBreakProps(BLOCK_FIRE, 0.0f, "", 0, 0.0f, 15);

    // Special drops: coal ore drops coal (itself for now), diamond ore drops diamond
    // Stone drops cobblestone
    {
        auto& stone = m_blocks[BLOCK_STONE];
        stone.dropsItself = false;
        stone.drops.push_back({BLOCK_COBBLESTONE, 1, 1, 1.0f});
    }

    // Glass drops nothing
    {
        auto& glass = m_blocks[BLOCK_GLASS];
        glass.dropsItself = false;
        // No drops entry = drops nothing
    }

    // --- Default tool definitions ---
    // Helper: {id, name, type, tier, speed, damage, color, durability, atkSpeed, knockback}
    auto regTool = [&](int id, const char* name, const char* type, int tier,
                       float speed, float dmg, Vec3 col, int dur, float atkSpd = 1.0f, float kb = 0.0f) {
        ToolDefinition t;
        t.id = id; t.name = name; t.toolType = type; t.tier = tier;
        t.speedMultiplier = speed; t.damage = dmg; t.color = col;
        t.durability = dur; t.attackSpeed = atkSpd; t.knockback = kb;
        registerTool(t);
    };

    // Pickaxes
    regTool(1,  "Wooden Pickaxe",   "pickaxe", 1, 2.0f, 2.0f, {0.6f, 0.4f, 0.2f},  59, 1.2f);
    regTool(2,  "Stone Pickaxe",    "pickaxe", 2, 4.0f, 3.0f, {0.5f, 0.5f, 0.5f},  131, 1.2f);
    regTool(3,  "Iron Pickaxe",     "pickaxe", 3, 6.0f, 4.0f, {0.85f, 0.85f, 0.85f}, 250, 1.2f);
    regTool(4,  "Diamond Pickaxe",  "pickaxe", 4, 8.0f, 5.0f, {0.3f, 0.9f, 0.9f}, 1561, 1.2f);
    // Axes
    regTool(5,  "Wooden Axe",       "axe", 1, 2.0f, 3.0f, {0.6f, 0.4f, 0.2f},  59, 0.8f);
    regTool(6,  "Stone Axe",        "axe", 2, 4.0f, 4.0f, {0.5f, 0.5f, 0.5f},  131, 0.8f);
    regTool(7,  "Iron Axe",         "axe", 3, 6.0f, 5.0f, {0.85f, 0.85f, 0.85f}, 250, 0.9f);
    regTool(8,  "Diamond Axe",      "axe", 4, 8.0f, 6.0f, {0.3f, 0.9f, 0.9f}, 1561, 1.0f);
    // Shovels
    regTool(9,  "Wooden Shovel",    "shovel", 1, 2.0f, 1.0f, {0.6f, 0.4f, 0.2f},  59, 1.0f);
    regTool(10, "Stone Shovel",     "shovel", 2, 4.0f, 2.0f, {0.5f, 0.5f, 0.5f},  131, 1.0f);
    regTool(11, "Iron Shovel",      "shovel", 3, 6.0f, 3.0f, {0.85f, 0.85f, 0.85f}, 250, 1.0f);
    regTool(12, "Diamond Shovel",   "shovel", 4, 8.0f, 4.0f, {0.3f, 0.9f, 0.9f}, 1561, 1.0f);
    // Swords
    regTool(13, "Wooden Sword",     "sword", 1, 1.0f, 4.0f, {0.6f, 0.4f, 0.2f},  59, 1.6f, 0.4f);
    regTool(14, "Stone Sword",      "sword", 2, 1.0f, 5.0f, {0.5f, 0.5f, 0.5f},  131, 1.6f, 0.4f);
    regTool(15, "Iron Sword",       "sword", 3, 1.0f, 6.0f, {0.85f, 0.85f, 0.85f}, 250, 1.6f, 0.4f);
    regTool(16, "Diamond Sword",    "sword", 4, 1.0f, 7.0f, {0.3f, 0.9f, 0.9f}, 1561, 1.6f, 0.5f);
    // Hoes
    regTool(17, "Wooden Hoe",       "hoe", 1, 1.0f, 1.0f, {0.6f, 0.4f, 0.2f},  59, 1.0f);
    regTool(18, "Stone Hoe",        "hoe", 2, 1.0f, 1.0f, {0.5f, 0.5f, 0.5f},  131, 2.0f);
    regTool(19, "Iron Hoe",         "hoe", 3, 1.0f, 1.0f, {0.85f, 0.85f, 0.85f}, 250, 3.0f);
    regTool(20, "Diamond Hoe",      "hoe", 4, 1.0f, 1.0f, {0.3f, 0.9f, 0.9f}, 1561, 4.0f);
    // Bow & Shield
    regTool(21, "Bow",              "bow", 1, 1.0f, 6.0f, {0.6f, 0.4f, 0.2f}, 384, 1.0f, 0.0f);
    regTool(22, "Shield",           "shield", 1, 1.0f, 1.0f, {0.6f, 0.4f, 0.2f}, 336, 0.0f, 2.0f);
    regTool(23, "Fishing Rod",      "fishing_rod", 1, 1.0f, 0.0f, {0.6f, 0.4f, 0.2f}, 64, 0.0f, 0.0f);

    // --- Block physics defaults ---
    auto setPhysics = [&](uint8_t id, float fric, float slip, int opa, bool flame, int burn, bool repl, int redstone) {
        auto it = m_blocks.find(id);
        if (it == m_blocks.end()) return;
        auto& b = it->second;
        b.friction = fric; b.slipperiness = slip; b.opacity = opa;
        b.flammable = flame; b.burnTime = burn; b.replaceable = repl; b.redstonePower = redstone;
    };
    setPhysics(BLOCK_AIR,     0.0f, 0.0f, 0,  false, 0, true, 0);
    setPhysics(BLOCK_WATER,   0.0f, 0.0f, 1,  false, 0, true, 0);
    setPhysics(BLOCK_LAVA,    0.0f, 0.0f, 0,  false, 0, true, 0);
    setPhysics(BLOCK_ICE,     0.1f, 0.98f, 3, false, 0, false, 0);
    setPhysics(BLOCK_WOOD,    0.6f, 0.0f, 15, true, 300, false, 0);
    setPhysics(BLOCK_LEAVES,  0.6f, 0.0f, 1,  true, 60, false, 0);
    setPhysics(BLOCK_FIRE,    0.0f, 0.0f, 0,  false, 0, true, 0);
    setPhysics(BLOCK_OAK_PLANKS, 0.6f, 0.0f, 15, true, 300, false, 0);
    setPhysics(BLOCK_GLASS,   0.6f, 0.0f, 0,  false, 0, false, 0);
    setPhysics(BLOCK_SAND,    0.6f, 0.0f, 15, false, 0, false, 0);
    setPhysics(BLOCK_TALL_GRASS, 0.0f, 0.0f, 0, true, 60, true, 0);
}
