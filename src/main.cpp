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
#include "game_systems.h"
#include "lighting.h"
#include "quest_system.h"
#include "voice_system.h"
#include "help_system.h"
#include "menu_system.h"

// Global heightmap data
float g_heightmap[HEIGHTMAP_SIZE][HEIGHTMAP_SIZE];
bool g_heightmapInitialized = false;

// Global spatial hash for O(1) proximity queries
WorldSpatialData g_spatial;

// Global winter mode flag
bool g_winterMode = false;

int main(int argc, char* argv[]) {
    // Check for command-line flags
    bool testMode = false;
    bool screenshotMode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            testMode = true;
        } else if (strcmp(argv[i], "--screenshot") == 0) {
            screenshotMode = true;
        } else if (strcmp(argv[i], "--winter") == 0) {
            g_winterMode = true;
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
        printf("  NPCs: %d / %d\n", mapData.npcCount, MAX_NPCS);
        printf("  Walls: %d / %d\n", mapData.wallCount, MAX_WALLS);
        printf("  Trees: %d / %d\n", mapData.treeCount, MAX_TREES);
        printf("  Water: %d / %d\n", mapData.waterCount, MAX_WATER);
        printf("  Sand: %d / %d\n", mapData.sandCount, MAX_SAND);
        printf("  Valleys: %d / %d\n", mapData.valleyCount, MAX_VALLEYS);
        printf("  Lights: %d / %d\n", mapData.lightCount, MAX_LIGHTS);
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
    // Note: LoadGame is called after quests are loaded (need quest IDs for progress)

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

    // Initialize post-processing system (bloom, SSAO)
    PostProcessSystem postProcess = {};
    InitPostProcessSystem(&postProcess, screenWidth, screenHeight);

    // Apply saved time of day
    lighting.timeOfDay = playerState.timeOfDay;

    Enemy enemies[MAX_ENEMIES] = {};
    int enemyCount = 0;
    InitEnemiesFromMap(enemies, &enemyCount, mapData);

    Tree trees[MAX_TREES] = {};
    int treeCount = 0;
    InitTreesFromMap(trees, &treeCount, mapData);

    WorldItem worldItems[MAX_WORLD_ITEMS] = {};
    int worldItemCount = 0;
    InitItemsFromMap(worldItems, &worldItemCount, mapData, playerState.swordPickedUp);

    // Initialize NPCs from map
    NPC npcs[MAX_NPCS] = {};
    int npcCount = 0;
    for (int i = 0; i < mapData.npcCount && npcCount < MAX_NPCS; i++) {
        npcs[npcCount].position = mapData.npcSpawns[i];
        npcs[npcCount].type = mapData.npcTypes[i];
        npcs[npcCount].facingAngle = 0.0f;
        npcs[npcCount].targetFacingAngle = 0.0f;
        npcs[npcCount].active = true;
        npcCount++;
    }

    // Initialize light sources from map (lamps, campfires)
    LightSource lights[MAX_LIGHTS] = {};
    int lightCount = 0;
    for (int i = 0; i < mapData.lightCount && lightCount < MAX_LIGHTS; i++) {
        lights[lightCount].position = mapData.lightSpawns[i];
        lights[lightCount].type = mapData.lightTypes[i];
        lightCount++;
    }

    // Extract lamp positions for point lighting
    Vector3 lampPositions[MAX_LIGHTS];
    int lampCount = 0;
    for (int i = 0; i < lightCount; i++) {
        if (lights[i].type == LIGHT_LAMP) {
            lampPositions[lampCount++] = lights[i].position;
        }
    }
    SetLampPositions(&lighting, lampPositions, lampCount);

    // Extract campfire positions for point lighting (always on)
    Vector3 campfirePositions[MAX_LIGHTS];
    int campfireCount = 0;
    for (int i = 0; i < lightCount; i++) {
        if (lights[i].type == LIGHT_CAMPFIRE) {
            campfirePositions[campfireCount++] = lights[i].position;
        }
    }
    SetCampfirePositions(&lighting, campfirePositions, campfireCount);

    // Load quests
    Quest quests[MAX_QUESTS] = {};
    int questCount = LoadAllQuests(quests, MAX_QUESTS);

    // Load saved game (after quests so we can map progress by quest ID)
    if (LoadGame(playerState, quests, questCount)) {
        TraceLog(LOG_INFO, "Loaded save game");
        // Re-apply saved camera position and time of day after loading
        camera.position = (Vector3){ playerState.posX, playerState.posY, playerState.posZ };
        camera.target = (Vector3){ playerState.targetX, playerState.targetY, playerState.targetZ };
        lighting.timeOfDay = playerState.timeOfDay;
    }

    // Populate spatial hash
    PopulateSpatialHash(&g_spatial, walls, resources.wallCount, enemies, enemyCount, trees, treeCount);

    // Runtime state
    PlayerRuntime playerRuntime = {};
    InventoryMenu invMenu = {};
    DamageIndicator damageIndicators[MAX_DAMAGE_INDICATORS] = {};
    XPPopup xpPopups[MAX_XP_POPUPS] = {};
    LevelUpNotification levelUpNotif = {};
    DialogueState dialogueState = {false, -1, 0};
    ShopState shopState = {false, -1, {}, 0, -1};

    // Help system
    HelpSystem helpSystem = {};
    InitHelpSystem(&helpSystem);

    // Time selection menu
    TimeSelectMenu timeSelectMenu = {};

    // Unified menu system (initialized below after mouseMode is declared)
    MenuSystem menuSystem = {};

    // Snow system (only initialized in winter mode)
    SnowSystem snowSystem = {};
    if (g_winterMode) {
        InitSnowSystem(&snowSystem, camera.position);
    }

    // Quest dialogue state
    int activeQuestIndex = -1;              // Which quest is active in current dialogue
    const char** questDialogueLines = nullptr;  // Current quest dialogue lines
    int questDialogueCount = 0;             // Number of lines
    bool showQuestAcceptPrompt = false;     // Show accept/decline buttons

    float attackCooldown = 0.0f;
    float swingTimer = 0.0f;
    float screenshotMsgTimer = 0.0f;
    char screenshotMsg[128] = "";
    const char* statusMessage = nullptr;
    const char* attackMessage = nullptr;
    float attackMessageTimer = 0.0f;
    float autosaveTimer = 0.0f;
    const float AUTOSAVE_INTERVAL = 5.0f;

    bool showActionMenu = false;
    WorldItem* targetItem = nullptr;

    bool mouseMode = false;
    bool showQuitConfirm = false;
    bool shouldQuit = false;

    // Initialize menu system (after mouseMode is declared)
    InitMenuSystem(&menuSystem, &mouseMode, &dialogueState, &shopState, &timeSelectMenu, &helpSystem);

    DisableCursor();
    SetTargetFPS(60);

    // Main game loop
    while (!shouldQuit) {
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

        // Player movement (when not in any menu and alive)
        if (!playerRuntime.isDead && CanProcessGameInput(&menuSystem)) {
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
                // Save on death
                playerState.posX = camera.position.x;
                playerState.posY = camera.position.y;
                playerState.posZ = camera.position.z;
                playerState.targetX = camera.target.x;
                playerState.targetY = camera.target.y;
                playerState.targetZ = camera.target.z;
                playerState.timeOfDay = lighting.timeOfDay;
                SaveGame(playerState, quests, questCount);
            }
        }

        // Tree respawning
        UpdateTrees(trees, treeCount, dt);

        // Item respawning
        UpdateItemRespawns(worldItems, worldItemCount, dt);

        // Player attack (don't attack while in any menu)
        if (!playerRuntime.isDead && CanProcessGameInput(&menuSystem) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            attackCooldown <= 0 && playerState.equippedWeapon != ITEM_NONE) {
            const char* newAttackMsg = nullptr;
            ProcessPlayerAttack(&camera, &playerState, enemies, enemyCount,
                                trees, treeCount, worldItems, &worldItemCount,
                                damageIndicators, xpPopups, &levelUpNotif, &swingTimer,
                                &newAttackMsg);
            attackCooldown = GetWeaponCooldown(playerState.equippedWeapon);
            if (newAttackMsg) {
                attackMessage = newAttackMsg;
                attackMessageTimer = 2.0f;  // Show for 2 seconds
            }
        }

        // Update attack message timer
        if (attackMessageTimer > 0) {
            attackMessageTimer -= dt;
            if (attackMessageTimer <= 0) {
                attackMessage = nullptr;
            }
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

        // Find nearest NPC for interaction
        int nearestNPCIndex = -1;
        float nearestNPCDist = NPC_INTERACTION_RANGE;
        for (int i = 0; i < npcCount; i++) {
            if (npcs[i].active) {
                Vector3 npcPos = npcs[i].position;
                npcPos.y = GetTerrainHeight(npcPos.x, npcPos.z);
                float dist = Distance3D(camera.position, npcPos);
                if (dist < nearestNPCDist) {
                    nearestNPCDist = dist;
                    nearestNPCIndex = i;
                }
            }
        }

        // Help system handling (takes priority when active)
        bool helpWasOpen = (helpSystem.state != HelpState::CLOSED);
        if (helpWasOpen) {
            UpdateHelpSystem(&helpSystem, quests, questCount, &playerState);
        } else if (!dialogueState.active && !mouseMode && !playerRuntime.isDead) {
            // 'H' key opens help
            if (IsKeyPressed(KEY_H)) {
                OpenHelpUI(&helpSystem, quests, questCount, &playerState);
                EnableCursor();
            }
        }

        // Handle help UI just closed - restore cursor state
        bool helpJustClosed = helpWasOpen && (helpSystem.state == HelpState::CLOSED);
        if (helpJustClosed && !mouseMode && !dialogueState.active) {
            DisableCursor();
        }

        // Quit confirmation handling
        // Skip if help UI just closed (it consumed the ESC)
        if (showQuitConfirm) {
            if (IsKeyPressed(KEY_Y)) {
                shouldQuit = true;
            } else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)) {
                showQuitConfirm = false;
                if (!mouseMode && !dialogueState.active) {
                    DisableCursor();
                }
            }
        } else if (IsKeyPressed(KEY_ESCAPE) && !helpJustClosed) {
            // ESC priority: help UI > bank > dialogue > shop > time menu > show quit prompt
            if (helpSystem.state != HelpState::CLOSED) {
                // Help system handles its own ESC
            } else if (menuSystem.bank.active) {
                // Close bank
                CloseBank(&menuSystem);
            } else if (dialogueState.active) {
                // Dialogue handles its own ESC (see below)
            } else if (shopState.active) {
                // Shop handles its own ESC (see below)
            } else if (timeSelectMenu.active) {
                // Close time menu
                timeSelectMenu.active = false;
                DisableCursor();
            } else {
                // Show quit confirmation
                showQuitConfirm = true;
                EnableCursor();
            }
        }

        // Shop input handling
        if (shopState.active) {
            // ESC to close shop
            if (IsKeyPressed(KEY_ESCAPE)) {
                shopState.active = false;
                shopState.selectedIndex = -1;
                if (!mouseMode) DisableCursor();
            }
            // Mouse click handling
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();
                const int BOX_WIDTH = 400;
                const int BOX_HEIGHT = 350;
                const int BOX_X = (screenWidth - BOX_WIDTH) / 2;
                const int BOX_Y = (screenHeight - BOX_HEIGHT) / 2;
                const int ITEM_SIZE = 80;
                const int ITEM_SPACING = 30;
                int startX = BOX_X + (BOX_WIDTH - (3 * ITEM_SIZE + 2 * ITEM_SPACING)) / 2;
                int itemY = BOX_Y + 100;

                // Check item slot clicks
                for (int i = 0; i < shopState.itemCount; i++) {
                    int itemX = startX + i * (ITEM_SIZE + ITEM_SPACING);
                    if (mouse.x >= itemX && mouse.x <= itemX + ITEM_SIZE &&
                        mouse.y >= itemY && mouse.y <= itemY + ITEM_SIZE) {
                        shopState.selectedIndex = i;
                        break;
                    }
                }

                // Check buy button click
                if (shopState.selectedIndex >= 0) {
                    int detailY = itemY + ITEM_SIZE + 50;
                    int btnW = 120, btnH = 35;
                    int btnX = BOX_X + (BOX_WIDTH - btnW) / 2;
                    int btnY = detailY + 35;

                    if (mouse.x >= btnX && mouse.x <= btnX + btnW &&
                        mouse.y >= btnY && mouse.y <= btnY + btnH) {
                        int price = shopState.items[shopState.selectedIndex].price;
                        ItemType item = shopState.items[shopState.selectedIndex].item;
                        if (GetGilCount(&playerState) >= price) {
                            if (AddToInventory(&playerState, item)) {
                                RemoveGil(&playerState, price);
                                snprintf(screenshotMsg, sizeof(screenshotMsg),
                                         "Purchased %s!", ITEM_NAMES[item]);
                                screenshotMsgTimer = 2.0f;
                            } else {
                                snprintf(screenshotMsg, sizeof(screenshotMsg), "Inventory full!");
                                screenshotMsgTimer = 2.0f;
                            }
                        }
                    }
                }
            }
        }

        // Bank input handling
        if (menuSystem.bank.active) {
            const char* bankMsg = UpdateBankInput(&menuSystem, &playerState, screenWidth, screenHeight);
            if (bankMsg) {
                strncpy(screenshotMsg, bankMsg, sizeof(screenshotMsg) - 1);
                screenshotMsgTimer = 2.0f;
            }
        }

        // NPC dialogue handling (only when other blocking menus are closed)
        // Note: We check shop/help/bank/time but NOT dialogue - we need to handle active dialogue here
        if (helpSystem.state == HelpState::CLOSED && !shopState.active &&
            !timeSelectMenu.active && !menuSystem.bank.active) {
            if (!dialogueState.active) {
                // Start dialogue when pressing E near an NPC
                if (nearestNPCIndex >= 0 && !mouseMode && !playerRuntime.isDead) {
                    if (IsKeyPressed(KEY_E)) {
                        NPCType npcType = npcs[nearestNPCIndex].type;

                        // Check if this is a shop NPC
                        if (npcType == NPC_SCIMITAR_SHOP) {
                            // Open shop UI instead of dialogue
                            shopState.active = true;
                            shopState.npcIndex = nearestNPCIndex;
                            shopState.selectedIndex = -1;
                            shopState.itemCount = 3;
                            shopState.items[0] = {ITEM_STEEL_SCIMITAR, 200};
                            shopState.items[1] = {ITEM_MITHRIL_SCIMITAR, 500};
                            shopState.items[2] = {ITEM_ADAMANT_SCIMITAR, 3000};
                            EnableCursor();
                        } else if (npcType == NPC_BANKER) {
                            // Open bank UI
                            OpenBank(&menuSystem);
                        } else {
                            // Normal dialogue
                            dialogueState.active = true;
                            dialogueState.npcIndex = nearestNPCIndex;
                            dialogueState.currentLine = 0;
                            EnableCursor();

                            // Speak first line of dialogue (will be updated after quest check)
                            NPCType speakNpcType = npcs[nearestNPCIndex].type;

                            // Check if this NPC has a quest
                            activeQuestIndex = FindQuestByNPC(quests, questCount, npcType);

                            if (activeQuestIndex >= 0) {
                                // Get quest dialogue
                                questDialogueLines = GetQuestDialogue(
                                    &quests[activeQuestIndex],
                                    &playerState.questProgress[activeQuestIndex],
                                    &playerState,
                                    npcType,
                                    &questDialogueCount,
                                    &showQuestAcceptPrompt
                                );
                            } else {
                                questDialogueLines = nullptr;
                                questDialogueCount = 0;
                                showQuestAcceptPrompt = false;
                            }

                            // Speak the first dialogue line
                            const char* firstLine = (questDialogueLines && questDialogueCount > 0)
                                ? questDialogueLines[0]
                                : NPC_CONFIGS[speakNpcType].dialogueLines[0];
                            if (firstLine) {
                                SpeakAsNPC(firstLine, speakNpcType);
                            }
                        }
                    }
                }
            } else {
            // In dialogue - determine current dialogue source
            bool isQuestDialogue = (activeQuestIndex >= 0 && questDialogueLines != nullptr);
            int totalLines = isQuestDialogue ? questDialogueCount :
                             NPC_CONFIGS[npcs[dialogueState.npcIndex].type].dialogueCount;

            // Check for accept/decline button clicks (only on last line of quest intro)
            bool onLastLine = (dialogueState.currentLine >= totalLines - 1);
            bool acceptClicked = false;
            bool declineClicked = false;

            if (showQuestAcceptPrompt && onLastLine) {
                Vector2 mouse = GetMousePosition();

                // Accept button bounds (approximate - will match HUD rendering)
                int dialogueBoxY = screenHeight - 180 - 40;
                int buttonY = dialogueBoxY + 130;
                int acceptX = screenWidth / 2 - 120;
                int declineX = screenWidth / 2 + 20;
                int buttonW = 100;
                int buttonH = 30;

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (mouse.x >= acceptX && mouse.x <= acceptX + buttonW &&
                        mouse.y >= buttonY && mouse.y <= buttonY + buttonH) {
                        acceptClicked = true;
                    } else if (mouse.x >= declineX && mouse.x <= declineX + buttonW &&
                               mouse.y >= buttonY && mouse.y <= buttonY + buttonH) {
                        declineClicked = true;
                    }
                }
            }

            if (acceptClicked) {
                // Accept quest
                NPCType npcType = npcs[dialogueState.npcIndex].type;
                AdvanceQuest(&quests[activeQuestIndex],
                            &playerState.questProgress[activeQuestIndex],
                            &playerState, npcType);

                // Show "Quest started" message
                snprintf(screenshotMsg, sizeof(screenshotMsg), "Quest started: %s",
                         quests[activeQuestIndex].name);
                screenshotMsgTimer = 3.0f;

                // Close dialogue
                StopSpeaking();
                dialogueState.active = false;
                dialogueState.npcIndex = -1;
                dialogueState.currentLine = 0;
                activeQuestIndex = -1;
                questDialogueLines = nullptr;
                showQuestAcceptPrompt = false;
                if (!mouseMode) {
                    DisableCursor();
                }
            } else if (declineClicked) {
                // Decline - just close dialogue
                StopSpeaking();
                dialogueState.active = false;
                dialogueState.npcIndex = -1;
                dialogueState.currentLine = 0;
                activeQuestIndex = -1;
                questDialogueLines = nullptr;
                showQuestAcceptPrompt = false;
                if (!mouseMode) {
                    DisableCursor();
                }
            } else if (!showQuestAcceptPrompt || !onLastLine) {
                // Normal dialogue advancement (click/space/E)
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                    IsKeyPressed(KEY_SPACE) ||
                    IsKeyPressed(KEY_E)) {

                    if (dialogueState.currentLine < totalLines - 1) {
                        // Advance to next line
                        dialogueState.currentLine++;

                        // Speak the new line
                        NPCType speakNpc = npcs[dialogueState.npcIndex].type;
                        const char* nextLine = isQuestDialogue
                            ? questDialogueLines[dialogueState.currentLine]
                            : NPC_CONFIGS[speakNpc].dialogueLines[dialogueState.currentLine];
                        if (nextLine) {
                            SpeakAsNPC(nextLine, speakNpc);
                        }
                    } else {
                        // Last line - handle quest advancement or close
                        if (isQuestDialogue && activeQuestIndex >= 0) {
                            QuestProgress* progress = &playerState.questProgress[activeQuestIndex];
                            NPCType npcType = npcs[dialogueState.npcIndex].type;

                            // If in progress and can turn in, do it
                            bool canAdvance = CanAdvanceQuest(&quests[activeQuestIndex], progress, &playerState, npcType);

                            if (progress->state == QUEST_IN_PROGRESS && canAdvance) {
                                bool completed = AdvanceQuest(&quests[activeQuestIndex],
                                                              progress, &playerState, npcType);

                                if (completed) {
                                    snprintf(screenshotMsg, sizeof(screenshotMsg),
                                             "Quest complete: %s! +%d gold, +%d QP",
                                             quests[activeQuestIndex].name,
                                             quests[activeQuestIndex].rewardGil,
                                             quests[activeQuestIndex].rewardQuestPoints);
                                    screenshotMsgTimer = 4.0f;
                                }
                            }
                        }

                        // End dialogue
                        StopSpeaking();
                        dialogueState.active = false;
                        dialogueState.npcIndex = -1;
                        dialogueState.currentLine = 0;
                        activeQuestIndex = -1;
                        questDialogueLines = nullptr;
                        showQuestAcceptPrompt = false;
                        if (!mouseMode) {
                            DisableCursor();
                        }
                    }
                }
            }

            // Escape to exit dialogue early
            if (IsKeyPressed(KEY_ESCAPE)) {
                StopSpeaking();
                dialogueState.active = false;
                dialogueState.npcIndex = -1;
                dialogueState.currentLine = 0;
                activeQuestIndex = -1;
                questDialogueLines = nullptr;
                showQuestAcceptPrompt = false;
                if (!mouseMode) {
                    DisableCursor();
                }
            }

            // NPC faces player during dialogue
            if (dialogueState.npcIndex >= 0) {
                float dx = camera.position.x - npcs[dialogueState.npcIndex].position.x;
                float dz = camera.position.z - npcs[dialogueState.npcIndex].position.z;
                npcs[dialogueState.npcIndex].targetFacingAngle = atan2f(dx, dz);

                // Smooth rotation toward target
                float angleDiff = NormalizeAngle(npcs[dialogueState.npcIndex].targetFacingAngle -
                                                  npcs[dialogueState.npcIndex].facingAngle);
                float rotateAmount = NPC_TURN_SPEED * dt;
                if (fabsf(angleDiff) < rotateAmount) {
                    npcs[dialogueState.npcIndex].facingAngle = npcs[dialogueState.npcIndex].targetFacingAngle;
                } else {
                    npcs[dialogueState.npcIndex].facingAngle += (angleDiff > 0 ? 1 : -1) * rotateAmount;
                }
            }
            }
        }

        // Screenshot (not while typing in help UI)
        if (IsKeyPressed(KEY_P) && CanProcessScreenshotKey(&menuSystem)) {
            time_t now = time(nullptr);
            char filename[64];
            strftime(filename, sizeof(filename), "screenshots/%Y%m%d_%H%M%S.png", localtime(&now));
            Image screenshot = LoadImageFromScreen();
            ExportImage(screenshot, filename);
            UnloadImage(screenshot);
            snprintf(screenshotMsg, sizeof(screenshotMsg), "Saved: %s", filename);
            screenshotMsgTimer = 2.0f;
        }

        // Time selection menu toggle (T key)
        if (IsKeyPressed(KEY_T) && CanProcessHotkeys(&menuSystem)) {
            timeSelectMenu.active = !timeSelectMenu.active;
            if (timeSelectMenu.active) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

        if (screenshotMsgTimer > 0.0f) {
            screenshotMsgTimer -= dt;
            statusMessage = screenshotMsg;
        } else if (attackMessage) {
            statusMessage = attackMessage;
        } else {
            statusMessage = nullptr;
        }

        // Autosave every 5 seconds
        autosaveTimer += dt;
        if (autosaveTimer >= AUTOSAVE_INTERVAL) {
            autosaveTimer = 0.0f;
            // Update playerState with current camera position before saving
            playerState.posX = camera.position.x;
            playerState.posY = camera.position.y;
            playerState.posZ = camera.position.z;
            playerState.targetX = camera.target.x;
            playerState.targetY = camera.target.y;
            playerState.targetZ = camera.target.z;
            playerState.timeOfDay = lighting.timeOfDay;
            SaveGame(playerState, quests, questCount);
        }

        // Reload game (0 key) - useful for development (not while in menus)
        if (IsKeyPressed(KEY_ZERO) && CanProcessHotkeys(&menuSystem)) {
            // Save current state first
            playerState.posX = camera.position.x;
            playerState.posY = camera.position.y;
            playerState.posZ = camera.position.z;
            playerState.targetX = camera.target.x;
            playerState.targetY = camera.target.y;
            playerState.targetZ = camera.target.z;
            playerState.timeOfDay = lighting.timeOfDay;
            SaveGame(playerState, quests, questCount);

            // Reload map
            MapData newMapData = {};
            if (LoadMap("maps/world.map", newMapData)) {
                mapData = newMapData;
                InitializeHeightmap(mapData);

                // Reinitialize enemies
                enemyCount = 0;
                memset(enemies, 0, sizeof(enemies));
                InitEnemiesFromMap(enemies, &enemyCount, mapData);

                // Reinitialize trees
                treeCount = 0;
                memset(trees, 0, sizeof(trees));
                InitTreesFromMap(trees, &treeCount, mapData);

                // Reinitialize items
                worldItemCount = 0;
                memset(worldItems, 0, sizeof(worldItems));
                InitItemsFromMap(worldItems, &worldItemCount, mapData, playerState.swordPickedUp);

                // Reinitialize NPCs
                npcCount = 0;
                memset(npcs, 0, sizeof(npcs));
                for (int i = 0; i < mapData.npcCount && npcCount < MAX_NPCS; i++) {
                    npcs[npcCount].position = mapData.npcSpawns[i];
                    npcs[npcCount].type = mapData.npcTypes[i];
                    npcs[npcCount].facingAngle = 0.0f;
                    npcs[npcCount].targetFacingAngle = 0.0f;
                    npcs[npcCount].active = true;
                    npcCount++;
                }

                // Reload quests
                for (int i = 0; i < questCount; i++) {
                    FreeQuest(&quests[i]);
                }
                questCount = LoadAllQuests(quests, MAX_QUESTS);

                // Reload save (to restore quest progress etc.)
                LoadGame(playerState, quests, questCount);
                camera.position = (Vector3){ playerState.posX, playerState.posY, playerState.posZ };
                camera.target = (Vector3){ playerState.targetX, playerState.targetY, playerState.targetZ };
                lighting.timeOfDay = playerState.timeOfDay;

                // Repopulate spatial hash
                PopulateSpatialHash(&g_spatial, walls, resources.wallCount, enemies, enemyCount, trees, treeCount);

                // Reset runtime state
                playerRuntime = {};
                dialogueState = {false, -1, 0};
                activeQuestIndex = -1;
                questDialogueLines = nullptr;

                snprintf(screenshotMsg, sizeof(screenshotMsg), "Game reloaded");
                screenshotMsgTimer = 2.0f;
                TraceLog(LOG_INFO, "Game reloaded via 0 key");
            }
        }

        // ========== SHADOW PASS ==========
        BeginShadowPass(&lighting, camera.position, resources.depthShader);
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
                    DrawTree(&resources.entityModels, treePos, trees[i].type, false);
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

            // NPCs cast shadows
            for (int i = 0; i < npcCount; i++) {
                if (npcs[i].active) {
                    Vector3 npcPos = npcs[i].position;
                    npcPos.y = GetTerrainHeight(npcPos.x, npcPos.z);
                    NPC adjustedNPC = npcs[i];
                    adjustedNPC.position = npcPos;
                    DrawNPC(&resources.entityModels, adjustedNPC);
                }
            }

            // Light sources cast shadows (lamps, campfires)
            for (int i = 0; i < lightCount; i++) {
                Vector3 lightPos = lights[i].position;
                lightPos.y = GetTerrainHeight(lightPos.x, lightPos.z);
                LightSource adjustedLight = lights[i];
                adjustedLight.position = lightPos;
                DrawLightSource(&resources.entityModels, adjustedLight, lighting.lampsOn);
            }
        EndShadowPass(&lighting);

        // ========== MAIN PASS ==========
        // Set lighting uniforms for all shaders
        SetShaderLightingUniforms(&lighting, resources.grassShader, camera.position);
        SetShaderLightingUniforms(&lighting, resources.waterShader, camera.position);
        SetShaderLightingUniforms(&lighting, resources.entityShader, camera.position);
        SetShaderLightingUniforms(&lighting, resources.grass.bladeShader, camera.position);
        for (int i = 0; i < WALL_MATERIAL_COUNT; i++) {
            SetShaderLightingUniforms(&lighting, resources.wallShaders[i], camera.position);
        }

        // Bind shadow map to all shaders
        BindShadowMapToShader(&lighting, resources.grassShader);
        BindShadowMapToShader(&lighting, resources.waterShader);
        BindShadowMapToShader(&lighting, resources.entityShader);
        for (int i = 0; i < WALL_MATERIAL_COUNT; i++) {
            BindShadowMapToShader(&lighting, resources.wallShaders[i]);
        }

        // ========== RENDER SCENE TO TEXTURE (for post-processing) ==========
        BeginTextureMode(postProcess.sceneTexture);
        Color skyColor = GetSkyColor(lighting.timeOfDay);
        ClearBackground(skyColor);

        BeginMode3D(camera);
            // Draw sky (disable depth write and backface culling since we're inside the sphere)
            SetSkyShaderUniforms(&lighting, resources.skyShader);
            rlDisableDepthMask();
            rlDisableBackfaceCulling();
            DrawModel(resources.skyModel, camera.position, 1.0f, WHITE);
            rlEnableBackfaceCulling();
            rlEnableDepthMask();

            // Draw terrain
            DrawModel(resources.groundModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

            // Draw grass blades
            DrawGrassBlades(&resources.grass, (float)GetTime());

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

            // NPCs
            for (int i = 0; i < npcCount; i++) {
                if (npcs[i].active) {
                    Vector3 npcPos = npcs[i].position;
                    npcPos.y = GetTerrainHeight(npcPos.x, npcPos.z);
                    NPC adjustedNPC = npcs[i];
                    adjustedNPC.position = npcPos;
                    DrawNPC(&resources.entityModels, adjustedNPC);
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
                    DrawTree(&resources.entityModels, treePos, trees[i].type, inRange);
                }
            }

            // Light sources (lamps, campfires)
            for (int i = 0; i < lightCount; i++) {
                Vector3 lightPos = lights[i].position;
                lightPos.y = GetTerrainHeight(lightPos.x, lightPos.z);
                LightSource adjustedLight = lights[i];
                adjustedLight.position = lightPos;
                DrawLightSource(&resources.entityModels, adjustedLight, lighting.lampsOn);
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

            // Snow particles (winter mode only)
            if (g_winterMode) {
                UpdateAndDrawSnow(&snowSystem, camera.position, GetFrameTime());
            }
        EndMode3D();
        EndTextureMode();

        // ========== POST-PROCESSING ==========
        // Get projection matrix for SSAO
        Matrix projMatrix = MatrixPerspective(camera.fovy * DEG2RAD,
                                               (float)screenWidth / (float)screenHeight,
                                               0.1f, 1000.0f);
        RenderSSAO(&postProcess, camera, projMatrix);
        RenderBloom(&postProcess);

        // ========== FINAL COMPOSITE + HUD ==========
        BeginDrawing();
        ClearBackground(BLACK);
        CompositeScene(&postProcess);

        // Draw minimap
        float playerYaw = atan2f(camera.target.x - camera.position.x,
                                  camera.target.z - camera.position.z);
        DrawMinimap(camera.position, playerYaw,
                    enemies, enemyCount,
                    npcs, npcCount,
                    trees, treeCount,
                    walls, resources.wallCount,
                    screenWidth, screenHeight);

        // Draw HUD
        DrawHUD(&camera, &playerState, &playerRuntime,
                enemies, enemyCount,
                damageIndicators, xpPopups, &levelUpNotif,
                &invMenu, targetItem, showActionMenu,
                attackCooldown, swingTimer,
                mouseMode, statusMessage,
                screenWidth, screenHeight);

        // Draw NPC prompt (when near an NPC but not in dialogue/shop/bank)
        if (nearestNPCIndex >= 0 && !dialogueState.active && !shopState.active &&
            !menuSystem.bank.active && !mouseMode && !playerRuntime.isDead) {
            const NPCConfig& config = NPC_CONFIGS[npcs[nearestNPCIndex].type];
            DrawNPCPrompt(config.name, screenWidth, screenHeight);
        }

        // Draw dialogue box (when in dialogue)
        DrawDialogueBox(&dialogueState, npcs, questDialogueLines, questDialogueCount,
                        showQuestAcceptPrompt, screenWidth, screenHeight);

        // Draw shop UI (when shop is open)
        DrawShopUI(&shopState, &playerState, screenWidth, screenHeight);

        // Draw bank UI (when bank is open)
        DrawBankUI(&menuSystem, &playerState, screenWidth, screenHeight);

        // Draw time selection menu and handle clicks
        int timePreset = DrawTimeSelectMenu(&timeSelectMenu, lighting.timeOfDay, screenWidth, screenHeight);
        if (timePreset > 0) {
            float presets[] = { TIME_PRESET_DAWN, TIME_PRESET_NOON, TIME_PRESET_DUSK, TIME_PRESET_MIDNIGHT };
            lighting.timeOfDay = presets[timePreset - 1];
            timeSelectMenu.active = false;
            DisableCursor();
        }

        // Draw help UI (on top of everything)
        DrawHelpUI(&helpSystem, screenWidth, screenHeight);

        // Draw quit confirmation (on very top)
        if (showQuitConfirm) {
            // Semi-transparent overlay
            DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 150});

            // Dialog box
            const int BOX_W = 400;
            const int BOX_H = 150;
            const int BOX_X = (screenWidth - BOX_W) / 2;
            const int BOX_Y = (screenHeight - BOX_H) / 2;

            DrawRectangle(BOX_X - 4, BOX_Y - 4, BOX_W + 8, BOX_H + 8, (Color){139, 90, 43, 255});
            DrawRectangle(BOX_X, BOX_Y, BOX_W, BOX_H, (Color){222, 198, 158, 240});
            DrawRectangleLines(BOX_X + 6, BOX_Y + 6, BOX_W - 12, BOX_H - 12, (Color){180, 150, 100, 255});

            const char* title = "Save and Quit?";
            int titleW = MeasureText(title, 28);
            DrawText(title, BOX_X + (BOX_W - titleW) / 2, BOX_Y + 25, 28, (Color){139, 90, 43, 255});

            const char* prompt = "Your progress will be saved.";
            int promptW = MeasureText(prompt, 18);
            DrawText(prompt, BOX_X + (BOX_W - promptW) / 2, BOX_Y + 65, 18, (Color){60, 40, 20, 255});

            const char* controls = "Press Y to quit, N to cancel";
            int controlsW = MeasureText(controls, 16);
            DrawText(controls, BOX_X + (BOX_W - controlsW) / 2, BOX_Y + 105, 16, (Color){60, 40, 20, 200});
        }

        EndDrawing();

        // Screenshot mode: capture and exit after a few frames (allow GPU to fully render)
        static int screenshotFrameCount = 0;
        if (screenshotMode) {
            screenshotFrameCount++;
            if (screenshotFrameCount >= 3) {  // Wait 3 frames for GPU to stabilize
                time_t now = time(nullptr);
                char filename[64];
                strftime(filename, sizeof(filename), "screenshots/%Y%m%d_%H%M%S.png", localtime(&now));
                Image screenshot = LoadImageFromScreen();
                ExportImage(screenshot, filename);
                UnloadImage(screenshot);
                printf("Screenshot saved: %s\n", filename);

                // Cleanup and exit
                ShutdownHelpSystem(&helpSystem);
                UnloadLightingSystem(&lighting);
                CleanupGameResources(&resources);
                CloseWindow();
                return 0;
            }
        }
    }

    // Save game state
    playerState.posX = camera.position.x;
    playerState.posY = camera.position.y;
    playerState.posZ = camera.position.z;
    playerState.targetX = camera.target.x;
    playerState.targetY = camera.target.y;
    playerState.targetZ = camera.target.z;
    playerState.swordPickedUp = (worldItemCount > 0) ? worldItems[0].pickedUp : false;
    playerState.timeOfDay = lighting.timeOfDay;
    SaveGame(playerState, quests, questCount);
    TraceLog(LOG_INFO, "Game saved to %s", SAVE_FILE);

    // Cleanup
    ShutdownHelpSystem(&helpSystem);
    UnloadPostProcessSystem(&postProcess);
    UnloadLightingSystem(&lighting);
    CleanupGameResources(&resources);
    CloseWindow();
    return 0;
}
