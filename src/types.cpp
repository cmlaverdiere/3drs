#include "types.h"

// WARNING: Keep in sync with Skill enum in types.h - add new skills at the end!
const char* SKILL_NAMES[SKILL_COUNT] = {
    "Combat", "Hitpoints", "Ranged", "Prayer", "Magic", "Woodcutting"
};

// WARNING: Keep in sync with ItemType enum in types.h - add new items at the end!
const char* ITEM_NAMES[ITEM_COUNT] = {
    "Nothing",
    "Bronze Shortsword",
    "Cow Hide",
    "Bones",
    "Gil",
    "Bronze Axe",
    "Logs",
    "Chitin",
    "Iron 2H Sword",
    // Trading Expedition Quest Items
    "Trade Manifest",
    "Silk",
    "Spice",
    "Iron Ore",
    "Rare Wine",
    "Bandit Orders",
    "Desert Artifact",
    "Trade Ledger",
    // Scimitar Shop Items
    "Steel Scimitar",
    "Mithril Scimitar",
    "Adamant Scimitar",
    // Woodcutting Items
    "Oak Logs"
};

// NPC configurations with dialogue
const NPCConfig NPC_CONFIGS[NPC_COUNT] = {
    // NPC_HANS - Friendly wandering townsperson
    {
        .name = "Hans",
        .skinColor = {255, 220, 180, 255},   // Light peach skin
        .shirtColor = {100, 100, 200, 255},  // Blue shirt
        .pantsColor = {80, 60, 40, 255},     // Brown pants
        .height = 1.0f,
        .dialogueLines = {
            "Hello there, adventurer! Welcome to Lumbridge.",
            "I've been wandering around here for... well, forever it seems.",
            "The bank is to the north, and there's a general store to the west.",
            "Watch out for goblins across the river - they can be nasty!"
        },
        .dialogueCount = 4
    },
    // NPC_SHOPKEEPER - General store merchant
    {
        .name = "Shopkeeper",
        .skinColor = {240, 200, 160, 255},   // Tan skin
        .shirtColor = {180, 140, 100, 255},  // Beige apron
        .pantsColor = {60, 60, 60, 255},     // Dark gray pants
        .height = 1.0f,
        .dialogueLines = {
            "Welcome to the Lumbridge General Store!",
            "I buy and sell all manner of goods.",
            "My stock is a bit limited at the moment...",
            "Perhaps you have some items to sell?"
        },
        .dialogueCount = 4
    },
    // NPC_GUARD - Stern castle guard
    {
        .name = "Guard",
        .skinColor = {220, 180, 140, 255},   // Medium skin
        .shirtColor = {140, 140, 150, 255},  // Chainmail gray
        .pantsColor = {100, 100, 110, 255},  // Armor gray
        .height = 1.1f,                       // Slightly taller
        .dialogueLines = {
            "Halt! State your business.",
            "...Oh, just an adventurer. Very well, carry on.",
            "Keep your weapons sheathed in town, understood?"
        },
        .dialogueCount = 3
    },
    // NPC_COOK - Frantic castle cook
    {
        .name = "Cook",
        .skinColor = {255, 210, 170, 255},   // Fair skin
        .shirtColor = {255, 255, 255, 255},  // White chef coat
        .pantsColor = {40, 40, 40, 255},     // Black pants
        .height = 0.95f,                      // Slightly shorter
        .dialogueLines = {
            "Oh dear, oh dear!",
            "The Duke's birthday is today and I need to bake a cake!",
            "I need eggs, flour, and milk... but I'm all out!",
            "Could you help me? ...No? Well, worth asking."
        },
        .dialogueCount = 4
    },
    // NPC_VARROCK_TRADER - Zaff, general goods merchant in Varrock
    {
        .name = "Zaff",
        .skinColor = {220, 190, 160, 255},   // Tanned skin
        .shirtColor = {120, 80, 60, 255},    // Brown merchant outfit
        .pantsColor = {60, 50, 40, 255},     // Dark brown pants
        .height = 1.05f,
        .dialogueLines = {
            "Welcome to Zaff's Superior Goods!",
            "I trade with merchants from all across Gielinor.",
            "Looking for rare imports? You've come to the right place."
        },
        .dialogueCount = 3
    },
    // NPC_VARROCK_BARTENDER - Blue Moon Inn
    {
        .name = "Bartender",
        .skinColor = {240, 210, 180, 255},   // Fair skin
        .shirtColor = {80, 60, 120, 255},    // Purple vest
        .pantsColor = {40, 40, 40, 255},     // Black pants
        .height = 0.98f,
        .dialogueLines = {
            "Welcome to the Blue Moon Inn!",
            "Can I get you a drink? We have the finest wines.",
            "Watch out for those bandits on the roads lately..."
        },
        .dialogueCount = 3
    },
    // NPC_ALKHARID_SILK - Silk trader in Al Kharid
    {
        .name = "Silk Merchant",
        .skinColor = {180, 140, 100, 255},   // Desert tan
        .shirtColor = {200, 180, 140, 255},  // Cream robes
        .pantsColor = {160, 140, 100, 255},  // Light tan pants
        .height = 1.0f,
        .dialogueLines = {
            "Finest silk in all the desert!",
            "Imported from the eastern lands.",
            "Only 3 gold pieces per roll!"
        },
        .dialogueCount = 3
    },
    // NPC_ALKHARID_SPICE - Ali the spice trader
    {
        .name = "Ali the Spice Trader",
        .skinColor = {170, 130, 90, 255},    // Desert tan
        .shirtColor = {180, 60, 40, 255},    // Red merchant outfit
        .pantsColor = {100, 80, 60, 255},    // Brown pants
        .height = 0.95f,
        .dialogueLines = {
            "Spices! Get your exotic spices here!",
            "Straight from the heart of the desert.",
            "The sand golems guard the best spice fields..."
        },
        .dialogueCount = 3
    },
    // NPC_SCIMITAR_SHOP - Zeke, Varrock scimitar seller
    {
        .name = "Zeke",
        .skinColor = {230, 190, 160, 255},   // Light tan
        .shirtColor = {100, 80, 60, 255},    // Dark leather apron
        .pantsColor = {50, 45, 40, 255},     // Dark pants
        .height = 1.02f,
        .dialogueLines = {
            "Welcome to Zeke's Superior Scimitars!",
            "I have the finest curved blades in Varrock.",
            "Scimitars are lighter and faster than regular swords."
        },
        .dialogueCount = 3
    }
};

const EnemyConfig ENEMY_CONFIGS[ENEMY_TYPE_COUNT] = {
    // ENEMY_TROLL
    {
        .name = "Troll",
        .combatLevel = 3,
        .maxHealth = 5,
        .maxHit = 2,
        .attackCooldown = 1.0f,
        .chaseSpeed = 3.0f,
        .attackRange = 2.0f,
        .respawnTime = 15.0f,
        .aggressive = false,
        .drops = {
            { ITEM_BONES, 1, 1, 1.0f },      // Always drops bones
            { ITEM_GIL, 5, 25, 1.0f },       // Always drops 5-25 gil
        },
        .dropCount = 2
    },
    // ENEMY_COW
    {
        .name = "Cow",
        .combatLevel = 2,
        .maxHealth = 8,
        .maxHit = 1,
        .attackCooldown = 2.0f,
        .chaseSpeed = 1.5f,
        .attackRange = 1.5f,
        .respawnTime = 10.0f,
        .aggressive = false,
        .drops = {
            { ITEM_COW_HIDE, 1, 1, 1.0f },   // Always drops cow hide
            { ITEM_BONES, 1, 1, 1.0f },      // Always drops bones
        },
        .dropCount = 2
    },
    // ENEMY_SCORPION
    {
        .name = "Scorpion",
        .combatLevel = 14,
        .maxHealth = 15,
        .maxHit = 4,
        .attackCooldown = 1.5f,
        .chaseSpeed = 2.5f,
        .attackRange = 1.8f,
        .respawnTime = 20.0f,
        .aggressive = true,  // Scorpions attack on sight!
        .drops = {
            { ITEM_CHITIN, 1, 1, 1.0f },     // Always drops chitin
            { ITEM_BONES, 1, 1, 1.0f },      // Always drops bones
            { ITEM_GIL, 10, 50, 0.8f },      // 80% chance 10-50 gil
        },
        .dropCount = 3
    },
    // ENEMY_BANDIT - Road bandits between cities
    {
        .name = "Bandit",
        .combatLevel = 10,
        .maxHealth = 20,
        .maxHit = 4,
        .attackCooldown = 1.2f,
        .chaseSpeed = 4.0f,
        .attackRange = 2.0f,
        .respawnTime = 30.0f,
        .aggressive = true,  // Bandits attack on sight!
        .drops = {
            { ITEM_BONES, 1, 1, 1.0f },          // Always drops bones
            { ITEM_GIL, 20, 50, 1.0f },          // Always drops 20-50 gil
            { ITEM_BANDIT_ORDERS, 1, 1, 0.5f },  // 50% chance quest item
        },
        .dropCount = 3
    },
    // ENEMY_SAND_GOLEM - Desert golem in Al Kharid
    {
        .name = "Sand Golem",
        .combatLevel = 18,
        .maxHealth = 35,
        .maxHit = 6,
        .attackCooldown = 2.0f,
        .chaseSpeed = 2.0f,
        .attackRange = 2.5f,
        .respawnTime = 45.0f,
        .aggressive = true,  // Sand golems attack on sight!
        .drops = {
            { ITEM_BONES, 1, 1, 1.0f },              // Always drops bones
            { ITEM_GIL, 30, 80, 1.0f },              // Always drops 30-80 gil
            { ITEM_DESERT_ARTIFACT, 1, 1, 0.4f },   // 40% chance quest item
            { ITEM_SPICE, 1, 2, 0.6f },              // 60% chance spice
        },
        .dropCount = 4
    }
};
