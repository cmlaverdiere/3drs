#include "map.h"
#include "raylib.h"
#include <cstdio>
#include <cstring>
#include <libgen.h>

// Forward declaration
static bool LoadMapFile(const char* filename, MapData& map, float offsetX, float offsetZ, const char* baseDir);

// Get directory from a file path
static void GetDirectory(const char* filepath, char* dir, size_t dirSize) {
    strncpy(dir, filepath, dirSize - 1);
    dir[dirSize - 1] = '\0';
    char* lastSlash = strrchr(dir, '/');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
    } else {
        dir[0] = '\0';
    }
}

// Load a single map file with offset applied to all coordinates
static bool LoadMapFile(const char* filename, MapData& map, float offsetX, float offsetZ, const char* baseDir) {
    // Build full path if baseDir is provided
    char fullPath[512];
    if (baseDir && baseDir[0] != '\0') {
        snprintf(fullPath, sizeof(fullPath), "%s%s", baseDir, filename);
    } else {
        strncpy(fullPath, filename, sizeof(fullPath) - 1);
        fullPath[sizeof(fullPath) - 1] = '\0';
    }

    FILE* f = fopen(fullPath, "r");
    if (!f) {
        TraceLog(LOG_ERROR, "Failed to load map file: %s", fullPath);
        return false;
    }

    TraceLog(LOG_INFO, "Loading map file: %s (offset: %.1f, %.1f)", fullPath, offsetX, offsetZ);

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        char type[64];
        if (sscanf(line, "%63s", type) != 1) continue;

        if (strcmp(type, "include") == 0) {
            // Format: include filename.map offsetX offsetZ
            char includeFile[128];
            float incOffsetX = 0.0f, incOffsetZ = 0.0f;
            if (sscanf(line, "%*s %127s %f %f", includeFile, &incOffsetX, &incOffsetZ) >= 1) {
                // Get directory of current file for relative includes
                char currentDir[512];
                GetDirectory(fullPath, currentDir, sizeof(currentDir));

                // Recursively load included file with combined offset
                LoadMapFile(includeFile, map, offsetX + incOffsetX, offsetZ + incOffsetZ, currentDir);
            }
        }
        else if (strcmp(type, "player_spawn") == 0) {
            float x, y, z;
            if (sscanf(line, "%*s %f %f %f", &x, &y, &z) == 3) {
                map.playerSpawn = { x + offsetX, y, z + offsetZ };
            }
        }
        else if (strcmp(type, "item") == 0) {
            if (map.itemCount < MAX_WORLD_ITEMS) {
                char itemName[64];
                float x, y, z;
                if (sscanf(line, "%*s %63s %f %f %f", itemName, &x, &y, &z) == 4) {
                    if (strcmp(itemName, "bronze_shortsword") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_BRONZE_SHORTSWORD;
                    } else if (strcmp(itemName, "bronze_axe") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_BRONZE_AXE;
                    } else if (strcmp(itemName, "cow_hide") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_COW_HIDE;
                    } else if (strcmp(itemName, "bones") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_BONES;
                    } else if (strcmp(itemName, "gil") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_GIL;
                    } else if (strcmp(itemName, "logs") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_LOGS;
                    } else if (strcmp(itemName, "chitin") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_CHITIN;
                    } else if (strcmp(itemName, "iron_2h_sword") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_IRON_2H_SWORD;
                    } else if (strcmp(itemName, "bronze_pickaxe") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_BRONZE_PICKAXE;
                    } else if (strcmp(itemName, "copper_ore") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_COPPER_ORE;
                    } else if (strcmp(itemName, "tin_ore") == 0) {
                        map.itemTypes[map.itemCount] = ITEM_TIN_ORE;
                    } else {
                        map.itemTypes[map.itemCount] = ITEM_NONE;
                    }
                    map.itemSpawns[map.itemCount] = { x + offsetX, y, z + offsetZ };
                    map.itemCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_WORLD_ITEMS (%d) exceeded, skipping item", MAX_WORLD_ITEMS);
            }
        }
        else if (strcmp(type, "troll") == 0) {
            // Legacy support for "troll" keyword
            if (map.enemyCount < MAX_ENEMIES) {
                float x, y, z;
                if (sscanf(line, "%*s %f %f %f", &x, &y, &z) == 3) {
                    map.enemySpawns[map.enemyCount] = { x + offsetX, y, z + offsetZ };
                    map.enemyTypes[map.enemyCount] = ENEMY_TROLL;
                    map.enemyCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_ENEMIES (%d) exceeded, skipping enemy", MAX_ENEMIES);
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
                    } else if (strcmp(enemyName, "scorpion") == 0) {
                        enemyType = ENEMY_SCORPION;
                    } else if (strcmp(enemyName, "bandit") == 0) {
                        enemyType = ENEMY_BANDIT;
                    } else if (strcmp(enemyName, "sand_golem") == 0) {
                        enemyType = ENEMY_SAND_GOLEM;
                    }
                    map.enemySpawns[map.enemyCount] = { x + offsetX, y, z + offsetZ };
                    map.enemyTypes[map.enemyCount] = enemyType;
                    map.enemyCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_ENEMIES (%d) exceeded, skipping enemy", MAX_ENEMIES);
            }
        }
        else if (strcmp(type, "wall") == 0) {
            if (map.wallCount < MAX_WALLS) {
                float x, y, z, w, h, d;
                char materialName[64];
                if (sscanf(line, "%*s %f %f %f %f %f %f %63s", &x, &y, &z, &w, &h, &d, materialName) == 7) {
                    map.walls[map.wallCount].position = { x + offsetX, y, z + offsetZ };
                    map.walls[map.wallCount].width = w;
                    map.walls[map.wallCount].height = h;
                    map.walls[map.wallCount].depth = d;

                    // Parse material type
                    if (strcmp(materialName, "wood") == 0) {
                        map.walls[map.wallCount].material = WALL_WOOD;
                    } else if (strcmp(materialName, "stone") == 0) {
                        map.walls[map.wallCount].material = WALL_STONE;
                    } else if (strcmp(materialName, "brick") == 0) {
                        map.walls[map.wallCount].material = WALL_BRICK;
                    } else {
                        map.walls[map.wallCount].material = WALL_STONE; // default
                    }
                    map.wallCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_WALLS (%d) exceeded, skipping wall", MAX_WALLS);
            }
        }
        else if (strcmp(type, "tree") == 0) {
            if (map.treeCount < MAX_TREES) {
                float x, y, z;
                if (sscanf(line, "%*s %f %f %f", &x, &y, &z) == 3) {
                    map.treeSpawns[map.treeCount] = { x + offsetX, y, z + offsetZ };
                    map.treeTypes[map.treeCount] = TREE_NORMAL;
                    map.treeCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_TREES (%d) exceeded, skipping tree", MAX_TREES);
            }
        }
        else if (strcmp(type, "oak_tree") == 0) {
            if (map.treeCount < MAX_TREES) {
                float x, y, z;
                if (sscanf(line, "%*s %f %f %f", &x, &y, &z) == 3) {
                    map.treeSpawns[map.treeCount] = { x + offsetX, y, z + offsetZ };
                    map.treeTypes[map.treeCount] = TREE_OAK;
                    map.treeCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_TREES (%d) exceeded, skipping oak tree", MAX_TREES);
            }
        }
        else if (strcmp(type, "rock") == 0) {
            // Format: rock <type> x y z
            if (map.rockCount < MAX_ROCKS) {
                char rockName[64];
                float x, y, z;
                if (sscanf(line, "%*s %63s %f %f %f", rockName, &x, &y, &z) == 4) {
                    RockType rockType = ROCK_COPPER; // default
                    if (strcmp(rockName, "copper") == 0) {
                        rockType = ROCK_COPPER;
                    } else if (strcmp(rockName, "tin") == 0) {
                        rockType = ROCK_TIN;
                    }
                    map.rockSpawns[map.rockCount] = { x + offsetX, y, z + offsetZ };
                    map.rockTypes[map.rockCount] = rockType;
                    map.rockCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_ROCKS (%d) exceeded, skipping rock", MAX_ROCKS);
            }
        }
        else if (strcmp(type, "water") == 0) {
            // Format: water x y z width length
            if (map.waterCount < MAX_WATER) {
                float x, y, z, w, l;
                if (sscanf(line, "%*s %f %f %f %f %f", &x, &y, &z, &w, &l) == 5) {
                    map.waterBodies[map.waterCount].position = { x + offsetX, y, z + offsetZ };
                    map.waterBodies[map.waterCount].width = w;
                    map.waterBodies[map.waterCount].length = l;
                    map.waterCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_WATER (%d) exceeded, skipping water", MAX_WATER);
            }
        }
        else if (strcmp(type, "sand") == 0) {
            // Format: sand x y z width length
            if (map.sandCount < MAX_SAND) {
                float x, y, z, w, l;
                if (sscanf(line, "%*s %f %f %f %f %f", &x, &y, &z, &w, &l) == 5) {
                    map.sandZones[map.sandCount].position = { x + offsetX, y, z + offsetZ };
                    map.sandZones[map.sandCount].width = w;
                    map.sandZones[map.sandCount].length = l;
                    map.sandCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_SAND (%d) exceeded, skipping sand", MAX_SAND);
            }
        }
        else if (strcmp(type, "valley") == 0) {
            // Format: valley axis position width depth minExtent maxExtent
            // axis: x (north-south along Z) or z (east-west along X)
            // minExtent/maxExtent: range along the perpendicular axis
            if (map.valleyCount < MAX_VALLEYS) {
                char axisName[16];
                float pos, width, depth, minExt, maxExt;
                if (sscanf(line, "%*s %15s %f %f %f %f %f", axisName, &pos, &width, &depth, &minExt, &maxExt) == 6) {
                    bool isXAxis = (strcmp(axisName, "x") == 0);
                    // Apply offset based on axis
                    float adjustedPos = pos + (isXAxis ? offsetX : offsetZ);
                    // Apply offset to extent range (perpendicular axis)
                    float adjustedMin = minExt + (isXAxis ? offsetZ : offsetX);
                    float adjustedMax = maxExt + (isXAxis ? offsetZ : offsetX);

                    map.valleys[map.valleyCount].position = adjustedPos;
                    map.valleys[map.valleyCount].width = width;
                    map.valleys[map.valleyCount].depth = depth;
                    map.valleys[map.valleyCount].axis = isXAxis ? 0 : 1;
                    map.valleys[map.valleyCount].minExtent = adjustedMin;
                    map.valleys[map.valleyCount].maxExtent = adjustedMax;
                    map.valleyCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_VALLEYS (%d) exceeded, skipping valley", MAX_VALLEYS);
            }
        }
        else if (strcmp(type, "npc") == 0) {
            // Format: npc <type> x y z
            if (map.npcCount < MAX_NPCS) {
                char npcName[64];
                float x, y, z;
                if (sscanf(line, "%*s %63s %f %f %f", npcName, &x, &y, &z) == 4) {
                    NPCType npcType = NPC_HANS; // default
                    if (strcmp(npcName, "hans") == 0) {
                        npcType = NPC_HANS;
                    } else if (strcmp(npcName, "shopkeeper") == 0) {
                        npcType = NPC_SHOPKEEPER;
                    } else if (strcmp(npcName, "guard") == 0) {
                        npcType = NPC_GUARD;
                    } else if (strcmp(npcName, "cook") == 0) {
                        npcType = NPC_COOK;
                    } else if (strcmp(npcName, "varrock_trader") == 0) {
                        npcType = NPC_VARROCK_TRADER;
                    } else if (strcmp(npcName, "varrock_bartender") == 0) {
                        npcType = NPC_VARROCK_BARTENDER;
                    } else if (strcmp(npcName, "alkharid_silk") == 0) {
                        npcType = NPC_ALKHARID_SILK;
                    } else if (strcmp(npcName, "alkharid_spice") == 0) {
                        npcType = NPC_ALKHARID_SPICE;
                    } else if (strcmp(npcName, "scimitar_shop") == 0) {
                        npcType = NPC_SCIMITAR_SHOP;
                    } else if (strcmp(npcName, "banker") == 0) {
                        npcType = NPC_BANKER;
                    } else {
                        TraceLog(LOG_WARNING, "Unknown NPC type: %s", npcName);
                    }
                    map.npcSpawns[map.npcCount] = { x + offsetX, y, z + offsetZ };
                    map.npcTypes[map.npcCount] = npcType;
                    map.npcCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_NPCS (%d) exceeded, skipping npc", MAX_NPCS);
            }
        }
        else if (strcmp(type, "lamp") == 0) {
            // Format: lamp x y z
            if (map.lightCount < MAX_LIGHTS) {
                float x, y, z;
                if (sscanf(line, "%*s %f %f %f", &x, &y, &z) == 3) {
                    map.lightSpawns[map.lightCount] = { x + offsetX, y, z + offsetZ };
                    map.lightTypes[map.lightCount] = LIGHT_LAMP;
                    map.lightCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_LIGHTS (%d) exceeded, skipping lamp", MAX_LIGHTS);
            }
        }
        else if (strcmp(type, "campfire") == 0) {
            // Format: campfire x y z
            if (map.lightCount < MAX_LIGHTS) {
                float x, y, z;
                if (sscanf(line, "%*s %f %f %f", &x, &y, &z) == 3) {
                    map.lightSpawns[map.lightCount] = { x + offsetX, y, z + offsetZ };
                    map.lightTypes[map.lightCount] = LIGHT_CAMPFIRE;
                    map.lightCount++;
                }
            } else {
                TraceLog(LOG_WARNING, "MAX_LIGHTS (%d) exceeded, skipping campfire", MAX_LIGHTS);
            }
        }
    }

    fclose(f);
    return true;
}

bool LoadMap(const char* filename, MapData& map) {
    // Initialize map data
    map.playerSpawn = { 0.0f, 1.8f, 0.0f };
    map.itemCount = 0;
    map.enemyCount = 0;
    map.wallCount = 0;
    map.treeCount = 0;
    map.rockCount = 0;
    map.waterCount = 0;
    map.sandCount = 0;
    map.valleyCount = 0;
    map.npcCount = 0;
    map.lightCount = 0;

    // Load the root map file with no offset
    bool success = LoadMapFile(filename, map, 0.0f, 0.0f, nullptr);

    if (success) {
        TraceLog(LOG_INFO, "Loaded map: %s (%d items, %d enemies, %d walls, %d trees, %d rocks, %d water, %d sand, %d valleys, %d npcs, %d lights)",
            filename, map.itemCount, map.enemyCount, map.wallCount, map.treeCount, map.rockCount, map.waterCount, map.sandCount, map.valleyCount, map.npcCount, map.lightCount);
    }

    return success;
}
