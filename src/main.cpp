#include "raylib.h"
#include <cstdio>
#include <ctime>
#include <cstring>
#include <sys/stat.h>

#include "types.h"
#include "math_utils.h"
#include "xp_system.h"
#include "map.h"
#include "save_system.h"
#include "rendering.h"
#include "spatial_hash.h"
#include "player.h"
#include "enemy_ai.h"
#include "combat.h"
#include "inventory.h"
#include "hud.h"
#include "game_init.h"
#include "lighting.h"

// Global heightmap data
float g_heightmap[HEIGHTMAP_SIZE][HEIGHTMAP_SIZE];
bool g_heightmapInitialized = false;

// Global spatial hash for O(1) proximity queries
WorldSpatialData g_spatial;

int main(int argc, char* argv[]) {
    // Check for --test flag (headless mode for CI/testing)
    bool testMode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            testMode = true;
        }
    }

    if (testMode) {
        printf("=== HEADLESS TEST MODE ===\n");

        MapData mapData = {};
        if (!LoadMap("maps/world.map", mapData)) {
            printf("FAIL: Could not load map\n");
            return 1;
        }

        printf("Map loaded successfully:\n");
        printf("  Items: %d / %d\n", mapData.itemCount, MAX_WORLD_ITEMS);
        printf("  Enemies: %d / %d\n", mapData.enemyCount, MAX_ENEMIES);
        printf("  Walls: %d / %d\n", mapData.wallCount, MAX_WALLS);
        printf("  Trees: %d / %d\n", mapData.treeCount, MAX_TREES);
        printf("  Water: %d / %d\n", mapData.waterCount, MAX_WATER);
        printf("  Sand: %d / %d\n", mapData.sandCount, MAX_SAND);
        printf("  Valleys: %d / %d\n", mapData.valleyCount, MAX_VALLEYS);
        printf("  Player spawn: (%.1f, %.1f, %.1f)\n",
               mapData.playerSpawn.x, mapData.playerSpawn.y, mapData.playerSpawn.z);

        g_spatial.Clear();
        for (int i = 0; i < mapData.wallCount; i++) {
            g_spatial.walls.InsertBox(i, mapData.walls[i].position.x, mapData.walls[i].position.z,
                                      mapData.walls[i].width, mapData.walls[i].depth);
        }
        std::vector<int> nearby;
        g_spatial.walls.Query(0.0f, 0.0f, 20.0f, nearby);
        printf("  Spatial hash test: %zu walls near origin\n", nearby.size());

        printf("=== TEST PASSED ===\n");
        return 0;
    }

    // Initialize window and audio
    int screenWidth, screenHeight;
    InitGameWindow(&screenWidth, &screenHeight);
    mkdir("screenshots", 0755);

    // Initialize player state
    PlayerState playerState = {};
    InitPlayerState(&playerState);
    if (LoadGame(playerState)) {
        TraceLog(LOG_INFO, "Loaded save game");
    }

    // Initialize camera
    Camera3D camera = {};
    InitCamera(&camera, &playerState);

    // Load map data
    MapData mapData = {};
    if (!LoadMap("maps/world.map", mapData)) {
        mapData.playerSpawn = { 0.0f, PLAYER_EYE_HEIGHT, 0.0f };
        mapData.itemSpawns[0] = { 0.0f, 0.0f, 3.0f };
        mapData.itemTypes[0] = ITEM_BRONZE_SHORTSWORD;
        mapData.itemCount = 1;
        mapData.enemySpawns[0] = { 10.0f, 0.0f, 10.0f };
        mapData.enemyTypes[0] = ENEMY_TROLL;
        mapData.enemyCount = 1;
    }

    // Initialize heightmap
    InitializeHeightmap(mapData);

    // Initialize game entities
    Wall walls[MAX_WALLS] = {};
    Water waterBodies[MAX_WATER] = {};
    Sand sandZones[MAX_SAND] = {};

    GameResources resources = LoadGameResources(mapData, walls, waterBodies, sandZones);

    // Initialize lighting system
    LightingSystem lighting = {};
    InitLightingSystem(&lighting);

    Enemy enemies[MAX_ENEMIES] = {};
    int enemyCount = 0;
    InitEnemiesFromMap(enemies, &enemyCount, mapData);

    Tree trees[MAX_TREES] = {};
    int treeCount = 0;
    InitTreesFromMap(trees, &treeCount, mapData);

    WorldItem worldItems[MAX_WORLD_ITEMS] = {};
    int worldItemCount = 0;
    InitItemsFromMap(worldItems, &worldItemCount, mapData, playerState.swordPickedUp);

    // Populate spatial hash
    PopulateSpatialHash(&g_spatial, walls, resources.wallCount, enemies, enemyCount, trees, treeCount);

    // Runtime state
    PlayerRuntime playerRuntime = {};
    InventoryMenu invMenu = {};
    DamageIndicator damageIndicators[MAX_DAMAGE_INDICATORS] = {};
    XPPopup xpPopups[MAX_XP_POPUPS] = {};
    LevelUpNotification levelUpNotif = {};

    float attackCooldown = 0.0f;
    float swingTimer = 0.0f;
    float screenshotMsgTimer = 0.0f;
    char screenshotMsg[128] = "";
    const char* statusMessage = nullptr;

    bool showActionMenu = false;
    WorldItem* targetItem = nullptr;

    bool mouseMode = false;
    DisableCursor();
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        // Update lighting system (day/night cycle, sun position)
        UpdateLightingSystem(&lighting, dt, camera.position);

        // Mouse mode toggle (hold shift for inventory)
        bool wasMouseMode = mouseMode;
        mouseMode = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if (mouseMode && !wasMouseMode) {
            EnableCursor();
            invMenu.showContextMenu = false;
        } else if (!mouseMode && wasMouseMode) {
            DisableCursor();
            invMenu.showContextMenu = false;
        }

        // Handle inventory input (mouse mode only)
        if (mouseMode) {
            const char* msg = HandleInventoryInput(&playerState, &playerRuntime, &invMenu,
                                                    worldItems, &worldItemCount,
                                                    camera.position,
                                                    xpPopups, &levelUpNotif,
                                                    screenWidth, screenHeight);
            if (msg) {
                strncpy(screenshotMsg, msg, sizeof(screenshotMsg) - 1);
                screenshotMsgTimer = 3.0f;
            }
        }

        // Player movement (when not in mouse mode and alive)
        if (!mouseMode && !playerRuntime.isDead) {
            std::vector<int> nearbyWalls;
            g_spatial.walls.Query(camera.position.x, camera.position.z, PLAYER_RADIUS + 20.0f, nearbyWalls);
            UpdatePlayerMovement(&camera, &playerRuntime, walls, resources.wallCount, nearbyWalls, dt);
        }

        // Duck animation (always updates)
        UpdateDuckAnimation(&camera, &playerRuntime, dt);

        // Attack cooldown
        if (attackCooldown > 0) attackCooldown -= dt;
        if (swingTimer > 0) swingTimer -= dt;

        // Enemy AI
        int damageToPlayer = UpdateEnemies(enemies, enemyCount, camera.position,
                                           playerRuntime.isDead, damageIndicators, dt);
        if (damageToPlayer > 0) {
            playerState.currentHP -= damageToPlayer;
            if (playerState.currentHP <= 0) {
                playerState.currentHP = 0;
                playerRuntime.isDead = true;
                playerRuntime.deathFadeTimer = DEATH_FADE_DURATION;
                playerRuntime.deathPosition = camera.position;
            }
        }

        // Tree respawning
        UpdateTrees(trees, treeCount, dt);

        // Player attack
        if (!mouseMode && !playerRuntime.isDead && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            attackCooldown <= 0 && playerState.equippedWeapon != ITEM_NONE) {
            ProcessPlayerAttack(&camera, &playerState, enemies, enemyCount,
                                trees, treeCount, worldItems, &worldItemCount,
                                damageIndicators, xpPopups, &levelUpNotif, &swingTimer);
            attackCooldown = PLAYER_ATTACK_COOLDOWN;
        }

        // HUD timers
        UpdateHUDTimers(damageIndicators, xpPopups, &levelUpNotif, dt);

        // HP regeneration
        UpdateHPRegen(&playerState, &playerRuntime, dt);

        // Player death
        UpdatePlayerDeath(&camera, &playerState, &playerRuntime,
                          worldItems, &worldItemCount,
                          enemies, enemyCount, mapData.playerSpawn, dt);

        // Item proximity check
        showActionMenu = false;
        targetItem = nullptr;
        for (int i = 0; i < worldItemCount; i++) {
            if (!worldItems[i].pickedUp) {
                Vector3 itemPos = worldItems[i].position;
                itemPos.y += GetTerrainHeight(itemPos.x, itemPos.z);
                if (Distance3D(camera.position, itemPos) <= PICKUP_RANGE) {
                    showActionMenu = true;
                    targetItem = &worldItems[i];
                    break;
                }
            }
        }

        // Action menu input
        if (showActionMenu && targetItem != nullptr) {
            if (IsKeyPressed(KEY_ONE)) {
                if (HandleItemPickup(&playerState, targetItem)) {
                    showActionMenu = false;
                    targetItem = nullptr;
                }
            } else if (IsKeyPressed(KEY_TWO)) {
                snprintf(screenshotMsg, sizeof(screenshotMsg), "It's a %s.", ITEM_NAMES[targetItem->type]);
                screenshotMsgTimer = 3.0f;
            } else if (IsKeyPressed(KEY_THREE)) {
                showActionMenu = false;
            }
        }

        // Screenshot
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

        if (screenshotMsgTimer > 0.0f) {
            screenshotMsgTimer -= dt;
            statusMessage = screenshotMsg;
        } else {
            statusMessage = nullptr;
        }

        // ========== SHADOW PASS ==========
        BeginShadowPass(&lighting, camera.position);
            // Draw shadow-casting geometry
            DrawModel(resources.groundModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

            // Walls cast shadows
            for (int i = 0; i < resources.wallCount; i++) {
                Vector3 pos = walls[i].position;
                pos.y += GetTerrainHeight(pos.x, pos.z) + walls[i].height / 2.0f;
                DrawModel(resources.wallModels[i], pos, 1.0f, WHITE);
            }

            // Trees cast shadows
            for (int i = 0; i < treeCount; i++) {
                if (trees[i].alive) {
                    Vector3 treePos = trees[i].position;
                    treePos.y = GetTerrainHeight(treePos.x, treePos.z);
                    DrawTree(&resources.entityModels, treePos, false);
                }
            }

            // Enemies cast shadows
            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i].alive) {
                    Vector3 enemyPos = enemies[i].position;
                    enemyPos.y = GetTerrainHeight(enemyPos.x, enemyPos.z);
                    Enemy adjustedEnemy = enemies[i];
                    adjustedEnemy.position = enemyPos;
                    DrawEnemy(&resources.entityModels, adjustedEnemy, false);
                }
            }
        EndShadowPass(&lighting);

        // ========== MAIN PASS ==========
        // Set lighting uniforms for all shaders
        SetShaderLightingUniforms(&lighting, resources.grassShader, camera.position);
        SetShaderLightingUniforms(&lighting, resources.sandShader, camera.position);
        SetShaderLightingUniforms(&lighting, resources.waterShader, camera.position);
        SetShaderLightingUniforms(&lighting, resources.entityShader, camera.position);
        for (int i = 0; i < WALL_MATERIAL_COUNT; i++) {
            SetShaderLightingUniforms(&lighting, resources.wallShaders[i], camera.position);
        }

        // Bind shadow map to all shaders
        BindShadowMapToShader(&lighting, resources.grassShader);
        BindShadowMapToShader(&lighting, resources.sandShader);
        BindShadowMapToShader(&lighting, resources.waterShader);
        BindShadowMapToShader(&lighting, resources.entityShader);
        for (int i = 0; i < WALL_MATERIAL_COUNT; i++) {
            BindShadowMapToShader(&lighting, resources.wallShaders[i]);
        }

        // Rendering
        BeginDrawing();
        Color skyColor = GetSkyColor(lighting.timeOfDay);
        ClearBackground(skyColor);

        BeginMode3D(camera);
            // Draw terrain
            DrawModel(resources.groundModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

            // Draw entities (models have entity shader assigned)
            // World items
            for (int i = 0; i < worldItemCount; i++) {
                if (!worldItems[i].pickedUp) {
                    Vector3 itemPos = worldItems[i].position;
                    itemPos.y += GetTerrainHeight(itemPos.x, itemPos.z);
                    DrawWorldItem(&resources.entityModels, worldItems[i].type, itemPos);
                }
            }

            // Enemies
            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i].alive) {
                    Vector3 enemyPos = enemies[i].position;
                    enemyPos.y = GetTerrainHeight(enemyPos.x, enemyPos.z);
                    float dist = Distance3D(camera.position, enemyPos);
                    bool inRange = (dist <= PLAYER_ATTACK_RANGE) && IsFacing(camera, enemyPos);
                    Enemy adjustedEnemy = enemies[i];
                    adjustedEnemy.position = enemyPos;
                    DrawEnemy(&resources.entityModels, adjustedEnemy, inRange);
                }
            }

            // Trees
            for (int i = 0; i < treeCount; i++) {
                if (trees[i].alive) {
                    Vector3 treePos = trees[i].position;
                    treePos.y = GetTerrainHeight(treePos.x, treePos.z);
                    float dist = Distance3D(camera.position, treePos);
                    bool inRange = (dist <= CHOP_RANGE) && IsFacing(camera, treePos) &&
                                   (playerState.equippedWeapon == ITEM_BRONZE_AXE);
                    DrawTree(&resources.entityModels, treePos, inRange);
                }
            }

            // Walls (have their own shaders)
            for (int i = 0; i < resources.wallCount; i++) {
                Vector3 pos = walls[i].position;
                pos.y += GetTerrainHeight(pos.x, pos.z) + walls[i].height / 2.0f;
                DrawModel(resources.wallModels[i], pos, 1.0f, WHITE);
            }

            // Water
            float gameTime = (float)GetTime();
            SetShaderValue(resources.waterShader, resources.waterTimeLoc, &gameTime, SHADER_UNIFORM_FLOAT);
            for (int i = 0; i < resources.waterCount; i++) {
                DrawModel(resources.waterModels[i], waterBodies[i].position, 1.0f, WHITE);
            }

            // Sand
            for (int i = 0; i < resources.sandCount; i++) {
                Vector3 sandPos = sandZones[i].position;
                sandPos.y = GetTerrainHeight(sandPos.x, sandPos.z) + 0.02f;
                DrawModel(resources.sandModels[i], sandPos, 1.0f, WHITE);
            }
        EndMode3D();

        // Draw HUD
        DrawHUD(&camera, &playerState, &playerRuntime,
                enemies, enemyCount,
                damageIndicators, xpPopups, &levelUpNotif,
                &invMenu, targetItem, showActionMenu,
                attackCooldown, swingTimer,
                mouseMode, statusMessage,
                screenWidth, screenHeight);

        EndDrawing();
    }

    // Save game state
    playerState.posX = camera.position.x;
    playerState.posY = camera.position.y;
    playerState.posZ = camera.position.z;
    playerState.targetX = camera.target.x;
    playerState.targetY = camera.target.y;
    playerState.targetZ = camera.target.z;
    playerState.swordPickedUp = (worldItemCount > 0) ? worldItems[0].pickedUp : false;
    SaveGame(playerState);
    TraceLog(LOG_INFO, "Game saved to %s", SAVE_FILE);

    // Cleanup
    UnloadLightingSystem(&lighting);
    CleanupGameResources(&resources);
    CloseWindow();
    return 0;
}
