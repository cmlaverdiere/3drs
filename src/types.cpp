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
    "Iron 2H Sword"
};

const EnemyConfig ENEMY_CONFIGS[ENEMY_TYPE_COUNT] = {
    // ENEMY_TROLL
    {
        .name = "Troll",
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
    }
};
