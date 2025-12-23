#include "raylib.h"
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstring>
#include <sys/stat.h>

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

const int MAX_TROLLS = 20;
const int MAX_WORLD_ITEMS = 50;
const int MAX_WALLS = 100;

// Wall structure (for buildings/barriers)
struct Wall {
    Vector3 position;  // Center position
    float width;       // X dimension
    float height;      // Y dimension
    float depth;       // Z dimension
    Color color;
};
const int TROLL_MAX_HEALTH = 5;
const float TROLL_RESPAWN_TIME = 15.0f;
const float ATTACK_RANGE = 5.0f;
const float ATTACK_COOLDOWN = 0.25f;
const float TROLL_ATTACK_COOLDOWN = 1.0f;
const float TROLL_ATTACK_RANGE = 2.0f;
const float TROLL_CHASE_SPEED = 3.0f;
const int TROLL_MAX_HIT = 2;

// Floating damage indicator
struct DamageIndicator {
    Vector3 position;
    int damage;
    float timer;
    bool active;
};

const int MAX_DAMAGE_INDICATORS = 20;
const float DAMAGE_INDICATOR_DURATION = 1.5f;

// XP popup notification (MW2 style)
struct XPPopup {
    int xpAmount;
    int skillIndex;
    float timer;
    bool active;
};

const int MAX_XP_POPUPS = 10;
const float XP_POPUP_DURATION = 2.0f;

// Level up notification
struct LevelUpNotification {
    int skillIndex;
    int newLevel;
    float timer;
    bool active;
};

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

// Load map from file
bool LoadMap(const char* filename, MapData& map) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        TraceLog(LOG_ERROR, "Failed to load map: %s", filename);
        return false;
    }

    map.playerSpawn = { 0.0f, 1.8f, 0.0f };
    map.itemCount = 0;
    map.trollCount = 0;
    map.wallCount = 0;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        char type[64];
        if (sscanf(line, "%63s", type) != 1) continue;

        if (strcmp(type, "player_spawn") == 0) {
            sscanf(line, "%*s %f %f %f", &map.playerSpawn.x, &map.playerSpawn.y, &map.playerSpawn.z);
        }
        else if (strcmp(type, "item") == 0) {
            if (map.itemCount < MAX_WORLD_ITEMS) {
                char itemName[64];
                float x, y, z;
                if (sscanf(line, "%*s %63s %f %f %f", itemName, &x, &y, &z) == 4) {
                    if (strcmp(itemName, "bronze_shortsword") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_BRONZE_SHORTSWORD;
                    } else {
                        map.itemTypes[map.itemCount] = ITEM_NONE;
                    }
                    map.itemSpawns[map.itemCount] = { x, y, z };
                    map.itemCount++;
                }
            }
        }
        else if (strcmp(type, "troll") == 0) {
            if (map.trollCount < MAX_TROLLS) {
                float x, y, z;
                if (sscanf(line, "%*s %f %f %f", &x, &y, &z) == 3) {
                    map.trollSpawns[map.trollCount] = { x, y, z };
                    map.trollCount++;
                }
            }
        }
        else if (strcmp(type, "wall") == 0) {
            if (map.wallCount < MAX_WALLS) {
                float x, y, z, w, h, d;
                int r, g, b;
                if (sscanf(line, "%*s %f %f %f %f %f %f %d %d %d", &x, &y, &z, &w, &h, &d, &r, &g, &b) == 9) {
                    map.walls[map.wallCount].position = { x, y, z };
                    map.walls[map.wallCount].width = w;
                    map.walls[map.wallCount].height = h;
                    map.walls[map.wallCount].depth = d;
                    map.walls[map.wallCount].color = { (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
                    map.wallCount++;
                }
            }
        }
    }

    fclose(f);
    TraceLog(LOG_INFO, "Loaded map: %s (%d items, %d trolls, %d walls)", filename, map.itemCount, map.trollCount, map.wallCount);
    return true;
}

// OSRS XP table (XP required for each level 1-99)
// Formula: floor(sum from x=1 to L-1 of floor(x + 300 * 2^(x/7)) / 4)
const int XP_TABLE[100] = {
    0,           // Level 1
    83,          // Level 2
    174,         // Level 3
    276,         // Level 4
    388,         // Level 5
    512,         // Level 6
    650,         // Level 7
    801,         // Level 8
    969,         // Level 9
    1154,        // Level 10
    1358,        // Level 11
    1584,        // Level 12
    1833,        // Level 13
    2107,        // Level 14
    2411,        // Level 15
    2746,        // Level 16
    3115,        // Level 17
    3523,        // Level 18
    3973,        // Level 19
    4470,        // Level 20
    5018,        // Level 21
    5624,        // Level 22
    6291,        // Level 23
    7028,        // Level 24
    7842,        // Level 25
    8740,        // Level 26
    9730,        // Level 27
    10824,       // Level 28
    12031,       // Level 29
    13363,       // Level 30
    14833,       // Level 31
    16456,       // Level 32
    18247,       // Level 33
    20224,       // Level 34
    22406,       // Level 35
    24815,       // Level 36
    27473,       // Level 37
    30408,       // Level 38
    33648,       // Level 39
    37224,       // Level 40
    41171,       // Level 41
    45529,       // Level 42
    50339,       // Level 43
    55649,       // Level 44
    61512,       // Level 45
    67983,       // Level 46
    75127,       // Level 47
    83014,       // Level 48
    91721,       // Level 49
    101333,      // Level 50
    111945,      // Level 51
    123660,      // Level 52
    136594,      // Level 53
    150872,      // Level 54
    166636,      // Level 55
    184040,      // Level 56
    203254,      // Level 57
    224466,      // Level 58
    247886,      // Level 59
    273742,      // Level 60
    302288,      // Level 61
    333804,      // Level 62
    368599,      // Level 63
    407015,      // Level 64
    449428,      // Level 65
    496254,      // Level 66
    547953,      // Level 67
    605032,      // Level 68
    668051,      // Level 69
    737627,      // Level 70
    814445,      // Level 71
    899257,      // Level 72
    992895,      // Level 73
    1096278,     // Level 74
    1210421,     // Level 75
    1336443,     // Level 76
    1475581,     // Level 77
    1629200,     // Level 78
    1798808,     // Level 79
    1986068,     // Level 80
    2192818,     // Level 81
    2421087,     // Level 82
    2673114,     // Level 83
    2951373,     // Level 84
    3258594,     // Level 85
    3597792,     // Level 86
    3972294,     // Level 87
    4385776,     // Level 88
    4842295,     // Level 89
    5346332,     // Level 90
    5902831,     // Level 91
    6517253,     // Level 92
    7195629,     // Level 93
    7944614,     // Level 94
    8771558,     // Level 95
    9684577,     // Level 96
    10692629,    // Level 97
    11805606,    // Level 98
    13034431,    // Level 99
};

// Get level from XP
int GetLevelFromXP(int xp) {
    for (int level = 98; level >= 0; level--) {
        if (xp >= XP_TABLE[level]) {
            return level + 1;
        }
    }
    return 1;
}

// Skill indices
enum Skill {
    SKILL_COMBAT = 0,
    SKILL_HITPOINTS,
    SKILL_RANGED,
    SKILL_PRAYER,
    SKILL_MAGIC,
    SKILL_COUNT
};

const char* SKILL_NAMES[SKILL_COUNT] = {
    "Combat", "Hitpoints", "Ranged", "Prayer", "Magic"
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

const char* SAVE_FILE = "savegame.json";

// Simple JSON save
void SaveGame(const PlayerState& state) {
    FILE* f = fopen(SAVE_FILE, "w");
    if (!f) return;

    fprintf(f, "{\n");
    fprintf(f, "  \"position\": { \"x\": %.4f, \"y\": %.4f, \"z\": %.4f },\n", state.posX, state.posY, state.posZ);
    fprintf(f, "  \"target\": { \"x\": %.4f, \"y\": %.4f, \"z\": %.4f },\n", state.targetX, state.targetY, state.targetZ);
    fprintf(f, "  \"skills\": {\n");
    for (int i = 0; i < SKILL_COUNT; i++) {
        fprintf(f, "    \"%s\": %d%s\n", SKILL_NAMES[i], state.skillXP[i], i < SKILL_COUNT - 1 ? "," : "");
    }
    fprintf(f, "  },\n");
    fprintf(f, "  \"inventory\": [");
    for (int i = 0; i < INV_SLOTS; i++) {
        fprintf(f, "%d%s", state.inventory[i], i < INV_SLOTS - 1 ? ", " : "");
    }
    fprintf(f, "],\n");
    fprintf(f, "  \"equippedWeapon\": %d,\n", state.equippedWeapon);
    fprintf(f, "  \"swordPickedUp\": %s,\n", state.swordPickedUp ? "true" : "false");
    fprintf(f, "  \"currentHP\": %d,\n", state.currentHP);
    fprintf(f, "  \"maxHP\": %d\n", state.maxHP);
    fprintf(f, "}\n");
    fclose(f);
}

// Helper to find a number after a key in JSON
int ParseIntAfter(const char* json, const char* key, int defaultVal) {
    const char* pos = strstr(json, key);
    if (!pos) return defaultVal;
    pos = strchr(pos, ':');
    if (!pos) return defaultVal;
    return atoi(pos + 1);
}

float ParseFloatAfter(const char* json, const char* key, float defaultVal) {
    const char* pos = strstr(json, key);
    if (!pos) return defaultVal;
    pos = strchr(pos, ':');
    if (!pos) return defaultVal;
    return (float)atof(pos + 1);
}

bool ParseBoolAfter(const char* json, const char* key, bool defaultVal) {
    const char* pos = strstr(json, key);
    if (!pos) return defaultVal;
    pos = strchr(pos, ':');
    if (!pos) return defaultVal;
    while (*pos && (*pos == ':' || *pos == ' ')) pos++;
    return strncmp(pos, "true", 4) == 0;
}

// Simple JSON load
bool LoadGame(PlayerState& state) {
    FILE* f = fopen(SAVE_FILE, "r");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* json = new char[size + 1];
    fread(json, 1, size, f);
    json[size] = '\0';
    fclose(f);

    // Parse position
    const char* posSection = strstr(json, "\"position\"");
    if (posSection) {
        state.posX = ParseFloatAfter(posSection, "\"x\"", 0.0f);
        state.posY = ParseFloatAfter(posSection, "\"y\"", 1.8f);
        state.posZ = ParseFloatAfter(posSection, "\"z\"", 0.0f);
    }

    // Parse target
    const char* targetSection = strstr(json, "\"target\"");
    if (targetSection) {
        state.targetX = ParseFloatAfter(targetSection, "\"x\"", 0.0f);
        state.targetY = ParseFloatAfter(targetSection, "\"y\"", 1.8f);
        state.targetZ = ParseFloatAfter(targetSection, "\"z\"", 1.0f);
    }

    // Parse skills
    const char* skillsSection = strstr(json, "\"skills\"");
    if (skillsSection) {
        for (int i = 0; i < SKILL_COUNT; i++) {
            char key[64];
            snprintf(key, sizeof(key), "\"%s\"", SKILL_NAMES[i]);
            // Search within skills section only
            const char* skillPos = strstr(skillsSection, key);
            if (skillPos) {
                const char* colon = strchr(skillPos, ':');
                if (colon) {
                    state.skillXP[i] = atoi(colon + 1);
                }
            }
        }
    }

    // Parse inventory
    const char* invSection = strstr(json, "\"inventory\"");
    if (invSection) {
        const char* bracket = strchr(invSection, '[');
        if (bracket) {
            const char* p = bracket + 1;
            for (int i = 0; i < INV_SLOTS && *p; i++) {
                while (*p && (*p == ' ' || *p == ',')) p++;
                if (*p == ']') break;
                state.inventory[i] = (ItemType)atoi(p);
                while (*p && *p != ',' && *p != ']') p++;
            }
        }
    }

    state.equippedWeapon = (ItemType)ParseIntAfter(json, "\"equippedWeapon\"", ITEM_NONE);
    state.swordPickedUp = ParseBoolAfter(json, "\"swordPickedUp\"", false);
    state.currentHP = ParseIntAfter(json, "\"currentHP\"", 10);
    state.maxHP = ParseIntAfter(json, "\"maxHP\"", 10);

    delete[] json;
    return true;
}

// Distance helper
float Distance3D(Vector3 a, Vector3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

// Vector normalization
Vector3 Normalize3D(Vector3 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len > 0) {
        return { v.x/len, v.y/len, v.z/len };
    }
    return { 0, 0, 0 };
}

// Dot product
float Dot3D(Vector3 a, Vector3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

// Check if player is facing a target (within ~60 degree cone)
bool IsFacing(Camera3D& camera, Vector3 targetPos) {
    Vector3 forward = {
        camera.target.x - camera.position.x,
        0, // ignore Y for horizontal facing
        camera.target.z - camera.position.z
    };
    forward = Normalize3D(forward);

    Vector3 toTarget = {
        targetPos.x - camera.position.x,
        0,
        targetPos.z - camera.position.z
    };
    toTarget = Normalize3D(toTarget);

    float dot = Dot3D(forward, toTarget);
    return dot > 0.5f; // ~60 degree cone
}

// OSRS max hit calculation (simplified - no equipment)
// Max hit = floor(0.5 + effective_strength * (bonus + 64) / 640)
// Effective strength (aggressive) = strength_level + 8
int CalculateMaxHit(int strengthLevel) {
    int effectiveStrength = strengthLevel + 8;
    int maxHit = (int)(0.5f + effectiveStrength * 64.0f / 640.0f);
    if (maxHit < 1) maxHit = 1;
    return maxHit;
}

// Random hit from 0 to max (inclusive), 0 = miss
int RollDamage(int maxHit) {
    return GetRandomValue(0, maxHit);
}

// Spawn a damage indicator
void SpawnDamageIndicator(DamageIndicator* indicators, Vector3 pos, int damage) {
    for (int i = 0; i < MAX_DAMAGE_INDICATORS; i++) {
        if (!indicators[i].active) {
            indicators[i].position = pos;
            indicators[i].position.y += 1.5f; // Above troll head
            indicators[i].damage = damage;
            indicators[i].timer = DAMAGE_INDICATOR_DURATION;
            indicators[i].active = true;
            break;
        }
    }
}

// Random float in range
float RandomFloat(float min, float max) {
    return min + (float)GetRandomValue(0, 10000) / 10000.0f * (max - min);
}

// Spawn an XP popup
void SpawnXPPopup(XPPopup* popups, int xpAmount, int skillIndex) {
    for (int i = 0; i < MAX_XP_POPUPS; i++) {
        if (!popups[i].active) {
            popups[i].xpAmount = xpAmount;
            popups[i].skillIndex = skillIndex;
            popups[i].timer = XP_POPUP_DURATION;
            popups[i].active = true;
            break;
        }
    }
}

// Check if a point is inside a wall (with padding for player radius)
// Uses XZ collision only - player can't step over any wall regardless of height
bool PointInWall(Vector3 point, Wall& wall, float padding) {
    Vector3 wpos = wall.position;
    float halfW = wall.width / 2.0f + padding;
    float halfD = wall.depth / 2.0f + padding;

    return (point.x >= wpos.x - halfW && point.x <= wpos.x + halfW &&
            point.z >= wpos.z - halfD && point.z <= wpos.z + halfD);
}

// Resolve collision between player and wall, returns adjusted position
Vector3 ResolveWallCollision(Vector3 pos, Wall& wall, float padding) {
    Vector3 wpos = wall.position;
    float halfW = wall.width / 2.0f + padding;
    float halfD = wall.depth / 2.0f + padding;

    // Find nearest edge and push out
    float distLeft = pos.x - (wpos.x - halfW);
    float distRight = (wpos.x + halfW) - pos.x;
    float distBack = pos.z - (wpos.z - halfD);
    float distFront = (wpos.z + halfD) - pos.z;

    float minDist = distLeft;
    Vector3 result = pos;

    if (distRight < minDist) {
        minDist = distRight;
    }
    if (distBack < minDist) {
        minDist = distBack;
    }
    if (distFront < minDist) {
        minDist = distFront;
    }

    // Push out along the minimum penetration axis
    if (minDist == distLeft) {
        result.x = wpos.x - halfW;
    } else if (minDist == distRight) {
        result.x = wpos.x + halfW;
    } else if (minDist == distBack) {
        result.z = wpos.z - halfD;
    } else if (minDist == distFront) {
        result.z = wpos.z + halfD;
    }

    return result;
}

// Draw a simple sword shape
void DrawSword(Vector3 pos, Color bladeColor, Color handleColor) {
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.08f, 0.05f, 0.6f, bladeColor);
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z - 0.35f}, 0.06f, 0.08f, 0.15f, handleColor);
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z - 0.25f}, 0.2f, 0.04f, 0.04f, handleColor);
}

// Draw a troll (simple humanoid shape)
void DrawTroll(Vector3 pos, bool highlighted) {
    Color trollSkin = { 100, 140, 100, 255 };  // Greenish troll color
    Color trollDark = { 70, 100, 70, 255 };
    Color eyeColor = { 200, 50, 50, 255 };     // Angry red eyes
    Color eyeWhite = { 220, 220, 180, 255 };   // Yellowed eye whites
    Color browColor = { 50, 70, 50, 255 };     // Dark brow ridges
    Color mouthColor = { 40, 30, 30, 255 };    // Dark mouth

    // Body
    DrawCube((Vector3){pos.x, pos.y + 0.8f, pos.z}, 0.6f, 0.8f, 0.4f, trollSkin);
    // Head
    DrawSphere((Vector3){pos.x, pos.y + 1.5f, pos.z}, 0.35f, trollSkin);

    // Face - angry expression
    float headY = pos.y + 1.5f;
    float faceZ = pos.z + 0.30f;  // Front of face

    // Eye whites (slightly yellowed for menacing look)
    DrawSphere((Vector3){pos.x - 0.10f, headY + 0.05f, faceZ}, 0.07f, eyeWhite);
    DrawSphere((Vector3){pos.x + 0.10f, headY + 0.05f, faceZ}, 0.07f, eyeWhite);

    // Pupils (red, angry)
    DrawSphere((Vector3){pos.x - 0.10f, headY + 0.05f, faceZ + 0.04f}, 0.04f, eyeColor);
    DrawSphere((Vector3){pos.x + 0.10f, headY + 0.05f, faceZ + 0.04f}, 0.04f, eyeColor);

    // Angry eyebrows (angled downward toward center - furrowed)
    // Left eyebrow - angled down toward nose
    DrawCube((Vector3){pos.x - 0.12f, headY + 0.15f, faceZ}, 0.10f, 0.03f, 0.02f, browColor);
    DrawCube((Vector3){pos.x - 0.06f, headY + 0.12f, faceZ}, 0.06f, 0.03f, 0.02f, browColor);
    // Right eyebrow - angled down toward nose
    DrawCube((Vector3){pos.x + 0.12f, headY + 0.15f, faceZ}, 0.10f, 0.03f, 0.02f, browColor);
    DrawCube((Vector3){pos.x + 0.06f, headY + 0.12f, faceZ}, 0.06f, 0.03f, 0.02f, browColor);

    // Scowling mouth (downturned frown)
    DrawCube((Vector3){pos.x, headY - 0.12f, faceZ}, 0.14f, 0.03f, 0.02f, mouthColor);
    // Mouth corners turned down
    DrawCube((Vector3){pos.x - 0.08f, headY - 0.10f, faceZ}, 0.03f, 0.03f, 0.02f, mouthColor);
    DrawCube((Vector3){pos.x + 0.08f, headY - 0.10f, faceZ}, 0.03f, 0.03f, 0.02f, mouthColor);

    // Arms
    DrawCube((Vector3){pos.x - 0.45f, pos.y + 0.8f, pos.z}, 0.2f, 0.6f, 0.2f, trollDark);
    DrawCube((Vector3){pos.x + 0.45f, pos.y + 0.8f, pos.z}, 0.2f, 0.6f, 0.2f, trollDark);
    // Legs
    DrawCube((Vector3){pos.x - 0.15f, pos.y + 0.2f, pos.z}, 0.2f, 0.4f, 0.2f, trollDark);
    DrawCube((Vector3){pos.x + 0.15f, pos.y + 0.2f, pos.z}, 0.2f, 0.4f, 0.2f, trollDark);

    // Draw highlight outline when in attack range
    if (highlighted) {
        Color outlineColor = { 255, 255, 0, 255 }; // Yellow outline
        // Body outline
        DrawCubeWires((Vector3){pos.x, pos.y + 0.8f, pos.z}, 0.65f, 0.85f, 0.45f, outlineColor);
        // Head outline
        DrawSphereWires((Vector3){pos.x, pos.y + 1.5f, pos.z}, 0.38f, 8, 8, outlineColor);
        // Arms outline
        DrawCubeWires((Vector3){pos.x - 0.45f, pos.y + 0.8f, pos.z}, 0.25f, 0.65f, 0.25f, outlineColor);
        DrawCubeWires((Vector3){pos.x + 0.45f, pos.y + 0.8f, pos.z}, 0.25f, 0.65f, 0.25f, outlineColor);
        // Legs outline
        DrawCubeWires((Vector3){pos.x - 0.15f, pos.y + 0.2f, pos.z}, 0.25f, 0.45f, 0.25f, outlineColor);
        DrawCubeWires((Vector3){pos.x + 0.15f, pos.y + 0.2f, pos.z}, 0.25f, 0.45f, 0.25f, outlineColor);
    }
}

int main() {
    InitWindow(1, 1, "3D RuneScape-style Game");
    int monitorWidth = GetMonitorWidth(0);
    int monitorHeight = GetMonitorHeight(0);
    CloseWindow();

    int screenWidth = monitorWidth;
    int screenHeight = monitorHeight - 80;
    InitWindow(screenWidth, screenHeight, "3D RuneScape-style Game");
    SetWindowPosition(0, 25);

    // Initialize player state with defaults
    PlayerState playerState = {};
    playerState.posX = 0.0f;
    playerState.posY = 1.8f;
    playerState.posZ = 0.0f;
    playerState.targetX = 0.0f;
    playerState.targetY = 1.8f;
    playerState.targetZ = 1.0f;
    // Start with level 1 in all skills (0 XP), except Hitpoints starts at level 10
    for (int i = 0; i < SKILL_COUNT; i++) {
        playerState.skillXP[i] = 0;
    }
    playerState.skillXP[SKILL_HITPOINTS] = XP_TABLE[9]; // Level 10
    for (int i = 0; i < INV_SLOTS; i++) {
        playerState.inventory[i] = ITEM_NONE;
    }
    playerState.equippedWeapon = ITEM_NONE;
    playerState.swordPickedUp = false;
    // HP starts at max (level 10 = 10 HP)
    playerState.maxHP = 10;
    playerState.currentHP = 10;

    // Try to load saved game
    if (LoadGame(playerState)) {
        TraceLog(LOG_INFO, "Loaded save game");
    }

    Camera3D camera = {};
    camera.position = (Vector3){ playerState.posX, playerState.posY, playerState.posZ };
    camera.target = (Vector3){ playerState.targetX, playerState.targetY, playerState.targetZ };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Shader grassShader = LoadShader("shaders/grass.vs", "shaders/grass.fs");
    Mesh groundMesh = GenMeshPlane(100.0f, 100.0f, 10, 10);
    Model groundModel = LoadModelFromMesh(groundMesh);
    groundModel.materials[0].shader = grassShader;

    mkdir("screenshots", 0755);

    float screenshotMsgTimer = 0.0f;
    char screenshotMsg[128] = "";

    // Load map data
    MapData mapData = {};
    if (!LoadMap("maps/world.map", mapData)) {
        // Fallback defaults if map fails to load
        mapData.playerSpawn = { 0.0f, 1.8f, 0.0f };
        mapData.itemSpawns[0] = { 0.0f, 0.0f, 3.0f };
        mapData.itemTypes[0] = ITEM_BRONZE_SHORTSWORD;
        mapData.itemCount = 1;
        mapData.trollSpawns[0] = { 10.0f, 0.0f, 10.0f };
        mapData.trollCount = 1;
    }

    // Initialize world items from map
    WorldItem worldItems[MAX_WORLD_ITEMS] = {};
    int worldItemCount = mapData.itemCount;
    for (int i = 0; i < worldItemCount; i++) {
        worldItems[i].type = mapData.itemTypes[i];
        worldItems[i].position = mapData.itemSpawns[i];
        worldItems[i].pickedUp = false;
    }
    // Restore picked up state from save (for now just sword at index 0)
    if (worldItemCount > 0) {
        worldItems[0].pickedUp = playerState.swordPickedUp;
    }

    const float PICKUP_RANGE = 2.5f;
    bool showActionMenu = false;
    WorldItem* targetItem = nullptr;

    // Copy walls from map data
    Wall walls[MAX_WALLS] = {};
    int wallCount = mapData.wallCount;
    for (int i = 0; i < wallCount; i++) {
        walls[i] = mapData.walls[i];
    }

    // Initialize trolls from map spawn points
    Troll trolls[MAX_TROLLS] = {};
    int trollCount = mapData.trollCount;
    for (int i = 0; i < trollCount; i++) {
        trolls[i].spawnPoint = mapData.trollSpawns[i];
        trolls[i].position = mapData.trollSpawns[i];
        trolls[i].health = TROLL_MAX_HEALTH;
        trolls[i].maxHealth = TROLL_MAX_HEALTH;
        trolls[i].alive = true;
        trolls[i].respawnTimer = 0.0f;
        trolls[i].wanderTimer = 0.0f;
        trolls[i].wanderTarget = mapData.trollSpawns[i];
        trolls[i].hostile = false;
        trolls[i].attackCooldown = 0.0f;
    }

    // Damage indicators
    DamageIndicator damageIndicators[MAX_DAMAGE_INDICATORS] = {};

    // XP popups
    XPPopup xpPopups[MAX_XP_POPUPS] = {};

    // Level up notification
    LevelUpNotification levelUpNotif = {};

    // Attack cooldown
    float attackCooldown = 0.0f;

    // Weapon swing animation
    float swingTimer = 0.0f;
    const float SWING_DURATION = 0.2f;

    // Death state
    bool playerDead = false;
    float deathFadeTimer = 0.0f;
    const float DEATH_FADE_DURATION = 2.0f;
    Vector3 deathPosition = { 0, 0, 0 };

    // HP regeneration
    float hpRegenTimer = 0.0f;
    const float HP_REGEN_INTERVAL = 5.0f;

    bool mouseMode = false;
    DisableCursor();
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        if (IsKeyPressed(KEY_M)) {
            mouseMode = !mouseMode;
            if (mouseMode) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

        // Inventory click handling (mouse mode only)
        if (mouseMode && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            int invX = screenWidth - (INV_COLS * (SLOT_SIZE + SLOT_PADDING)) - 20;
            int invY = 60;

            for (int row = 0; row < INV_ROWS; row++) {
                for (int col = 0; col < INV_COLS; col++) {
                    int slotIdx = row * INV_COLS + col;
                    int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
                    int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);

                    // Check if mouse is over this slot
                    if (mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
                        mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE) {
                        ItemType clickedItem = playerState.inventory[slotIdx];
                        // If clicking a weapon, equip/unequip it
                        if (clickedItem == ITEM_BRONZE_SHORTSWORD) {
                            if (playerState.equippedWeapon == clickedItem) {
                                // Unequip
                                playerState.equippedWeapon = ITEM_NONE;
                            } else {
                                // Equip
                                playerState.equippedWeapon = clickedItem;
                            }
                        }
                    }
                }
            }
        }

        if (!mouseMode && !playerDead) {
            UpdateCamera(&camera, CAMERA_FIRST_PERSON);

            // Wall collision
            const float PLAYER_RADIUS = 0.3f;
            for (int i = 0; i < wallCount; i++) {
                if (PointInWall(camera.position, walls[i], PLAYER_RADIUS)) {
                    Vector3 oldTarget = camera.target;
                    Vector3 oldPos = camera.position;
                    camera.position = ResolveWallCollision(camera.position, walls[i], PLAYER_RADIUS);
                    // Maintain look direction
                    camera.target.x += camera.position.x - oldPos.x;
                    camera.target.z += camera.position.z - oldPos.z;
                }
            }
        }

        // Update attack cooldown
        if (attackCooldown > 0) {
            attackCooldown -= dt;
        }

        // Update swing animation
        if (swingTimer > 0) {
            swingTimer -= dt;
        }

        // Update trolls
        for (int i = 0; i < trollCount; i++) {
            // Update attack cooldown
            if (trolls[i].attackCooldown > 0) {
                trolls[i].attackCooldown -= dt;
            }

            if (trolls[i].alive) {
                if (trolls[i].hostile && !playerDead) {
                    // Chase player
                    float dx = camera.position.x - trolls[i].position.x;
                    float dz = camera.position.z - trolls[i].position.z;
                    float dist = sqrtf(dx*dx + dz*dz);

                    if (dist > TROLL_ATTACK_RANGE) {
                        // Move towards player
                        float speed = TROLL_CHASE_SPEED * dt;
                        trolls[i].position.x += (dx / dist) * speed;
                        trolls[i].position.z += (dz / dist) * speed;
                    } else if (trolls[i].attackCooldown <= 0) {
                        // Attack player!
                        int damage = GetRandomValue(0, TROLL_MAX_HIT);
                        playerState.currentHP -= damage;
                        SpawnDamageIndicator(damageIndicators, camera.position, damage);
                        trolls[i].attackCooldown = TROLL_ATTACK_COOLDOWN;

                        // Check for player death
                        if (playerState.currentHP <= 0) {
                            playerState.currentHP = 0;
                            playerDead = true;
                            deathFadeTimer = DEATH_FADE_DURATION;
                            deathPosition = camera.position;
                        }
                    }
                } else {
                    // Simple wandering behavior
                    trolls[i].wanderTimer -= dt;
                    if (trolls[i].wanderTimer <= 0) {
                        // Pick a new wander target near spawn point
                        trolls[i].wanderTarget.x = trolls[i].spawnPoint.x + RandomFloat(-3.0f, 3.0f);
                        trolls[i].wanderTarget.z = trolls[i].spawnPoint.z + RandomFloat(-3.0f, 3.0f);
                        trolls[i].wanderTimer = RandomFloat(2.0f, 5.0f);
                    }

                    // Move towards wander target slowly
                    float dx = trolls[i].wanderTarget.x - trolls[i].position.x;
                    float dz = trolls[i].wanderTarget.z - trolls[i].position.z;
                    float dist = sqrtf(dx*dx + dz*dz);
                    if (dist > 0.5f) {
                        float speed = 1.0f * dt;
                        trolls[i].position.x += (dx / dist) * speed;
                        trolls[i].position.z += (dz / dist) * speed;
                    }
                }
            } else {
                // Respawn timer
                trolls[i].respawnTimer -= dt;
                if (trolls[i].respawnTimer <= 0) {
                    // Respawn at random location near spawn point
                    trolls[i].position.x = trolls[i].spawnPoint.x + RandomFloat(-5.0f, 5.0f);
                    trolls[i].position.z = trolls[i].spawnPoint.z + RandomFloat(-5.0f, 5.0f);
                    trolls[i].position.y = 0.0f;
                    trolls[i].health = TROLL_MAX_HEALTH;
                    trolls[i].alive = true;
                    trolls[i].wanderTimer = 0.0f;
                    trolls[i].hostile = false;  // Reset hostility on respawn
                    trolls[i].attackCooldown = 0.0f;
                }
            }
        }

        // Attack with mouse click (when not in mouse mode, weapon equipped, and not dead)
        if (!mouseMode && !playerDead && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && attackCooldown <= 0 && playerState.equippedWeapon != ITEM_NONE) {
            // Trigger swing animation
            swingTimer = SWING_DURATION;
            attackCooldown = ATTACK_COOLDOWN;

            int combatLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
            int maxHit = CalculateMaxHit(combatLevel);

            // Find closest troll player is facing
            Troll* target = nullptr;
            float closestDist = ATTACK_RANGE + 1.0f;

            for (int i = 0; i < trollCount; i++) {
                if (trolls[i].alive) {
                    float dist = Distance3D(camera.position, trolls[i].position);
                    if (dist <= ATTACK_RANGE && dist < closestDist && IsFacing(camera, trolls[i].position)) {
                        target = &trolls[i];
                        closestDist = dist;
                    }
                }
            }

            if (target != nullptr) {
                // Make troll hostile when attacked
                target->hostile = true;

                int damage = RollDamage(maxHit);
                target->health -= damage;
                SpawnDamageIndicator(damageIndicators, target->position, damage);

                // Check if troll died
                if (target->health <= 0) {
                    target->alive = false;
                    target->respawnTimer = TROLL_RESPAWN_TIME;

                    // Award XP for kill (4 XP per hitpoint, like OSRS)
                    int xpGain = TROLL_MAX_HEALTH * 4;

                    // Check for level up
                    int oldLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
                    playerState.skillXP[SKILL_COMBAT] += xpGain;
                    int newLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
                    SpawnXPPopup(xpPopups, xpGain, SKILL_COMBAT);

                    // Level up!
                    if (newLevel > oldLevel) {
                        levelUpNotif.skillIndex = SKILL_COMBAT;
                        levelUpNotif.newLevel = newLevel;
                        levelUpNotif.timer = LEVEL_UP_DURATION;
                        levelUpNotif.active = true;
                    }
                }
            }
        }

        // Update damage indicators
        for (int i = 0; i < MAX_DAMAGE_INDICATORS; i++) {
            if (damageIndicators[i].active) {
                damageIndicators[i].timer -= dt;
                if (damageIndicators[i].timer <= 0) {
                    damageIndicators[i].active = false;
                }
            }
        }

        // Update XP popups
        for (int i = 0; i < MAX_XP_POPUPS; i++) {
            if (xpPopups[i].active) {
                xpPopups[i].timer -= dt;
                if (xpPopups[i].timer <= 0) {
                    xpPopups[i].active = false;
                }
            }
        }

        // Update level up notification
        if (levelUpNotif.active) {
            levelUpNotif.timer -= dt;
            if (levelUpNotif.timer <= 0) {
                levelUpNotif.active = false;
            }
        }

        // HP regeneration (only when alive and not at max HP)
        if (!playerDead && playerState.currentHP < playerState.maxHP) {
            hpRegenTimer += dt;
            if (hpRegenTimer >= HP_REGEN_INTERVAL) {
                playerState.currentHP++;
                hpRegenTimer = 0.0f;
            }
        } else {
            hpRegenTimer = 0.0f;
        }

        // Handle player death
        if (playerDead) {
            deathFadeTimer -= dt;

            // Drop all items at first frame of death
            if (deathFadeTimer > DEATH_FADE_DURATION - dt - 0.01f) {
                // Drop all inventory items on the ground
                for (int i = 0; i < INV_SLOTS; i++) {
                    if (playerState.inventory[i] != ITEM_NONE && worldItemCount < MAX_WORLD_ITEMS) {
                        // Add item to world at death position with slight random offset
                        worldItems[worldItemCount].type = playerState.inventory[i];
                        worldItems[worldItemCount].position = deathPosition;
                        worldItems[worldItemCount].position.x += RandomFloat(-1.0f, 1.0f);
                        worldItems[worldItemCount].position.z += RandomFloat(-1.0f, 1.0f);
                        worldItems[worldItemCount].position.y = 0.0f;
                        worldItems[worldItemCount].pickedUp = false;
                        worldItemCount++;

                        playerState.inventory[i] = ITEM_NONE;
                    }
                }
                // Unequip weapon since it was dropped
                playerState.equippedWeapon = ITEM_NONE;
            }

            // Respawn after fade completes
            if (deathFadeTimer <= 0) {
                playerDead = false;
                // Respawn at spawn point
                camera.position = mapData.playerSpawn;
                camera.target = (Vector3){ mapData.playerSpawn.x, mapData.playerSpawn.y, mapData.playerSpawn.z + 1.0f };
                // Reset HP to max
                playerState.maxHP = GetLevelFromXP(playerState.skillXP[SKILL_HITPOINTS]);
                playerState.currentHP = playerState.maxHP;
                // Reset all troll hostility
                for (int i = 0; i < trollCount; i++) {
                    trolls[i].hostile = false;
                }
            }
        }

        // Check proximity to world items
        showActionMenu = false;
        targetItem = nullptr;
        for (int i = 0; i < worldItemCount; i++) {
            if (!worldItems[i].pickedUp) {
                float dist = Distance3D(camera.position, worldItems[i].position);
                if (dist <= PICKUP_RANGE) {
                    showActionMenu = true;
                    targetItem = &worldItems[i];
                    break;  // Show menu for closest item we find
                }
            }
        }

        // Handle action menu input
        if (showActionMenu && targetItem != nullptr) {
            if (IsKeyPressed(KEY_ONE)) {
                for (int i = 0; i < INV_SLOTS; i++) {
                    if (playerState.inventory[i] == ITEM_NONE) {
                        playerState.inventory[i] = targetItem->type;
                        targetItem->pickedUp = true;
                        showActionMenu = false;
                        targetItem = nullptr;
                        break;
                    }
                }
            } else if (IsKeyPressed(KEY_TWO)) {
                snprintf(screenshotMsg, sizeof(screenshotMsg), "A bronze shortsword. Not very sharp.");
                screenshotMsgTimer = 3.0f;
            } else if (IsKeyPressed(KEY_THREE)) {
                showActionMenu = false;
            }
        }

        if (IsKeyPressed(KEY_P)) {
            time_t now = time(nullptr);
            char filename[64];
            strftime(filename, sizeof(filename), "screenshots/%Y%m%d_%H%M%S.png", localtime(&now));
            Image screenshot = LoadImageFromScreen();
            ExportImage(screenshot, filename);
            UnloadImage(screenshot);
            snprintf(screenshotMsg, sizeof(screenshotMsg), "Saved: %s", filename);
            screenshotMsgTimer = 2.0f;
        }

        if (screenshotMsgTimer > 0.0f) screenshotMsgTimer -= dt;

        BeginDrawing();
            ClearBackground(SKYBLUE);

            BeginMode3D(camera);
                DrawModel(groundModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                // Draw world items (all unpicked items)
                for (int i = 0; i < worldItemCount; i++) {
                    if (!worldItems[i].pickedUp) {
                        if (worldItems[i].type == ITEM_BRONZE_SHORTSWORD) {
                            Color bronzeBlade = { 205, 127, 50, 255 };
                            Color bronzeHandle = { 139, 90, 43, 255 };
                            DrawSword(worldItems[i].position, bronzeBlade, bronzeHandle);
                        }
                    }
                }

                // Draw trolls
                for (int i = 0; i < trollCount; i++) {
                    if (trolls[i].alive) {
                        float dist = Distance3D(camera.position, trolls[i].position);
                        bool inRange = (dist <= ATTACK_RANGE) && IsFacing(camera, trolls[i].position);
                        DrawTroll(trolls[i].position, inRange);
                    }
                }

                // Draw walls
                for (int i = 0; i < wallCount; i++) {
                    Vector3 pos = walls[i].position;
                    pos.y += walls[i].height / 2.0f; // Walls sit on ground
                    DrawCube(pos, walls[i].width, walls[i].height, walls[i].depth, walls[i].color);
                    DrawCubeWires(pos, walls[i].width, walls[i].height, walls[i].depth, DARKGRAY);
                }
            EndMode3D();

            // HUD
            DrawText("WASD to move, Mouse to look, M for mouse mode, LMB to attack", 10, 10, 20, WHITE);
            if (mouseMode) {
                DrawText("[MOUSE MODE]", 10, 35, 16, YELLOW);
            }
            DrawFPS(screenWidth - 100, 10);

            // Player HP bar (left of screen, below skills will go here)
            int hpBarX = 10;
            int hpBarY = screenHeight - 50;
            int hpBarW = 150;
            int hpBarH = 20;
            float hpRatio = (playerState.maxHP > 0) ? (float)playerState.currentHP / playerState.maxHP : 0.0f;
            DrawRectangle(hpBarX, hpBarY, hpBarW, hpBarH, DARKGRAY);
            DrawRectangle(hpBarX, hpBarY, (int)(hpBarW * hpRatio), hpBarH, RED);
            DrawRectangleLines(hpBarX, hpBarY, hpBarW, hpBarH, BLACK);
            char hpText[32];
            snprintf(hpText, sizeof(hpText), "HP: %d/%d", playerState.currentHP, playerState.maxHP);
            DrawText(hpText, hpBarX + 5, hpBarY + 3, 14, WHITE);

            // Attack cooldown indicator
            if (attackCooldown > 0) {
                int cdWidth = (int)(100 * (attackCooldown / ATTACK_COOLDOWN));
                DrawRectangle(screenWidth/2 - 50, screenHeight - 40, 100, 10, DARKGRAY);
                DrawRectangle(screenWidth/2 - 50, screenHeight - 40, cdWidth, 10, RED);
            }

            // Draw damage indicators (billboard text)
            for (int i = 0; i < MAX_DAMAGE_INDICATORS; i++) {
                if (damageIndicators[i].active) {
                    // Check if indicator is in front of camera
                    Vector3 toIndicator = {
                        damageIndicators[i].position.x - camera.position.x,
                        0,
                        damageIndicators[i].position.z - camera.position.z
                    };
                    Vector3 camForward = {
                        camera.target.x - camera.position.x,
                        0,
                        camera.target.z - camera.position.z
                    };
                    if (Dot3D(toIndicator, camForward) <= 0) continue; // Behind camera

                    Vector2 screenPos = GetWorldToScreen(damageIndicators[i].position, camera);
                    if (screenPos.x > 0 && screenPos.x < screenWidth &&
                        screenPos.y > 0 && screenPos.y < screenHeight) {
                        char dmgText[16];
                        snprintf(dmgText, sizeof(dmgText), "%d", damageIndicators[i].damage);
                        // Stay solid for 80% of duration, then fade quickly
                        float timeRatio = damageIndicators[i].timer / DAMAGE_INDICATOR_DURATION;
                        float alpha = (timeRatio > 0.2f) ? 1.0f : (timeRatio / 0.2f);
                        int fontSize = 48; // Much larger
                        int textWidth = MeasureText(dmgText, fontSize);
                        int tx = (int)screenPos.x - textWidth/2;
                        int ty = (int)screenPos.y - fontSize/2;

                        // Draw black outline for visibility
                        Color outlineColor = BLACK;
                        outlineColor.a = (unsigned char)(255 * alpha);
                        for (int ox = -2; ox <= 2; ox++) {
                            for (int oy = -2; oy <= 2; oy++) {
                                if (ox != 0 || oy != 0) {
                                    DrawText(dmgText, tx + ox, ty + oy, fontSize, outlineColor);
                                }
                            }
                        }

                        // Draw main text
                        Color dmgColor = (damageIndicators[i].damage == 0) ? BLUE : RED;
                        dmgColor.a = (unsigned char)(255 * alpha);
                        DrawText(dmgText, tx, ty, fontSize, dmgColor);
                    }
                }
            }

            // Draw troll health bars (world to screen)
            for (int i = 0; i < trollCount; i++) {
                if (trolls[i].alive) {
                    // Check if troll is in front of camera
                    Vector3 toTroll = {
                        trolls[i].position.x - camera.position.x,
                        0,
                        trolls[i].position.z - camera.position.z
                    };
                    Vector3 camForward = {
                        camera.target.x - camera.position.x,
                        0,
                        camera.target.z - camera.position.z
                    };
                    if (Dot3D(toTroll, camForward) <= 0) continue; // Behind camera

                    Vector3 healthBarPos = { trolls[i].position.x, trolls[i].position.y + 2.0f, trolls[i].position.z };
                    Vector2 screenPos = GetWorldToScreen(healthBarPos, camera);
                    if (screenPos.x > 0 && screenPos.x < screenWidth &&
                        screenPos.y > 0 && screenPos.y < screenHeight) {
                        int barWidth = 40;
                        int barHeight = 6;
                        int healthWidth = (int)(barWidth * trolls[i].health / (float)trolls[i].maxHealth);
                        DrawRectangle((int)screenPos.x - barWidth/2, (int)screenPos.y, barWidth, barHeight, DARKGRAY);
                        DrawRectangle((int)screenPos.x - barWidth/2, (int)screenPos.y, healthWidth, barHeight, GREEN);
                        DrawRectangleLines((int)screenPos.x - barWidth/2, (int)screenPos.y, barWidth, barHeight, BLACK);
                    }
                }
            }

            // Crosshair (when not in mouse mode)
            if (!mouseMode) {
                int cx = screenWidth / 2;
                int cy = screenHeight / 2;
                DrawLine(cx - 10, cy, cx + 10, cy, WHITE);
                DrawLine(cx, cy - 10, cx, cy + 10, WHITE);
            }

            // FPS weapon view (bottom right)
            if (playerState.equippedWeapon == ITEM_BRONZE_SHORTSWORD) {
                // Base position for weapon
                float weaponBaseX = screenWidth - 150.0f;
                float weaponBaseY = screenHeight - 100.0f;

                // Swing animation - rotate and move weapon
                float swingAngle = 0.0f;
                float swingOffsetX = 0.0f;
                float swingOffsetY = 0.0f;
                if (swingTimer > 0) {
                    float swingProgress = swingTimer / SWING_DURATION;
                    // Swing arc from right to left
                    swingAngle = sinf(swingProgress * PI) * 60.0f; // degrees
                    swingOffsetX = -sinf(swingProgress * PI) * 80.0f;
                    swingOffsetY = -sinf(swingProgress * PI) * 40.0f;
                }

                float wpnX = weaponBaseX + swingOffsetX;
                float wpnY = weaponBaseY + swingOffsetY;

                // Draw sword (2D representation with rotation)
                Color bronzeBlade = { 205, 127, 50, 255 };
                Color bronzeHandle = { 139, 90, 43, 255 };

                // Calculate rotated sword vertices
                float radAngle = swingAngle * DEG2RAD;
                float cosA = cosf(radAngle);
                float sinA = sinf(radAngle);

                // Blade dimensions
                float bladeLen = 120.0f;
                float bladeWidth = 12.0f;

                // Blade points (relative to grip)
                Vector2 bladeTip = { wpnX + (-bladeLen * sinA), wpnY + (-bladeLen * cosA) };
                Vector2 bladeBase = { wpnX, wpnY };

                // Draw blade as thick line
                DrawLineEx(bladeBase, bladeTip, bladeWidth, bronzeBlade);
                // Blade outline
                DrawLineEx(bladeBase, bladeTip, bladeWidth + 2, DARKGRAY);
                DrawLineEx(bladeBase, bladeTip, bladeWidth, bronzeBlade);

                // Handle
                Vector2 handleEnd = { wpnX + (30.0f * sinA), wpnY + (30.0f * cosA) };
                DrawLineEx(bladeBase, handleEnd, 10.0f, bronzeHandle);

                // Crossguard
                Vector2 guardLeft = { wpnX + (-15.0f * cosA), wpnY + (15.0f * sinA) };
                Vector2 guardRight = { wpnX + (15.0f * cosA), wpnY + (-15.0f * sinA) };
                DrawLineEx(guardLeft, guardRight, 6.0f, bronzeHandle);
            }

            // MW2-style XP popups (center of screen, stacked)
            int xpPopupY = screenHeight / 3;
            for (int i = 0; i < MAX_XP_POPUPS; i++) {
                if (xpPopups[i].active) {
                    // Stay solid for 80% of duration, then fade quickly
                    float timeRatio = xpPopups[i].timer / XP_POPUP_DURATION;
                    float alpha = (timeRatio > 0.2f) ? 1.0f : (timeRatio / 0.2f);
                    int skillIdx = xpPopups[i].skillIndex;
                    int currentXP = playerState.skillXP[skillIdx];
                    int currentLevel = GetLevelFromXP(currentXP);
                    int xpForCurrent = XP_TABLE[currentLevel - 1];
                    int xpForNext = (currentLevel < 99) ? XP_TABLE[currentLevel] : XP_TABLE[98];
                    int xpIntoLevel = currentXP - xpForCurrent;
                    int xpNeeded = xpForNext - xpForCurrent;
                    float progress = (xpNeeded > 0) ? (float)xpIntoLevel / xpNeeded : 1.0f;

                    // Draw "+XP SKILLNAME" text (centered)
                    char xpText[64];
                    snprintf(xpText, sizeof(xpText), "+%d %s", xpPopups[i].xpAmount, SKILL_NAMES[skillIdx]);
                    int textWidth = MeasureText(xpText, 28);
                    int popupX = (screenWidth - textWidth) / 2;

                    // Black outline
                    Color outlineColor = { 0, 0, 0, (unsigned char)(200 * alpha) };
                    for (int ox = -2; ox <= 2; ox++) {
                        for (int oy = -2; oy <= 2; oy++) {
                            if (ox != 0 || oy != 0) {
                                DrawText(xpText, popupX + ox, xpPopupY + oy, 28, outlineColor);
                            }
                        }
                    }

                    // Yellow text
                    Color xpColor = { 255, 215, 0, (unsigned char)(255 * alpha) };
                    DrawText(xpText, popupX, xpPopupY, 28, xpColor);

                    // Level progress bar below (centered)
                    int barWidth = 200;
                    int barX = (screenWidth - barWidth) / 2;
                    int barY = xpPopupY + 32;
                    int barHeight = 8;
                    Color barBg = { 40, 40, 40, (unsigned char)(180 * alpha) };
                    Color barFg = { 50, 205, 50, (unsigned char)(255 * alpha) };
                    Color barBorder = { 100, 100, 100, (unsigned char)(200 * alpha) };

                    DrawRectangle(barX, barY, barWidth, barHeight, barBg);
                    DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, barFg);
                    DrawRectangleLines(barX, barY, barWidth, barHeight, barBorder);

                    // Level text (right of bar)
                    char levelText[32];
                    snprintf(levelText, sizeof(levelText), "Lv %d", currentLevel);
                    Color levelColor = { 255, 255, 255, (unsigned char)(255 * alpha) };
                    DrawText(levelText, barX + barWidth + 10, barY - 2, 14, levelColor);

                    xpPopupY += 55;
                }
            }

            // Skills display (top-left, below controls)
            int skillY = 60;
            DrawText("Skills:", 10, skillY, 18, GOLD);
            skillY += 22;
            for (int i = 0; i < SKILL_COUNT; i++) {
                int level = GetLevelFromXP(playerState.skillXP[i]);
                int xpForCurrent = XP_TABLE[level - 1];
                int xpForNext = (level < 99) ? XP_TABLE[level] : XP_TABLE[98];
                char skillText[64];
                snprintf(skillText, sizeof(skillText), "%s: %d (%d/%d)",
                    SKILL_NAMES[i], level, playerState.skillXP[i], xpForNext);
                DrawText(skillText, 10, skillY, 14, WHITE);
                skillY += 18;
            }

            // Action menu overlay
            if (showActionMenu) {
                int menuX = screenWidth / 2 - 100;
                int menuY = screenHeight / 2 - 60;
                int menuW = 200;
                int menuH = 120;

                DrawRectangle(menuX, menuY, menuW, menuH, (Color){0, 0, 0, 180});
                DrawRectangleLines(menuX, menuY, menuW, menuH, GOLD);

                DrawText("Bronze Shortsword", menuX + 10, menuY + 10, 18, GOLD);
                DrawText("1. Pickup", menuX + 10, menuY + 40, 16, WHITE);
                DrawText("2. Examine", menuX + 10, menuY + 60, 16, WHITE);
                DrawText("3. Cancel", menuX + 10, menuY + 80, 16, GRAY);
            }

            // Inventory UI (right side)
            int invX = screenWidth - (INV_COLS * (SLOT_SIZE + SLOT_PADDING)) - 20;
            int invY = 60;

            int invW = INV_COLS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING;
            int invH = INV_ROWS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING + 25;
            DrawRectangle(invX - SLOT_PADDING, invY - 25, invW, invH, (Color){62, 53, 41, 220});
            DrawRectangleLines(invX - SLOT_PADDING, invY - 25, invW, invH, (Color){86, 74, 57, 255});
            DrawText("Inventory", invX, invY - 22, 16, (Color){255, 204, 0, 255});

            for (int row = 0; row < INV_ROWS; row++) {
                for (int col = 0; col < INV_COLS; col++) {
                    int slotIdx = row * INV_COLS + col;
                    int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
                    int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);

                    DrawRectangle(slotX, slotY, SLOT_SIZE, SLOT_SIZE, (Color){40, 35, 28, 255});

                    // Highlight equipped item
                    bool isEquipped = (playerState.inventory[slotIdx] != ITEM_NONE &&
                                       playerState.inventory[slotIdx] == playerState.equippedWeapon);
                    if (isEquipped) {
                        DrawRectangleLines(slotX, slotY, SLOT_SIZE, SLOT_SIZE, (Color){255, 215, 0, 255}); // Gold border
                        DrawRectangleLines(slotX+1, slotY+1, SLOT_SIZE-2, SLOT_SIZE-2, (Color){255, 215, 0, 255});
                    } else {
                        DrawRectangleLines(slotX, slotY, SLOT_SIZE, SLOT_SIZE, (Color){86, 74, 57, 255});
                    }

                    if (playerState.inventory[slotIdx] == ITEM_BRONZE_SHORTSWORD) {
                        Color bronzeColor = { 205, 127, 50, 255 };
                        int cx = slotX + SLOT_SIZE / 2;
                        int cy = slotY + SLOT_SIZE / 2;
                        DrawRectangle(cx - 2, cy - 14, 4, 24, bronzeColor);
                        DrawRectangle(cx - 2, cy + 10, 4, 8, BROWN);
                        DrawRectangle(cx - 8, cy + 8, 16, 3, BROWN);
                    }
                }
            }

            if (screenshotMsgTimer > 0.0f) {
                DrawText(screenshotMsg, 10, screenHeight - 80, 20, YELLOW);
            }

            // Death blackout overlay
            if (playerDead) {
                // Fade from transparent to black, then stay black
                float fadeProgress = 1.0f - (deathFadeTimer / DEATH_FADE_DURATION);
                unsigned char alpha = (unsigned char)(255 * fadeProgress);
                if (fadeProgress > 0.5f) alpha = 255; // Full black for second half
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, alpha });

                // "You died" text appears after initial fade
                if (fadeProgress > 0.3f) {
                    const char* deathText = "You died!";
                    int textWidth = MeasureText(deathText, 48);
                    unsigned char textAlpha = (unsigned char)(255 * ((fadeProgress - 0.3f) / 0.7f));
                    DrawText(deathText, (screenWidth - textWidth) / 2, screenHeight / 2 - 24, 48, (Color){ 200, 0, 0, textAlpha });
                }
            }

            // Level up parchment banner
            if (levelUpNotif.active) {
                // Parchment colors
                Color parchmentBg = { 222, 198, 158, 240 };     // Tan/beige parchment
                Color parchmentBorder = { 139, 90, 43, 255 };   // Brown border
                Color parchmentDark = { 180, 150, 100, 255 };   // Darker parchment for texture
                Color textColor = { 60, 40, 20, 255 };          // Dark brown text

                // Banner dimensions
                int bannerW = 400;
                int bannerH = 100;
                int bannerX = (screenWidth - bannerW) / 2;
                int bannerY = screenHeight - bannerH - 20;

                // Fade in/out
                float alpha = 1.0f;
                if (levelUpNotif.timer > LEVEL_UP_DURATION - 0.3f) {
                    alpha = (LEVEL_UP_DURATION - levelUpNotif.timer) / 0.3f;
                } else if (levelUpNotif.timer < 0.5f) {
                    alpha = levelUpNotif.timer / 0.5f;
                }

                // Apply alpha to colors
                parchmentBg.a = (unsigned char)(240 * alpha);
                parchmentBorder.a = (unsigned char)(255 * alpha);
                parchmentDark.a = (unsigned char)(255 * alpha);
                textColor.a = (unsigned char)(255 * alpha);

                // Draw parchment background with torn edge effect
                DrawRectangle(bannerX, bannerY, bannerW, bannerH, parchmentBg);

                // Add some texture lines
                for (int i = 0; i < 5; i++) {
                    int lineY = bannerY + 15 + i * 18;
                    DrawLine(bannerX + 10, lineY, bannerX + bannerW - 10, lineY, parchmentDark);
                }

                // Border (double line for scroll effect)
                DrawRectangleLinesEx((Rectangle){(float)bannerX, (float)bannerY, (float)bannerW, (float)bannerH}, 3, parchmentBorder);
                DrawRectangleLinesEx((Rectangle){(float)bannerX + 5, (float)bannerY + 5, (float)bannerW - 10, (float)bannerH - 10}, 1, parchmentBorder);

                // Decorative corners
                int cornerSize = 12;
                DrawRectangle(bannerX, bannerY, cornerSize, cornerSize, parchmentBorder);
                DrawRectangle(bannerX + bannerW - cornerSize, bannerY, cornerSize, cornerSize, parchmentBorder);
                DrawRectangle(bannerX, bannerY + bannerH - cornerSize, cornerSize, cornerSize, parchmentBorder);
                DrawRectangle(bannerX + bannerW - cornerSize, bannerY + bannerH - cornerSize, cornerSize, cornerSize, parchmentBorder);

                // Text
                const char* skillName = SKILL_NAMES[levelUpNotif.skillIndex];
                char titleText[64];
                snprintf(titleText, sizeof(titleText), "Congratulations!");
                char levelText[64];
                snprintf(levelText, sizeof(levelText), "You've advanced a %s level!", skillName);
                char newLevelText[64];
                snprintf(newLevelText, sizeof(newLevelText), "You are now level %d.", levelUpNotif.newLevel);

                int titleWidth = MeasureText(titleText, 24);
                int levelWidth = MeasureText(levelText, 20);
                int newLevelWidth = MeasureText(newLevelText, 18);

                DrawText(titleText, bannerX + (bannerW - titleWidth) / 2, bannerY + 15, 24, textColor);
                DrawText(levelText, bannerX + (bannerW - levelWidth) / 2, bannerY + 45, 20, textColor);
                DrawText(newLevelText, bannerX + (bannerW - newLevelWidth) / 2, bannerY + 70, 18, textColor);
            }
        EndDrawing();
    }

    // Save game state before quitting
    playerState.posX = camera.position.x;
    playerState.posY = camera.position.y;
    playerState.posZ = camera.position.z;
    playerState.targetX = camera.target.x;
    playerState.targetY = camera.target.y;
    playerState.targetZ = camera.target.z;
    playerState.swordPickedUp = (worldItemCount > 0) ? worldItems[0].pickedUp : false;
    SaveGame(playerState);
    TraceLog(LOG_INFO, "Game saved to %s", SAVE_FILE);

    UnloadModel(groundModel);
    UnloadShader(grassShader);
    CloseWindow();
    return 0;
}
