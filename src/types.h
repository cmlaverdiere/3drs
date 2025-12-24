#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"
#include <cmath>

// ============================================================================
// GAME CONSTANTS
// ============================================================================

// Player movement
constexpr float WALK_SPEED = 5.0f;
constexpr float RUN_SPEED = 10.0f;
constexpr float ENERGY_DRAIN_RATE = 100.0f / 30.0f;   // Depletes in 30s
constexpr float ENERGY_REGEN_RATE = 100.0f / 120.0f;  // Regens in 120s
constexpr float MOUSE_SENSITIVITY = 0.003f;
constexpr float PLAYER_RADIUS = 0.3f;
constexpr float PLAYER_EYE_HEIGHT = 1.8f;

// Jump physics
constexpr float JUMP_FORCE = 8.0f;
constexpr float GRAVITY = 20.0f;

// Duck animation (burying bones)
constexpr float DUCK_DURATION = 0.6f;
constexpr float DUCK_DEPTH = 0.8f;
constexpr int BURY_XP = 5;

// Combat
constexpr float SWING_DURATION = 0.2f;
constexpr float PICKUP_RANGE = 2.5f;
constexpr float DEFAULT_ATTACK_COOLDOWN = 0.25f;
constexpr float IRON_2H_ATTACK_COOLDOWN = 0.5f;
constexpr float IRON_2H_DAMAGE_MULTIPLIER = 2.5f;

// HP regeneration
constexpr float HP_REGEN_INTERVAL = 5.0f;

// Death
constexpr float DEATH_FADE_DURATION = 2.0f;

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
    ITEM_CHITIN,
    ITEM_IRON_2H_SWORD,
    // === ADD NEW ITEMS HERE ===
    ITEM_COUNT
};

extern const char* ITEM_NAMES[ITEM_COUNT];

// Enemy types
enum EnemyType {
    ENEMY_TROLL = 0,
    ENEMY_COW,
    ENEMY_SCORPION,
    ENEMY_TYPE_COUNT
};

// NPC types (friendly, talkable characters)
enum NPCType {
    NPC_HANS = 0,        // Friendly wandering townsperson
    NPC_SHOPKEEPER,      // General store merchant
    NPC_GUARD,           // Stern castle guard
    NPC_COOK,            // Frantic castle cook
    NPC_COUNT
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
    float respawnTimer;      // Time until respawn (0 = no respawn)
    Vector3 spawnPosition;   // Original spawn point for respawning items
    bool canRespawn;         // True for map-spawned items
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
    int combatLevel;
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

// NPC configuration (static data)
constexpr int MAX_NPC_DIALOGUE_LINES = 8;

struct NPCConfig {
    const char* name;
    Color skinColor;
    Color shirtColor;
    Color pantsColor;
    float height;  // Height multiplier (1.0 = standard)
    const char* dialogueLines[MAX_NPC_DIALOGUE_LINES];
    int dialogueCount;
};

extern const NPCConfig NPC_CONFIGS[NPC_COUNT];

// NPC instance (runtime data)
struct NPC {
    Vector3 position;
    NPCType type;
    float facingAngle;
    float targetFacingAngle;
    bool active;
};

// Dialogue state (for active conversation)
struct DialogueState {
    bool active;
    int npcIndex;
    int currentLine;
};

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

// Water body (river, lake, pond)
struct Water {
    Vector3 position;  // Center position
    float width;       // X extent
    float length;      // Z extent
};

// Sand zone (desert terrain)
struct Sand {
    Vector3 position;  // Center position
    float width;       // X extent
    float length;      // Z extent
};

// Terrain valley (carves into heightmap)
struct Valley {
    float position;    // X or Z position of valley center
    float width;       // Half-width of valley
    float depth;       // How deep to carve
    int axis;          // 0 = X-axis (N-S), 1 = Z-axis (E-W)
};

const int MAX_TREES = 1000;
const int MAX_WATER = 100;
const float ITEM_RESPAWN_TIME = 60.0f;  // 60 seconds for respawning items
const int MAX_SAND = 50;
const int MAX_VALLEYS = 50;
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

// ============================================================================
// QUEST SYSTEM
// ============================================================================

// Quest state
enum QuestState {
    QUEST_NOT_STARTED = 0,
    QUEST_IN_PROGRESS,
    QUEST_COMPLETE
};

// Quest limits
constexpr int MAX_QUESTS = 32;
constexpr int MAX_QUEST_OBJECTIVES = 8;
constexpr int MAX_QUEST_DIALOGUE_LINES = 16;

// Quest definition (loaded from file)
struct Quest {
    char id[32];                    // "pest_control"
    char name[64];                  // "Pest Control"
    NPCType npc;                    // Which NPC gives this quest

    // Objectives (sequential item turn-ins)
    ItemType objectives[MAX_QUEST_OBJECTIVES];
    int objectiveCount;

    // Rewards
    int rewardGil;
    int rewardQuestPoints;

    // Dialogue storage (heap allocated during load)
    char** dialogueStart;
    int dialogueStartCount;
    char** dialogueStage[MAX_QUEST_OBJECTIVES];
    int dialogueStageCount[MAX_QUEST_OBJECTIVES];
    char** dialogueTurnin[MAX_QUEST_OBJECTIVES];
    int dialogueTurninCount[MAX_QUEST_OBJECTIVES];
    char** dialogueComplete;
    int dialogueCompleteCount;

    bool loaded;
};

// Player's quest progress (per quest, by quest index)
struct QuestProgress {
    QuestState state;
    int currentObjective;  // Which objective (0-indexed)
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
    float timeOfDay;  // 0.0 to 1.0, for day/night cycle persistence
    int questPoints;
    QuestProgress questProgress[MAX_QUESTS];
};

// Check if an item type is stackable
inline bool IsItemStackable(ItemType item) {
    return item == ITEM_GIL;
}

// Check if an item is a weapon
inline bool IsWeapon(ItemType item) {
    return item == ITEM_BRONZE_SHORTSWORD || item == ITEM_BRONZE_AXE || item == ITEM_IRON_2H_SWORD;
}

// Get weapon attack cooldown
inline float GetWeaponCooldown(ItemType item) {
    if (item == ITEM_IRON_2H_SWORD) return IRON_2H_ATTACK_COOLDOWN;
    return DEFAULT_ATTACK_COOLDOWN;
}

// Get weapon damage multiplier
inline float GetWeaponDamageMultiplier(ItemType item) {
    if (item == ITEM_IRON_2H_SWORD) return IRON_2H_DAMAGE_MULTIPLIER;
    return 1.0f;
}

// Normalize angle to [-PI, PI]
inline float NormalizeAngle(float angle) {
    while (angle > PI) angle -= 2.0f * PI;
    while (angle < -PI) angle += 2.0f * PI;
    return angle;
}

// Smooth turn toward target angle
inline float SmoothTurn(float current, float target, float maxTurn) {
    float diff = NormalizeAngle(target - current);
    if (fabsf(diff) < maxTurn) return target;
    return current + (diff > 0 ? maxTurn : -maxTurn);
}

// Constants
const int MAX_ENEMIES = 500;
const int MAX_WORLD_ITEMS = 500;
const int MAX_WALLS = 1000;
const int MAX_NPCS = 32;
const int MAX_DAMAGE_INDICATORS = 20;
const int MAX_XP_POPUPS = 10;

// NPC interaction
constexpr float NPC_INTERACTION_RANGE = 3.0f;
constexpr float NPC_TURN_SPEED = 4.0f;

const float PLAYER_ATTACK_RANGE = 5.0f;

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
    Water waterBodies[MAX_WATER];
    int waterCount;
    Sand sandZones[MAX_SAND];
    int sandCount;
    Valley valleys[MAX_VALLEYS];
    int valleyCount;
    Vector3 npcSpawns[MAX_NPCS];
    NPCType npcTypes[MAX_NPCS];
    int npcCount;
};

#endif
