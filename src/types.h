#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"

// Inventory constants
const int INV_COLS = 4;
const int INV_ROWS = 8;
const int INV_SLOTS = INV_COLS * INV_ROWS;
const int SLOT_SIZE = 40;
const int SLOT_PADDING = 4;

// Item types
enum ItemType {
    ITEM_NONE = 0,
    ITEM_BRONZE_SHORTSWORD
};

// Skill indices
enum Skill {
    SKILL_COMBAT = 0,
    SKILL_HITPOINTS,
    SKILL_RANGED,
    SKILL_PRAYER,
    SKILL_MAGIC,
    SKILL_COUNT
};

extern const char* SKILL_NAMES[SKILL_COUNT];

// World item (item on the ground)
struct WorldItem {
    ItemType type;
    Vector3 position;
    bool pickedUp;
};

// Troll NPC
struct Troll {
    Vector3 position;
    Vector3 spawnPoint;
    int health;
    int maxHealth;
    bool alive;
    float respawnTimer;
    float wanderTimer;
    Vector3 wanderTarget;
    bool hostile;
    float attackCooldown;
};

// Wall structure (for buildings/barriers)
struct Wall {
    Vector3 position;
    float width;
    float height;
    float depth;
    Color color;
};

// Floating damage indicator
struct DamageIndicator {
    Vector3 position;
    int damage;
    float timer;
    bool active;
};

// XP popup notification (MW2 style)
struct XPPopup {
    int xpAmount;
    int skillIndex;
    float timer;
    bool active;
};

// Level up notification
struct LevelUpNotification {
    int skillIndex;
    int newLevel;
    float timer;
    bool active;
};

// Player state
struct PlayerState {
    float posX, posY, posZ;
    float targetX, targetY, targetZ;
    int skillXP[SKILL_COUNT];
    ItemType inventory[INV_SLOTS];
    ItemType equippedWeapon;
    bool swordPickedUp;
    int currentHP;
    int maxHP;
};

// Constants
const int MAX_TROLLS = 20;
const int MAX_WORLD_ITEMS = 50;
const int MAX_WALLS = 100;
const int MAX_DAMAGE_INDICATORS = 20;
const int MAX_XP_POPUPS = 10;

const int TROLL_MAX_HEALTH = 5;
const float TROLL_RESPAWN_TIME = 15.0f;
const float ATTACK_RANGE = 5.0f;
const float ATTACK_COOLDOWN = 0.25f;
const float TROLL_ATTACK_COOLDOWN = 1.0f;
const float TROLL_ATTACK_RANGE = 2.0f;
const float TROLL_CHASE_SPEED = 3.0f;
const int TROLL_MAX_HIT = 2;

const float DAMAGE_INDICATOR_DURATION = 1.5f;
const float XP_POPUP_DURATION = 2.0f;
const float LEVEL_UP_DURATION = 5.0f;

// Map data loaded from file
struct MapData {
    Vector3 playerSpawn;
    Vector3 itemSpawns[MAX_WORLD_ITEMS];
    ItemType itemTypes[MAX_WORLD_ITEMS];
    int itemCount;
    Vector3 trollSpawns[MAX_TROLLS];
    int trollCount;
    Wall walls[MAX_WALLS];
    int wallCount;
};

#endif
