#include "save_system.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

const char* SAVE_FILE = "savegame.json";

// Helper to find a number after a key in JSON
static int ParseIntAfter(const char* json, const char* key, int defaultVal) {
    const char* pos = strstr(json, key);
    if (!pos) return defaultVal;
    pos = strchr(pos, ':');
    if (!pos) return defaultVal;
    return atoi(pos + 1);
}

static float ParseFloatAfter(const char* json, const char* key, float defaultVal) {
    const char* pos = strstr(json, key);
    if (!pos) return defaultVal;
    pos = strchr(pos, ':');
    if (!pos) return defaultVal;
    return (float)atof(pos + 1);
}

static bool ParseBoolAfter(const char* json, const char* key, bool defaultVal) {
    const char* pos = strstr(json, key);
    if (!pos) return defaultVal;
    pos = strchr(pos, ':');
    if (!pos) return defaultVal;
    while (*pos && (*pos == ':' || *pos == ' ')) pos++;
    return strncmp(pos, "true", 4) == 0;
}

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
