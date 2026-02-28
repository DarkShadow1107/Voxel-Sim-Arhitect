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
    registerDefBlock(28, "Fire", {1.00f, 0.75f, 0.20f}, 8, 4, true);
    registerDefBlock(29, "Snow Layer", {1.00f, 1.00f, 1.00f}, 9, 0, true);
    registerDefBlock(30, "Gravel", {0.90f, 0.85f, 0.70f}, 14, 1);
    registerDefBlock(31, "Clay", {0.65f, 0.65f, 0.70f}, 15, 1);
    registerDefBlock(32, "Sponge", {0.75f, 0.75f, 0.85f}, 0, 2);
    registerDefBlock(33, "Glass Pane", {1.00f, 1.00f, 1.00f}, 1, 2, true);
    registerDefBlock(34, "Lapis Lazuli Ore", {0.20f, 0.40f, 0.80f}, 2, 2);
    registerDefBlock(35, "Lapis Lazuli Block", {0.15f, 0.35f, 0.75f}, 3, 2);
    registerDefBlock(36, "Dispenser", {0.80f, 0.80f, 0.40f}, 4, 2);
    registerDefBlock(37, "Sandstone", {0.90f, 0.85f, 0.70f}, 5, 2);
    registerDefBlock(38, "Note Block", {0.40f, 0.30f, 0.20f}, 6, 2);
    registerDefBlock(39, "Bed", {0.80f, 0.20f, 0.20f}, 7, 2);
    registerDefBlock(40, "Powered Rail", {0.50f, 0.40f, 0.30f}, 8, 2);
    registerDefBlock(41, "Detector Rail", {0.60f, 0.50f, 0.40f}, 9, 2);
    registerDefBlock(42, "Sticky Piston", {0.50f, 0.50f, 0.50f}, 10, 2);
    registerDefBlock(43, "Cobweb", {0.90f, 0.90f, 0.90f}, 11, 2, true);
    registerDefBlock(44, "Piston", {0.50f, 0.50f, 0.50f}, 12, 2);
    registerDefBlock(45, "Piston Head", {0.60f, 0.60f, 0.60f}, 13, 2);
    registerDefBlock(46, "White Wool", {0.90f, 0.90f, 0.90f}, 14, 2);
    registerDefBlock(47, "Orange Wool", {0.90f, 0.50f, 0.10f}, 15, 2);
    registerDefBlock(48, "Magenta Wool", {0.80f, 0.30f, 0.80f}, 0, 3);
    registerDefBlock(49, "Light Blue Wool", {0.40f, 0.60f, 0.90f}, 1, 3);
    registerDefBlock(50, "Yellow Wool", {0.90f, 0.90f, 0.20f}, 2, 3);
    registerDefBlock(51, "Lime Wool", {0.50f, 0.80f, 0.20f}, 3, 3);
    registerDefBlock(52, "Pink Wool", {0.90f, 0.50f, 0.60f}, 4, 3);
    registerDefBlock(53, "Gray Wool", {0.30f, 0.30f, 0.30f}, 5, 3);
    registerDefBlock(54, "Light Gray Wool", {0.60f, 0.60f, 0.60f}, 6, 3);
    registerDefBlock(55, "Cyan Wool", {0.30f, 0.60f, 0.60f}, 7, 3);
    registerDefBlock(56, "Purple Wool", {0.50f, 0.20f, 0.70f}, 8, 3);
    registerDefBlock(57, "Blue Wool", {0.20f, 0.30f, 0.70f}, 9, 3);
    registerDefBlock(58, "Brown Wool", {0.40f, 0.20f, 0.10f}, 10, 3);
    registerDefBlock(59, "Green Wool", {0.30f, 0.50f, 0.20f}, 11, 3);
    registerDefBlock(60, "Red Wool", {0.70f, 0.20f, 0.20f}, 12, 3);
    registerDefBlock(61, "Black Wool", {0.10f, 0.10f, 0.10f}, 13, 3);
    registerDefBlock(62, "Gold Block", {0.95f, 0.90f, 0.40f}, 14, 3);
    registerDefBlock(63, "Iron Block", {0.85f, 0.85f, 0.85f}, 15, 3);
    registerDefBlock(64, "Double Stone Slab", {0.70f, 0.70f, 0.70f}, 0, 4);
    registerDefBlock(65, "Stone Slab", {0.70f, 0.70f, 0.70f}, 1, 4);
    registerDefBlock(66, "Bricks", {0.80f, 0.40f, 0.30f}, 2, 4);
    registerDefBlock(67, "TNT", {0.80f, 0.20f, 0.20f}, 3, 4);
    registerDefBlock(68, "Bookshelf", {0.60f, 0.40f, 0.20f}, 4, 4);
    registerDefBlock(69, "Mossy Cobblestone", {0.50f, 0.60f, 0.50f}, 5, 4);
    registerDefBlock(70, "Obsidian", {0.10f, 0.10f, 0.15f}, 6, 4);
    registerDefBlock(71, "Torch", {0.90f, 0.80f, 0.40f}, 7, 4, true);
    registerDefBlock(72, "Fire", {0.90f, 0.50f, 0.10f}, 8, 4, true);
    registerDefBlock(73, "Monster Spawner", {0.20f, 0.20f, 0.30f}, 9, 4);
    registerDefBlock(74, "Oak Stairs", {0.60f, 0.40f, 0.20f}, 10, 4);
    registerDefBlock(75, "Chest", {0.60f, 0.40f, 0.20f}, 11, 4);
    registerDefBlock(76, "Redstone Wire", {0.80f, 0.10f, 0.10f}, 12, 4, true);
    registerDefBlock(77, "Diamond Ore", {0.40f, 0.80f, 0.80f}, 13, 4);
    registerDefBlock(78, "Diamond Block", {0.30f, 0.90f, 0.90f}, 14, 4);
    registerDefBlock(79, "Crafting Table", {0.60f, 0.40f, 0.20f}, 15, 4);
    registerDefBlock(80, "Wheat Crops", {0.80f, 0.80f, 0.20f}, 0, 5, true);
    registerDefBlock(81, "Farmland", {0.40f, 0.30f, 0.20f}, 1, 5);
    registerDefBlock(82, "Furnace", {0.40f, 0.40f, 0.40f}, 2, 5);
    registerDefBlock(83, "Burning Furnace", {0.50f, 0.40f, 0.40f}, 3, 5);
    registerDefBlock(84, "Sign", {0.60f, 0.40f, 0.20f}, 4, 5, true);
    registerDefBlock(85, "Oak Door", {0.60f, 0.40f, 0.20f}, 5, 5, true);
    registerDefBlock(86, "Ladder", {0.60f, 0.40f, 0.20f}, 6, 5, true);
    registerDefBlock(87, "Rail", {0.50f, 0.50f, 0.50f}, 7, 5, true);
    registerDefBlock(88, "Cobblestone Stairs", {0.50f, 0.50f, 0.50f}, 8, 5);
    registerDefBlock(89, "Wall Sign", {0.60f, 0.40f, 0.20f}, 9, 5, true);
    registerDefBlock(90, "Lever", {0.50f, 0.50f, 0.50f}, 10, 5, true);
    registerDefBlock(91, "Stone Pressure Plate", {0.50f, 0.50f, 0.50f}, 11, 5, true);
    registerDefBlock(92, "Iron Door", {0.80f, 0.80f, 0.80f}, 12, 5, true);
    registerDefBlock(93, "Oak Pressure Plate", {0.60f, 0.40f, 0.20f}, 13, 5, true);
    registerDefBlock(94, "Redstone Ore", {0.80f, 0.20f, 0.20f}, 14, 5);
    registerDefBlock(95, "Glowing Redstone Ore", {0.90f, 0.30f, 0.30f}, 15, 5);
    registerDefBlock(96, "Redstone Torch", {0.80f, 0.20f, 0.20f}, 0, 6, true);
    registerDefBlock(97, "Redstone Torch", {0.90f, 0.30f, 0.30f}, 1, 6, true);
    registerDefBlock(98, "Stone Button", {0.50f, 0.50f, 0.50f}, 2, 6, true);
    registerDefBlock(99, "Snow", {0.90f, 0.90f, 0.90f}, 3, 6, true);
    registerDefBlock(100, "Ice", {0.60f, 0.80f, 0.90f}, 4, 6, true);
    registerDefBlock(101, "Snow Block", {0.90f, 0.90f, 0.90f}, 5, 6);
    registerDefBlock(102, "Cactus", {0.20f, 0.60f, 0.20f}, 6, 6);
    registerDefBlock(103, "Clay", {0.60f, 0.60f, 0.70f}, 7, 6);
    registerDefBlock(104, "Sugar Cane", {0.30f, 0.70f, 0.30f}, 8, 6);
    registerDefBlock(105, "Jukebox", {0.50f, 0.30f, 0.20f}, 9, 6);
    registerDefBlock(106, "Oak Fence", {0.60f, 0.40f, 0.20f}, 10, 6, true);
    registerDefBlock(107, "Pumpkin", {0.80f, 0.40f, 0.10f}, 11, 6);
    registerDefBlock(108, "Netherrack", {0.20f, 0.10f, 0.20f}, 12, 6);
    registerDefBlock(109, "Soul Sand", {0.40f, 0.30f, 0.20f}, 13, 6);
    registerDefBlock(110, "Glowstone", {0.80f, 0.60f, 0.20f}, 14, 6);
    registerDefBlock(111, "Nether Portal", {0.40f, 0.20f, 0.80f}, 15, 6, true);
    registerDefBlock(112, "Jack o'Lantern", {0.90f, 0.50f, 0.10f}, 0, 7);
    registerDefBlock(113, "Cake", {0.90f, 0.80f, 0.70f}, 1, 7);
    registerDefBlock(114, "Repeater", {0.60f, 0.60f, 0.60f}, 2, 7, true);
    registerDefBlock(115, "Repeater", {0.70f, 0.70f, 0.70f}, 3, 7, true);
    registerDefBlock(116, "White Stained Glass", {0.90f, 0.90f, 0.90f}, 4, 7, true);
    registerDefBlock(117, "Orange Stained Glass", {0.90f, 0.50f, 0.10f}, 5, 7, true);
    registerDefBlock(118, "Magenta Stained Glass", {0.80f, 0.30f, 0.80f}, 6, 7, true);
    registerDefBlock(119, "Light Blue Stained Glass", {0.40f, 0.60f, 0.90f}, 7, 7, true);
    registerDefBlock(120, "Yellow Stained Glass", {0.90f, 0.90f, 0.20f}, 8, 7, true);
    registerDefBlock(121, "Lime Stained Glass", {0.50f, 0.80f, 0.20f}, 9, 7, true);
    registerDefBlock(122, "Pink Stained Glass", {0.90f, 0.50f, 0.60f}, 10, 7, true);
    registerDefBlock(123, "Gray Stained Glass", {0.30f, 0.30f, 0.30f}, 11, 7, true);
    registerDefBlock(124, "Light Gray Stained Glass", {0.60f, 0.60f, 0.60f}, 12, 7, true);
    registerDefBlock(125, "Cyan Stained Glass", {0.30f, 0.60f, 0.60f}, 13, 7, true);
    registerDefBlock(126, "Purple Stained Glass", {0.50f, 0.20f, 0.70f}, 14, 7, true);
    registerDefBlock(127, "Blue Stained Glass", {0.20f, 0.30f, 0.70f}, 15, 7, true);
    registerDefBlock(128, "Brown Stained Glass", {0.40f, 0.20f, 0.10f}, 0, 8, true);
    registerDefBlock(129, "Green Stained Glass", {0.30f, 0.50f, 0.20f}, 1, 8, true);
    registerDefBlock(130, "Red Stained Glass", {0.70f, 0.20f, 0.20f}, 2, 8, true);
    registerDefBlock(131, "Black Stained Glass", {0.10f, 0.10f, 0.10f}, 3, 8, true);
    registerDefBlock(132, "Oak Trapdoor", {0.60f, 0.40f, 0.20f}, 4, 8, true);
    registerDefBlock(133, "Stone Bricks", {0.60f, 0.60f, 0.60f}, 5, 8);
    registerDefBlock(134, "Brown Mushroom", {0.60f, 0.40f, 0.20f}, 6, 8, true);
    registerDefBlock(135, "Red Mushroom", {0.80f, 0.20f, 0.20f}, 7, 8, true);
    registerDefBlock(136, "Iron Bars", {0.80f, 0.80f, 0.80f}, 8, 8, true);
    registerDefBlock(137, "Glass Pane", {0.90f, 0.90f, 0.90f}, 9, 8, true);
    registerDefBlock(138, "Melon", {0.60f, 0.80f, 0.20f}, 10, 8);
    registerDefBlock(139, "Pumpkin Stem", {0.40f, 0.60f, 0.20f}, 11, 8, true);
    registerDefBlock(140, "Melon Stem", {0.40f, 0.60f, 0.20f}, 12, 8, true);
    registerDefBlock(141, "Vines", {0.30f, 0.60f, 0.30f}, 13, 8, true);
    registerDefBlock(142, "Oak Fence Gate", {0.60f, 0.40f, 0.20f}, 14, 8, true);
    registerDefBlock(143, "Brick Stairs", {0.80f, 0.40f, 0.30f}, 15, 8);
    registerDefBlock(144, "Stone Brick Stairs", {0.60f, 0.60f, 0.60f}, 0, 9);
    registerDefBlock(145, "Mycelium", {0.40f, 0.50f, 0.30f}, 1, 9);
    registerDefBlock(146, "Lily Pad", {0.30f, 0.60f, 0.30f}, 2, 9, true);
    registerDefBlock(147, "Nether Bricks", {0.30f, 0.20f, 0.30f}, 3, 9);
    registerDefBlock(148, "Nether Brick Fence", {0.30f, 0.20f, 0.30f}, 4, 9, true);
    registerDefBlock(149, "Nether Brick Stairs", {0.30f, 0.20f, 0.30f}, 5, 9);
    registerDefBlock(150, "Nether Wart", {0.60f, 0.20f, 0.20f}, 6, 9, true);
    registerDefBlock(151, "Enchanting Table", {0.20f, 0.10f, 0.20f}, 7, 9);
    registerDefBlock(152, "Brewing Stand", {0.40f, 0.40f, 0.40f}, 8, 9);
    registerDefBlock(153, "Cauldron", {0.30f, 0.30f, 0.30f}, 9, 9);
    registerDefBlock(154, "End Portal", {0.40f, 0.20f, 0.80f}, 10, 9, true);
    registerDefBlock(155, "End Portal Frame", {0.40f, 0.50f, 0.40f}, 11, 9);
    registerDefBlock(156, "End Stone", {0.80f, 0.80f, 0.60f}, 12, 9);
    registerDefBlock(157, "Dragon Egg", {0.10f, 0.10f, 0.10f}, 13, 9);
    registerDefBlock(158, "Redstone Lamp", {0.40f, 0.20f, 0.10f}, 14, 9);
    registerDefBlock(159, "Redstone Lamp", {0.90f, 0.70f, 0.20f}, 15, 9);
    registerDefBlock(160, "Oak Wood Slab", {0.60f, 0.40f, 0.20f}, 0, 10);
    registerDefBlock(161, "Spruce Wood Slab", {0.50f, 0.30f, 0.10f}, 1, 10);
    registerDefBlock(162, "Birch Wood Slab", {0.80f, 0.70f, 0.50f}, 2, 10);
    registerDefBlock(163, "Jungle Wood Slab", {0.40f, 0.20f, 0.10f}, 3, 10);
    registerDefBlock(164, "Acacia Wood Slab", {0.70f, 0.40f, 0.20f}, 4, 10);
    registerDefBlock(165, "Dark Oak Wood Slab", {0.30f, 0.20f, 0.10f}, 5, 10);
    registerDefBlock(166, "Sandstone Stairs", {0.90f, 0.80f, 0.60f}, 6, 10);
    registerDefBlock(167, "Emerald Ore", {0.20f, 0.80f, 0.60f}, 7, 10);
    registerDefBlock(168, "Ender Chest", {0.20f, 0.30f, 0.40f}, 8, 10);
    registerDefBlock(169, "Tripwire Hook", {0.50f, 0.50f, 0.50f}, 9, 10, true);
    registerDefBlock(170, "Tripwire", {0.80f, 0.80f, 0.80f}, 10, 10, true);
    registerDefBlock(171, "Emerald Block", {0.30f, 0.90f, 0.50f}, 11, 10);
    registerDefBlock(172, "Spruce Stairs", {0.50f, 0.30f, 0.10f}, 12, 10);
    registerDefBlock(173, "Birch Stairs", {0.80f, 0.70f, 0.50f}, 13, 10);
    registerDefBlock(174, "Jungle Stairs", {0.40f, 0.20f, 0.10f}, 14, 10);
    registerDefBlock(175, "Command Block", {0.60f, 0.40f, 0.20f}, 15, 10);
    registerDefBlock(176, "Beacon", {0.60f, 0.80f, 0.90f}, 0, 11, true);
    registerDefBlock(177, "Cobblestone Wall", {0.50f, 0.50f, 0.50f}, 1, 11, true);
    registerDefBlock(178, "Mossy Cobblestone Wall", {0.40f, 0.50f, 0.40f}, 2, 11, true);
    registerDefBlock(179, "Flower Pot", {0.60f, 0.30f, 0.20f}, 3, 11, true);
    registerDefBlock(180, "Carrots", {0.80f, 0.50f, 0.10f}, 4, 11, true);
    registerDefBlock(181, "Potatoes", {0.80f, 0.60f, 0.20f}, 5, 11, true);
    registerDefBlock(182, "Wooden Button", {0.60f, 0.40f, 0.20f}, 6, 11, true);
    registerDefBlock(183, "Skeleton Skull", {0.80f, 0.80f, 0.80f}, 7, 11, true);
    registerDefBlock(184, "Wither Skeleton Skull", {0.20f, 0.20f, 0.20f}, 8, 11, true);
    registerDefBlock(185, "Zombie Head", {0.40f, 0.60f, 0.40f}, 9, 11, true);
    registerDefBlock(186, "Player Head", {0.80f, 0.60f, 0.40f}, 10, 11, true);
    registerDefBlock(187, "Creeper Head", {0.20f, 0.80f, 0.20f}, 11, 11, true);
    registerDefBlock(188, "Dragon Head", {0.10f, 0.10f, 0.10f}, 12, 11, true);
    registerDefBlock(189, "Anvil", {0.20f, 0.20f, 0.20f}, 13, 11);
    registerDefBlock(190, "Trapped Chest", {0.60f, 0.40f, 0.20f}, 14, 11);
    registerDefBlock(191, "Light Weighted Pressure Plate", {0.90f, 0.80f, 0.40f}, 15, 11, true);
    registerDefBlock(192, "Heavy Weighted Pressure Plate", {0.80f, 0.80f, 0.80f}, 0, 12, true);
    registerDefBlock(193, "Comparator", {0.60f, 0.60f, 0.60f}, 1, 12, true);
    registerDefBlock(194, "Comparator", {0.70f, 0.70f, 0.70f}, 2, 12, true);
    registerDefBlock(195, "Daylight Detector", {0.60f, 0.50f, 0.40f}, 3, 12, true);
    registerDefBlock(196, "Redstone Block", {0.80f, 0.20f, 0.20f}, 4, 12);
    registerDefBlock(197, "Nether Quartz Ore", {0.60f, 0.20f, 0.20f}, 5, 12);
    registerDefBlock(198, "Hopper", {0.40f, 0.40f, 0.40f}, 6, 12);
    registerDefBlock(199, "Quartz Block", {0.90f, 0.90f, 0.90f}, 7, 12);
    registerDefBlock(200, "Chiseled Quartz Block", {0.90f, 0.90f, 0.90f}, 8, 12);
    registerDefBlock(201, "Quartz Pillar", {0.90f, 0.90f, 0.90f}, 9, 12);
    registerDefBlock(202, "Quartz Stairs", {0.90f, 0.90f, 0.90f}, 10, 12);
    registerDefBlock(203, "Activator Rail", {0.60f, 0.40f, 0.30f}, 11, 12, true);
    registerDefBlock(204, "Dropper", {0.50f, 0.50f, 0.50f}, 12, 12);
    registerDefBlock(205, "White Terracotta", {0.90f, 0.90f, 0.90f}, 13, 12);
    registerDefBlock(206, "Orange Terracotta", {0.90f, 0.50f, 0.10f}, 14, 12);
    registerDefBlock(207, "Magenta Terracotta", {0.80f, 0.30f, 0.80f}, 15, 12);
    registerDefBlock(208, "Light Blue Terracotta", {0.40f, 0.60f, 0.90f}, 0, 13);
    registerDefBlock(209, "Yellow Terracotta", {0.90f, 0.90f, 0.20f}, 1, 13);
    registerDefBlock(210, "Lime Terracotta", {0.50f, 0.80f, 0.20f}, 2, 13);
    registerDefBlock(211, "Pink Terracotta", {0.90f, 0.50f, 0.60f}, 3, 13);
    registerDefBlock(212, "Gray Terracotta", {0.30f, 0.30f, 0.30f}, 4, 13);
    registerDefBlock(213, "Light Gray Terracotta", {0.60f, 0.60f, 0.60f}, 5, 13);
    registerDefBlock(214, "Cyan Terracotta", {0.30f, 0.60f, 0.60f}, 6, 13);
    registerDefBlock(215, "Purple Terracotta", {0.50f, 0.20f, 0.70f}, 7, 13);
    registerDefBlock(216, "Blue Terracotta", {0.20f, 0.30f, 0.70f}, 8, 13);
    registerDefBlock(217, "Brown Terracotta", {0.40f, 0.20f, 0.10f}, 9, 13);
    registerDefBlock(218, "Green Terracotta", {0.30f, 0.50f, 0.20f}, 10, 13);
    registerDefBlock(219, "Red Terracotta", {0.70f, 0.20f, 0.20f}, 11, 13);
    registerDefBlock(220, "Black Terracotta", {0.10f, 0.10f, 0.10f}, 12, 13);
    registerDefBlock(221, "White Stained Glass Pane", {0.90f, 0.90f, 0.90f}, 13, 13, true);
    registerDefBlock(222, "Orange Stained Glass Pane", {0.90f, 0.50f, 0.10f}, 14, 13, true);
    registerDefBlock(223, "Magenta Stained Glass Pane", {0.80f, 0.30f, 0.80f}, 15, 13, true);
    registerDefBlock(224, "Light Blue Stained Glass Pane", {0.40f, 0.60f, 0.90f}, 0, 14, true);
    registerDefBlock(225, "Yellow Stained Glass Pane", {0.90f, 0.90f, 0.20f}, 1, 14, true);
    registerDefBlock(226, "Lime Stained Glass Pane", {0.50f, 0.80f, 0.20f}, 2, 14, true);
    registerDefBlock(227, "Pink Stained Glass Pane", {0.90f, 0.50f, 0.60f}, 3, 14, true);
    registerDefBlock(228, "Gray Stained Glass Pane", {0.30f, 0.30f, 0.30f}, 4, 14, true);
    registerDefBlock(229, "Light Gray Stained Glass Pane", {0.60f, 0.60f, 0.60f}, 5, 14, true);
    registerDefBlock(230, "Cyan Stained Glass Pane", {0.30f, 0.60f, 0.60f}, 6, 14, true);
    registerDefBlock(231, "Purple Stained Glass Pane", {0.50f, 0.20f, 0.70f}, 7, 14, true);
    registerDefBlock(232, "Blue Stained Glass Pane", {0.20f, 0.30f, 0.70f}, 8, 14, true);
    registerDefBlock(233, "Brown Stained Glass Pane", {0.40f, 0.20f, 0.10f}, 9, 14, true);
    registerDefBlock(234, "Green Stained Glass Pane", {0.30f, 0.50f, 0.20f}, 10, 14, true);
    registerDefBlock(235, "Red Stained Glass Pane", {0.70f, 0.20f, 0.20f}, 11, 14, true);
    registerDefBlock(236, "Black Stained Glass Pane", {0.10f, 0.10f, 0.10f}, 12, 14, true);
    registerDefBlock(237, "Acacia Leaves", {0.40f, 0.70f, 0.30f}, 13, 14, true);
    registerDefBlock(238, "Dark Oak Leaves", {0.20f, 0.50f, 0.20f}, 14, 14, true);
    registerDefBlock(239, "Slime Block", {0.50f, 0.80f, 0.50f}, 15, 14);
    registerDefBlock(240, "Barrier", {0.00f, 0.00f, 0.00f}, 0, 15, true);
    registerDefBlock(241, "Iron Trapdoor", {0.80f, 0.80f, 0.80f}, 1, 15, true);
    registerDefBlock(242, "Prismarine", {0.40f, 0.70f, 0.60f}, 2, 15);
    registerDefBlock(243, "Prismarine Bricks", {0.30f, 0.60f, 0.50f}, 3, 15);
    registerDefBlock(244, "Dark Prismarine", {0.20f, 0.40f, 0.30f}, 4, 15);
    registerDefBlock(245, "Prismarine Stairs", {0.40f, 0.70f, 0.60f}, 5, 15);
    registerDefBlock(246, "Prismarine Brick Stairs", {0.30f, 0.60f, 0.50f}, 6, 15);
    registerDefBlock(247, "Dark Prismarine Stairs", {0.20f, 0.40f, 0.30f}, 7, 15);
    registerDefBlock(248, "Sea Lantern", {0.80f, 0.90f, 0.80f}, 8, 15);
    registerDefBlock(249, "Hay Block", {0.80f, 0.70f, 0.20f}, 9, 15);
    registerDefBlock(250, "White Carpet", {0.90f, 0.90f, 0.90f}, 10, 15, true);
    registerDefBlock(251, "Orange Carpet", {0.90f, 0.50f, 0.10f}, 11, 15, true);
    registerDefBlock(252, "Magenta Carpet", {0.80f, 0.30f, 0.80f}, 12, 15, true);
    registerDefBlock(253, "Light Blue Carpet", {0.40f, 0.60f, 0.90f}, 13, 15, true);
    registerDefBlock(254, "Yellow Carpet", {0.90f, 0.90f, 0.20f}, 14, 15, true);
    registerDefBlock(255, "Lime Carpet", {0.50f, 0.80f, 0.20f}, 15, 15, true);

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
        m.parts.push_back({"Body",    {-0.3125f, 0.375f, -0.5f}, {0.625f, 0.5f, 1.0f}, {0,0,0}, {0.95f, 0.65f, 0.70f}});
        m.parts.push_back({"Head",    {-0.25f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0, -0.125f, -0.25f}, {0.95f, 0.65f, 0.70f}, false, true});
        m.parts.push_back({"Snout",   {-0.125f, 0.5625f, 1.0f}, {0.25f, 0.1875f, 0.0625f}, {0,0,0}, {0.85f, 0.55f, 0.60f}});
        m.parts.push_back({"Leg FL",  {-0.3125f, 0.0f, 0.25f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.95f, 0.65f, 0.70f}, true});
        m.parts.push_back({"Leg FR",  { 0.0625f, 0.0f, 0.25f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.95f, 0.65f, 0.70f}, true});
        m.parts.push_back({"Leg BL",  {-0.3125f, 0.0f, -0.5f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.95f, 0.65f, 0.70f}, true});
        m.parts.push_back({"Leg BR",  { 0.0625f, 0.0f, -0.5f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.95f, 0.65f, 0.70f}, true});
        registerMob(m);
    }

    // ========== SHEEP ==========
    {
        MobDefinition m;
        m.type = MOB_SHEEP; m.name = "Sheep"; m.maxHp = 8; m.speed = 2.0f;
        m.parts.push_back({"Body",    {-0.3125f, 0.5f, -0.5f}, {0.625f, 0.5f, 1.0f}, {0,0,0}, {0.9f, 0.9f, 0.9f}});
        m.parts.push_back({"Head",    {-0.25f, 0.625f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0, -0.125f, -0.25f}, {0.8f, 0.7f, 0.6f}, false, true});
        m.parts.push_back({"Leg FL",  {-0.3125f, 0.0f, 0.25f}, {0.25f, 0.5f, 0.25f}, {0.125f, 0.5f, 0.125f}, {0.8f, 0.7f, 0.6f}, true});
        m.parts.push_back({"Leg FR",  { 0.0625f, 0.0f, 0.25f}, {0.25f, 0.5f, 0.25f}, {0.125f, 0.5f, 0.125f}, {0.8f, 0.7f, 0.6f}, true});
        m.parts.push_back({"Leg BL",  {-0.3125f, 0.0f, -0.5f}, {0.25f, 0.5f, 0.25f}, {0.125f, 0.5f, 0.125f}, {0.8f, 0.7f, 0.6f}, true});
        m.parts.push_back({"Leg BR",  { 0.0625f, 0.0f, -0.5f}, {0.25f, 0.5f, 0.25f}, {0.125f, 0.5f, 0.125f}, {0.8f, 0.7f, 0.6f}, true});
        registerMob(m);
    }

    // ========== CHICKEN ==========
    {
        MobDefinition m;
        m.type = MOB_CHICKEN; m.name = "Chicken"; m.maxHp = 4; m.speed = 2.5f;
        m.parts.push_back({"Body",    {-0.1875f, 0.25f, -0.25f}, {0.375f, 0.375f, 0.5f}, {0,0,0}, {0.95f, 0.95f, 0.95f}});
        m.parts.push_back({"Head",    {-0.125f, 0.5625f, 0.125f}, {0.25f, 0.375f, 0.1875f}, {0, -0.1875f, -0.09375f}, {0.95f, 0.95f, 0.95f}, false, true});
        m.parts.push_back({"Beak",    {-0.0625f, 0.6875f, 0.3125f}, {0.125f, 0.125f, 0.125f}, {0,0,0}, {0.9f, 0.8f, 0.2f}});
        m.parts.push_back({"Wattle",  {-0.0625f, 0.5625f, 0.3125f}, {0.125f, 0.125f, 0.125f}, {0,0,0}, {0.8f, 0.2f, 0.2f}});
        m.parts.push_back({"Wing L",  {-0.25f, 0.3125f, -0.1875f}, {0.0625f, 0.25f, 0.375f}, {0, 0.125f, 0.1875f}, {0.95f, 0.95f, 0.95f}});
        m.parts.push_back({"Wing R",  { 0.1875f, 0.3125f, -0.1875f}, {0.0625f, 0.25f, 0.375f}, {0, 0.125f, 0.1875f}, {0.95f, 0.95f, 0.95f}});
        m.parts.push_back({"Leg L",   {-0.125f, 0.0f, -0.0625f}, {0.0625f, 0.3125f, 0.0625f}, {0.03125f, 0.3125f, 0.03125f}, {0.9f, 0.8f, 0.2f}, true});
        m.parts.push_back({"Leg R",   { 0.0625f, 0.0f, -0.0625f}, {0.0625f, 0.3125f, 0.0625f}, {0.03125f, 0.3125f, 0.03125f}, {0.9f, 0.8f, 0.2f}, true});
        registerMob(m);
    }

    // ========== ZOMBIE ==========
    {
        MobDefinition m;
        m.type = MOB_ZOMBIE; m.name = "Zombie"; m.maxHp = 20; m.speed = 1.5f;
        m.parts.push_back({"Body",    {-0.25f, 0.75f, -0.125f}, {0.5f, 0.75f, 0.25f}, {0,0,0}, {0.2f, 0.4f, 0.6f}});
        m.parts.push_back({"Head",    {-0.25f, 1.5f, -0.25f}, {0.5f, 0.5f, 0.5f}, {0, -0.25f, 0}, {0.3f, 0.5f, 0.3f}, false, true});
        m.parts.push_back({"Arm L",   {-0.5f, 0.75f, -0.125f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.625f, 0.125f}, {0.2f, 0.4f, 0.6f}, true});
        m.parts.push_back({"Arm R",   { 0.25f, 0.75f, -0.125f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.625f, 0.125f}, {0.2f, 0.4f, 0.6f}, true});
        m.parts.push_back({"Leg L",   {-0.25f, 0.0f, -0.125f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.3f, 0.3f, 0.6f}, true});
        m.parts.push_back({"Leg R",   { 0.0f, 0.0f, -0.125f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.3f, 0.3f, 0.6f}, true});
        registerMob(m);
    }

    // ========== SKELETON ==========
    {
        MobDefinition m;
        m.type = MOB_SKELETON; m.name = "Skeleton"; m.maxHp = 20; m.speed = 1.5f;
        m.parts.push_back({"Body",    {-0.25f, 0.75f, -0.125f}, {0.5f, 0.75f, 0.25f}, {0,0,0}, {0.8f, 0.8f, 0.8f}});
        m.parts.push_back({"Head",    {-0.25f, 1.5f, -0.25f}, {0.5f, 0.5f, 0.5f}, {0, -0.25f, 0}, {0.8f, 0.8f, 0.8f}, false, true});
        m.parts.push_back({"Arm L",   {-0.375f, 0.75f, -0.0625f}, {0.125f, 0.75f, 0.125f}, {0.0625f, 0.625f, 0.0625f}, {0.8f, 0.8f, 0.8f}, true});
        m.parts.push_back({"Arm R",   { 0.25f, 0.75f, -0.0625f}, {0.125f, 0.75f, 0.125f}, {0.0625f, 0.625f, 0.0625f}, {0.8f, 0.8f, 0.8f}, true});
        m.parts.push_back({"Leg L",   {-0.1875f, 0.0f, -0.0625f}, {0.125f, 0.75f, 0.125f}, {0.0625f, 0.75f, 0.0625f}, {0.8f, 0.8f, 0.8f}, true});
        m.parts.push_back({"Leg R",   { 0.0625f, 0.0f, -0.0625f}, {0.125f, 0.75f, 0.125f}, {0.0625f, 0.75f, 0.0625f}, {0.8f, 0.8f, 0.8f}, true});
        registerMob(m);
    }

    // ========== CREEPER ==========
    {
        MobDefinition m;
        m.type = MOB_CREEPER; m.name = "Creeper"; m.maxHp = 20; m.speed = 1.5f;
        m.parts.push_back({"Body",    {-0.25f, 0.375f, -0.125f}, {0.5f, 0.75f, 0.25f}, {0,0,0}, {0.2f, 0.8f, 0.2f}});
        m.parts.push_back({"Head",    {-0.25f, 1.125f, -0.25f}, {0.5f, 0.5f, 0.5f}, {0, -0.25f, 0}, {0.2f, 0.8f, 0.2f}, false, true});
        m.parts.push_back({"Leg FL",  {-0.25f, 0.0f, 0.125f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.2f, 0.8f, 0.2f}, true});
        m.parts.push_back({"Leg FR",  { 0.0f, 0.0f, 0.125f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.2f, 0.8f, 0.2f}, true});
        m.parts.push_back({"Leg BL",  {-0.25f, 0.0f, -0.375f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.2f, 0.8f, 0.2f}, true});
        m.parts.push_back({"Leg BR",  { 0.0f, 0.0f, -0.375f}, {0.25f, 0.375f, 0.25f}, {0.125f, 0.375f, 0.125f}, {0.2f, 0.8f, 0.2f}, true});
        registerMob(m);
    }

    // ========== VILLAGER ==========
    {
        MobDefinition m;
        m.type = MOB_VILLAGER; m.name = "Villager"; m.maxHp = 20; m.speed = 1.5f;
        m.parts.push_back({"Body",    {-0.25f, 0.75f, -0.1875f}, {0.5f, 0.75f, 0.375f}, {0,0,0}, {0.5f, 0.3f, 0.2f}});
        m.parts.push_back({"Head",    {-0.25f, 1.5f, -0.25f}, {0.5f, 0.625f, 0.5f}, {0, -0.25f, 0}, {0.8f, 0.6f, 0.5f}, false, true});
        m.parts.push_back({"Nose",    {-0.0625f, 1.625f, 0.25f}, {0.125f, 0.25f, 0.125f}, {0,0,0}, {0.8f, 0.6f, 0.5f}});
        m.parts.push_back({"Arms",    {-0.25f, 0.875f, -0.3125f}, {0.5f, 0.25f, 0.125f}, {0,0,0}, {0.5f, 0.3f, 0.2f}});
        m.parts.push_back({"Leg L",   {-0.25f, 0.0f, -0.125f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.3f, 0.2f, 0.1f}, true});
        m.parts.push_back({"Leg R",   { 0.0f, 0.0f, -0.125f}, {0.25f, 0.75f, 0.25f}, {0.125f, 0.75f, 0.125f}, {0.3f, 0.2f, 0.1f}, true});
        registerMob(m);
    }
    // ========== PIG ==========
    // Minecraft pig: body 10w×8h×16d px, head 8×8×8, snout 4×3×1, legs 4×6×4
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
    // New biome blocks (IDs reuse existing registry entries)
    setBreakProps(BLOCK_GRAVEL,         0.6f, "shovel", 0, 0.6f, 0, true);   // gravity-affected
    setBreakProps(BLOCK_ASH,            0.5f, "shovel", 0, 0.5f);
    setBreakProps(BLOCK_BASALT,         2.5f, "pickaxe", 1, 8.0f);
    setBreakProps(BLOCK_PUMICE,         0.9f, "pickaxe", 0, 1.5f);
    setBreakProps(BLOCK_SCORCHED_GRASS, 0.6f, "shovel",  0, 0.5f);

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

    // Pickaxes (Minecraft exact: 2/3/4/5 damage, 1.2/s attack)
    regTool(1,  "Wooden Pickaxe",   "pickaxe", 1, 2.0f, 2.0f, {0.6f, 0.4f, 0.2f},  59, 1.2f, 0.0f);
    regTool(2,  "Stone Pickaxe",    "pickaxe", 2, 4.0f, 3.0f, {0.5f, 0.5f, 0.5f},  131, 1.2f, 0.0f);
    regTool(3,  "Iron Pickaxe",     "pickaxe", 3, 6.0f, 4.0f, {0.85f, 0.85f, 0.85f}, 250, 1.2f, 0.0f);
    regTool(4,  "Diamond Pickaxe",  "pickaxe", 4, 8.0f, 5.0f, {0.3f, 0.9f, 0.9f}, 1561, 1.2f, 0.0f);
    // Axes (Minecraft: 7/9/9/9 damage, 0.8/0.8/0.9/1.0 attack speed, axes deal more damage than swords)
    regTool(5,  "Wooden Axe",       "axe", 1, 2.0f, 7.0f, {0.6f, 0.4f, 0.2f},  59, 0.8f, 0.3f);
    regTool(6,  "Stone Axe",        "axe", 2, 4.0f, 9.0f, {0.5f, 0.5f, 0.5f},  131, 0.8f, 0.3f);
    regTool(7,  "Iron Axe",         "axe", 3, 6.0f, 9.0f, {0.85f, 0.85f, 0.85f}, 250, 0.9f, 0.3f);
    regTool(8,  "Diamond Axe",      "axe", 4, 8.0f, 9.0f, {0.3f, 0.9f, 0.9f}, 1561, 1.0f, 0.3f);
    // Shovels (Minecraft: 2.5/3.5/4.5/5.5 damage, 1.0/s)
    regTool(9,  "Wooden Shovel",    "shovel", 1, 2.0f, 2.5f, {0.6f, 0.4f, 0.2f},  59, 1.0f, 0.0f);
    regTool(10, "Stone Shovel",     "shovel", 2, 4.0f, 3.5f, {0.5f, 0.5f, 0.5f},  131, 1.0f, 0.0f);
    regTool(11, "Iron Shovel",      "shovel", 3, 6.0f, 4.5f, {0.85f, 0.85f, 0.85f}, 250, 1.0f, 0.0f);
    regTool(12, "Diamond Shovel",   "shovel", 4, 8.0f, 5.5f, {0.3f, 0.9f, 0.9f}, 1561, 1.0f, 0.0f);
    // Swords (Minecraft exact: 4/5/6/7 damage, 1.6/s attack speed)
    regTool(13, "Wooden Sword",     "sword", 1, 1.0f, 4.0f, {0.6f, 0.4f, 0.2f},  59, 1.6f, 0.4f);
    regTool(14, "Stone Sword",      "sword", 2, 1.0f, 5.0f, {0.5f, 0.5f, 0.5f},  131, 1.6f, 0.4f);
    regTool(15, "Iron Sword",       "sword", 3, 1.0f, 6.0f, {0.85f, 0.85f, 0.85f}, 250, 1.6f, 0.4f);
    regTool(16, "Diamond Sword",    "sword", 4, 1.0f, 7.0f, {0.3f, 0.9f, 0.9f}, 1561, 1.6f, 0.5f);
    // Hoes (Minecraft: 1 damage, increasing speed per tier)
    regTool(17, "Wooden Hoe",       "hoe", 1, 1.0f, 1.0f, {0.6f, 0.4f, 0.2f},  59, 1.0f);
    regTool(18, "Stone Hoe",        "hoe", 2, 1.0f, 1.0f, {0.5f, 0.5f, 0.5f},  131, 2.0f);
    regTool(19, "Iron Hoe",         "hoe", 3, 1.0f, 1.0f, {0.85f, 0.85f, 0.85f}, 250, 3.0f);
    regTool(20, "Diamond Hoe",      "hoe", 4, 1.0f, 1.0f, {0.3f, 0.9f, 0.9f}, 1561, 4.0f);
    // Bow & Shield
    regTool(21, "Bow",              "bow", 1, 1.0f, 6.0f, {0.6f, 0.4f, 0.2f}, 384, 1.0f, 0.0f);
    regTool(22, "Shield",           "shield", 1, 1.0f, 1.0f, {0.6f, 0.4f, 0.2f}, 336, 0.0f, 2.0f);
    regTool(23, "Fishing Rod",      "fishing_rod", 1, 1.0f, 0.0f, {0.6f, 0.4f, 0.2f}, 64, 0.0f, 0.0f);

    // Fire Aspect: Iron Sword (id 15) and Diamond Sword (id 16) set mobs on fire
    if (m_tools.count(15)) m_tools[15].specialEffect = "fire_aspect";
    if (m_tools.count(16)) m_tools[16].specialEffect = "fire_aspect";

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

    // Additional flammable blocks
    setPhysics(BLOCK_BIRCH_WOOD,    0.6f, 0.0f, 15, true, 300, false, 0);
    setPhysics(BLOCK_BIRCH_LEAVES,  0.6f, 0.0f, 1,  true,  60, false, 0);
    setPhysics(BLOCK_CHERRY_WOOD,   0.6f, 0.0f, 15, true, 300, false, 0);
    setPhysics(BLOCK_CHERRY_LEAVES, 0.6f, 0.0f, 1,  true,  60, false, 0);

    // New biome block physics (inorganic / non-flammable)
    setPhysics(BLOCK_GRAVEL,         0.6f, 0.0f, 15, false, 0, false, 0);
    setPhysics(BLOCK_ASH,            0.6f, 0.0f, 15, false, 0, false, 0);
    setPhysics(BLOCK_BASALT,         0.7f, 0.0f, 15, false, 0, false, 0);
    setPhysics(BLOCK_PUMICE,         0.6f, 0.0f, 15, false, 0, false, 0);
    setPhysics(BLOCK_SCORCHED_GRASS, 0.6f, 0.0f, 15, false, 0, false, 0);

    // Wool variants (IDs 46-61)
    for (uint8_t wid = 46; wid <= 61; ++wid) {
        auto it = m_blocks.find(wid);
        if (it != m_blocks.end()) {
            it->second.flammable = true;
            it->second.burnTime  = 200;
        }
    }

    // Bookshelf (68), Oak Fence (106), Chest (75), Crafting Table (79)
    for (uint8_t bid : {(uint8_t)68, (uint8_t)75, (uint8_t)79, (uint8_t)106}) {
        auto it = m_blocks.find(bid);
        if (it != m_blocks.end()) {
            it->second.flammable = true;
            it->second.burnTime  = 300;
        }
    }
}
