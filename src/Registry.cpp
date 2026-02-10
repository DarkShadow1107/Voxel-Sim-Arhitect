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

    // Register default blocks
    registerDefBlock(0, "Air", {0,0,0}, 0, 0, true, false);
    registerDefBlock(1, "Dirt", {0.55f, 0.40f, 0.25f}, 0, 0);
    registerDefBlock(2, "Grass", {0.30f, 0.75f, 0.30f}, 1, 0);
    registerDefBlock(3, "Stone", {0.60f, 0.60f, 0.65f}, 3, 0);
    registerDefBlock(4, "Water", {0.20f, 0.40f, 0.90f}, 4, 0, true, true);
    registerDefBlock(5, "Lava", {1.00f, 0.30f, 0.00f}, 5, 0, false, true);
    registerDefBlock(6, "Wood", {0.45f, 0.30f, 0.15f}, 6, 0);
    registerDefBlock(7, "Leaves", {0.20f, 0.60f, 0.20f}, 7, 0, true);
    registerDefBlock(8, "Sand", {0.95f, 0.90f, 0.60f}, 8, 0);
    registerDefBlock(9, "Snow", {1.00f, 1.00f, 1.00f}, 9, 0);
    registerDefBlock(14, "Glass", {1.00f, 1.00f, 1.00f}, 14, 0, true);

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
}
