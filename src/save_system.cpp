#include "save_system.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <string>

const char* SAVE_FILE = "savegame.json";
const char* BACKUP_DIR = "save_backups";
const int MAX_BACKUPS = 5;

// Create backup of current save file before overwriting
static void BackupSaveFile() {
    // Check if save file exists
    FILE* f = fopen(SAVE_FILE, "r");
    if (!f) return;  // No save to backup
    fclose(f);

    // Create backup directory if it doesn't exist
    mkdir(BACKUP_DIR, 0755);

    // Generate timestamp for backup filename
    time_t now = time(nullptr);
    char backupName[128];
    strftime(backupName, sizeof(backupName), "save_backups/savegame_%Y%m%d_%H%M%S.json", localtime(&now));

    // Copy current save to backup
    FILE* src = fopen(SAVE_FILE, "r");
    FILE* dst = fopen(backupName, "w");
    if (src && dst) {
        char buf[1024];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
            fwrite(buf, 1, n, dst);
        }
    }
    if (src) fclose(src);
    if (dst) fclose(dst);

    // Clean up old backups, keep only MAX_BACKUPS most recent
    std::vector<std::string> backups;
    DIR* dir = opendir(BACKUP_DIR);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strstr(entry->d_name, "savegame_") && strstr(entry->d_name, ".json")) {
                backups.push_back(std::string(BACKUP_DIR) + "/" + entry->d_name);
            }
        }
        closedir(dir);
    }

    // Sort by name (timestamp in name means alphabetical = chronological)
    std::sort(backups.begin(), backups.end());

    // Remove oldest backups if we have too many
    while (backups.size() > MAX_BACKUPS) {
        remove(backups[0].c_str());
        backups.erase(backups.begin());
    }
}

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
    // Backup existing save before overwriting
    BackupSaveFile();

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
    fprintf(f, "  \"inventoryCount\": [");
    for (int i = 0; i < INV_SLOTS; i++) {
        fprintf(f, "%d%s", state.inventoryCount[i], i < INV_SLOTS - 1 ? ", " : "");
    }
    fprintf(f, "],\n");
    fprintf(f, "  \"equippedWeapon\": %d,\n", state.equippedWeapon);
    fprintf(f, "  \"swordPickedUp\": %s,\n", state.swordPickedUp ? "true" : "false");
    fprintf(f, "  \"currentHP\": %d,\n", state.currentHP);
    fprintf(f, "  \"maxHP\": %d,\n", state.maxHP);
    fprintf(f, "  \"timeOfDay\": %.6f\n", state.timeOfDay);
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

    // Parse inventory counts
    const char* invCountSection = strstr(json, "\"inventoryCount\"");
    if (invCountSection) {
        const char* bracket = strchr(invCountSection, '[');
        if (bracket) {
            const char* p = bracket + 1;
            for (int i = 0; i < INV_SLOTS && *p; i++) {
                while (*p && (*p == ' ' || *p == ',')) p++;
                if (*p == ']') break;
                state.inventoryCount[i] = atoi(p);
                while (*p && *p != ',' && *p != ']') p++;
            }
        }
    } else {
        // Default: set count to 1 for any existing items (backwards compatibility)
        for (int i = 0; i < INV_SLOTS; i++) {
            state.inventoryCount[i] = (state.inventory[i] != ITEM_NONE) ? 1 : 0;
        }
    }

    state.equippedWeapon = (ItemType)ParseIntAfter(json, "\"equippedWeapon\"", ITEM_NONE);
    state.swordPickedUp = ParseBoolAfter(json, "\"swordPickedUp\"", false);
    state.currentHP = ParseIntAfter(json, "\"currentHP\"", 10);
    state.maxHP = ParseIntAfter(json, "\"maxHP\"", 10);
    state.timeOfDay = ParseFloatAfter(json, "\"timeOfDay\"", 0.5f);  // Default to midday

    delete[] json;
    return true;
}
