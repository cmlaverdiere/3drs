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
// WARNING: Only add new items BEFORE ITEM_COUNT, never reorder existing items!
// Reordering will corrupt existing save files since inventory stores item IDs.
enum ItemType {
    ITEM_NONE = 0,
    ITEM_BRONZE_SHORTSWORD,
    ITEM_COW_HIDE,
    ITEM_BONES,
    ITEM_GIL,
    ITEM_BRONZE_AXE,
    ITEM_LOGS,
    // === ADD NEW ITEMS HERE ===
    ITEM_COUNT
};

extern const char* ITEM_NAMES[ITEM_COUNT];

// Enemy types
enum EnemyType {
    ENEMY_TROLL = 0,
    ENEMY_COW,
    ENEMY_TYPE_COUNT
};

// Wall material types
enum WallMaterial {
    WALL_WOOD = 0,
    WALL_STONE,
    WALL_BRICK,
    WALL_MATERIAL_COUNT
};

// Skill indices
// WARNING: Only add new skills BEFORE SKILL_COUNT, never reorder existing skills!
// Reordering will corrupt existing save files since skills are saved by index.
enum Skill {
    SKILL_COMBAT = 0,
    SKILL_HITPOINTS,
    SKILL_RANGED,
    SKILL_PRAYER,
    SKILL_MAGIC,
    SKILL_WOODCUTTING,
    // === ADD NEW SKILLS HERE ===
    SKILL_COUNT
};

extern const char* SKILL_NAMES[SKILL_COUNT];

// World item (item on the ground)
struct WorldItem {
    ItemType type;
    Vector3 position;
    bool pickedUp;
};

// Drop table entry
struct DropEntry {
    ItemType item;
    int minAmount;
    int maxAmount;
    float chance;  // 0.0 to 1.0
};

const int MAX_DROPS_PER_ENEMY = 4;

// Enemy type configuration
struct EnemyConfig {
    const char* name;
    int maxHealth;
    int maxHit;
    float attackCooldown;
    float chaseSpeed;
    float attackRange;
    float respawnTime;
    bool aggressive;  // Attacks on sight vs only when attacked
    DropEntry drops[MAX_DROPS_PER_ENEMY];
    int dropCount;
};

extern const EnemyConfig ENEMY_CONFIGS[ENEMY_TYPE_COUNT];

// Generic Enemy NPC
struct Enemy {
    EnemyType type;
    Vector3 position;
    Vector3 spawnPoint;
    int health;
    bool alive;
    float respawnTimer;
    float wanderTimer;
    Vector3 wanderTarget;
    bool hostile;
    float attackCooldown;
    float facingAngle;  // Rotation in radians (0 = facing +Z)
};

// Wall structure (for buildings/barriers)
struct Wall {
    Vector3 position;
    float width;
    float height;
    float depth;
    WallMaterial material;
};

// Tree structure (choppable resource)
struct Tree {
    Vector3 position;
    int health;       // Chops remaining before felling
    bool alive;       // False when cut down
    float respawnTimer;
};

const int MAX_TREES = 100;
const int TREE_MAX_HEALTH = 3;      // 3 chops to fell a tree
const float TREE_RESPAWN_TIME = 30.0f;
const int WOODCUTTING_XP = 25;       // XP per log
const float CHOP_RANGE = 3.0f;

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
    int inventoryCount[INV_SLOTS];  // Stack count for each slot (1 for non-stackable)
    ItemType equippedWeapon;
    bool swordPickedUp;
    int currentHP;
    int maxHP;
};

// Check if an item type is stackable
inline bool IsItemStackable(ItemType item) {
    return item == ITEM_GIL;
}

// Constants
const int MAX_ENEMIES = 50;
const int MAX_WORLD_ITEMS = 100;
const int MAX_WALLS = 100;
const int MAX_DAMAGE_INDICATORS = 20;
const int MAX_XP_POPUPS = 10;

const float PLAYER_ATTACK_RANGE = 5.0f;
const float PLAYER_ATTACK_COOLDOWN = 0.25f;

const float DAMAGE_INDICATOR_DURATION = 1.5f;
const float XP_POPUP_DURATION = 2.0f;
const float LEVEL_UP_DURATION = 5.0f;

// Map data loaded from file
struct MapData {
    Vector3 playerSpawn;
    Vector3 itemSpawns[MAX_WORLD_ITEMS];
    ItemType itemTypes[MAX_WORLD_ITEMS];
    int itemCount;
    Vector3 enemySpawns[MAX_ENEMIES];
    EnemyType enemyTypes[MAX_ENEMIES];
    int enemyCount;
    Wall walls[MAX_WALLS];
    int wallCount;
    Vector3 treeSpawns[MAX_TREES];
    int treeCount;
};

#endif
