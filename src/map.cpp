#include "map.h"
#include "raylib.h"
#include <cstdio>
#include <cstring>

bool LoadMap(const char* filename, MapData& map) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        TraceLog(LOG_ERROR, "Failed to load map: %s", filename);
        return false;
    }

    map.playerSpawn = { 0.0f, 1.8f, 0.0f };
    map.itemCount = 0;
    map.enemyCount = 0;
    map.wallCount = 0;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
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
                    } else if (strcmp(itemName, "cow_hide") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_COW_HIDE;
                    } else if (strcmp(itemName, "bones") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_BONES;
                    } else if (strcmp(itemName, "gil") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_GIL;
                    } else {
                        map.itemTypes[map.itemCount] = ITEM_NONE;
                    }
                    map.itemSpawns[map.itemCount] = { x, y, z };
                    map.itemCount++;
                }
            }
        }
        else if (strcmp(type, "troll") == 0) {
            // Legacy support for "troll" keyword
            if (map.enemyCount < MAX_ENEMIES) {
                float x, y, z;
                if (sscanf(line, "%*s %f %f %f", &x, &y, &z) == 3) {
                    map.enemySpawns[map.enemyCount] = { x, y, z };
                    map.enemyTypes[map.enemyCount] = ENEMY_TROLL;
                    map.enemyCount++;
                }
            }
        }
        else if (strcmp(type, "enemy") == 0) {
            if (map.enemyCount < MAX_ENEMIES) {
                char enemyName[64];
                float x, y, z;
                if (sscanf(line, "%*s %63s %f %f %f", enemyName, &x, &y, &z) == 4) {
                    EnemyType enemyType = ENEMY_TROLL; // default
                    if (strcmp(enemyName, "troll") == 0) {
                        enemyType = ENEMY_TROLL;
                    } else if (strcmp(enemyName, "cow") == 0) {
                        enemyType = ENEMY_COW;
                    }
                    map.enemySpawns[map.enemyCount] = { x, y, z };
                    map.enemyTypes[map.enemyCount] = enemyType;
                    map.enemyCount++;
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
    TraceLog(LOG_INFO, "Loaded map: %s (%d items, %d enemies, %d walls)", filename, map.itemCount, map.enemyCount, map.wallCount);
    return true;
}
