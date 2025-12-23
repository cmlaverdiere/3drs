#include "raylib.h"
#include <cstdio>
#include <ctime>
#include <cmath>
#include <sys/stat.h>

#include "types.h"
#include "math_utils.h"
#include "xp_system.h"
#include "collision.h"
#include "map.h"
#include "save_system.h"
#include "rendering.h"
#include "game_systems.h"
#include "sound_system.h"

int main() {
    InitWindow(1, 1, "3D RuneScape-style Game");
    int monitorWidth = GetMonitorWidth(0);
    int monitorHeight = GetMonitorHeight(0);
    CloseWindow();

    int screenWidth = monitorWidth;
    int screenHeight = monitorHeight - 80;
    InitWindow(screenWidth, screenHeight, "3D RuneScape-style Game");
    SetWindowPosition(0, 25);

    // Initialize audio
    InitAudioDevice();
    InitSoundSystem();

    // Initialize player state with defaults
    PlayerState playerState = {};
    playerState.posX = 0.0f;
    playerState.posY = 1.8f;
    playerState.posZ = 0.0f;
    playerState.targetX = 0.0f;
    playerState.targetY = 1.8f;
    playerState.targetZ = 1.0f;
    for (int i = 0; i < SKILL_COUNT; i++) {
        playerState.skillXP[i] = 0;
    }
    playerState.skillXP[SKILL_HITPOINTS] = XP_TABLE[9];
    for (int i = 0; i < INV_SLOTS; i++) {
        playerState.inventory[i] = ITEM_NONE;
        playerState.inventoryCount[i] = 0;
    }
    playerState.equippedWeapon = ITEM_NONE;
    playerState.swordPickedUp = false;
    playerState.maxHP = 10;
    playerState.currentHP = 10;

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

    // Load wall shaders
    Shader wallShaders[WALL_MATERIAL_COUNT];
    wallShaders[WALL_WOOD] = LoadShader("shaders/wall.vs", "shaders/wood.fs");
    wallShaders[WALL_STONE] = LoadShader("shaders/wall.vs", "shaders/stone.fs");
    wallShaders[WALL_BRICK] = LoadShader("shaders/wall.vs", "shaders/brick.fs");

    mkdir("screenshots", 0755);

    float screenshotMsgTimer = 0.0f;
    char screenshotMsg[128] = "";

    // Load map data
    MapData mapData = {};
    if (!LoadMap("maps/world.map", mapData)) {
        mapData.playerSpawn = { 0.0f, 1.8f, 0.0f };
        mapData.itemSpawns[0] = { 0.0f, 0.0f, 3.0f };
        mapData.itemTypes[0] = ITEM_BRONZE_SHORTSWORD;
        mapData.itemCount = 1;
        mapData.enemySpawns[0] = { 10.0f, 0.0f, 10.0f };
        mapData.enemyTypes[0] = ENEMY_TROLL;
        mapData.enemyCount = 1;
    }

    // Initialize world items from map
    WorldItem worldItems[MAX_WORLD_ITEMS] = {};
    int worldItemCount = mapData.itemCount;
    for (int i = 0; i < worldItemCount; i++) {
        worldItems[i].type = mapData.itemTypes[i];
        worldItems[i].position = mapData.itemSpawns[i];
        worldItems[i].pickedUp = false;
    }
    if (worldItemCount > 0) {
        worldItems[0].pickedUp = playerState.swordPickedUp;
    }

    const float PICKUP_RANGE = 2.5f;
    bool showActionMenu = false;
    WorldItem* targetItem = nullptr;

    // Inventory context menu state
    bool showInvMenu = false;
    int invMenuSlot = -1;
    int invMenuX = 0;
    int invMenuY = 0;

    // Copy walls from map data and create models
    Wall walls[MAX_WALLS] = {};
    Model wallModels[MAX_WALLS] = {};
    int wallCount = mapData.wallCount;
    for (int i = 0; i < wallCount; i++) {
        walls[i] = mapData.walls[i];

        // Create a cube mesh for this wall
        Mesh wallMesh = GenMeshCube(walls[i].width, walls[i].height, walls[i].depth);
        wallModels[i] = LoadModelFromMesh(wallMesh);
        wallModels[i].materials[0].shader = wallShaders[walls[i].material];
    }

    // Initialize enemies from map spawn points
    Enemy enemies[MAX_ENEMIES] = {};
    int enemyCount = mapData.enemyCount;
    for (int i = 0; i < enemyCount; i++) {
        enemies[i].type = mapData.enemyTypes[i];
        enemies[i].spawnPoint = mapData.enemySpawns[i];
        enemies[i].position = mapData.enemySpawns[i];
        enemies[i].health = ENEMY_CONFIGS[enemies[i].type].maxHealth;
        enemies[i].alive = true;
        enemies[i].respawnTimer = 0.0f;
        enemies[i].wanderTimer = 0.0f;
        enemies[i].wanderTarget = mapData.enemySpawns[i];
        enemies[i].hostile = false;
        enemies[i].attackCooldown = 0.0f;
        enemies[i].facingAngle = RandomFloat(0.0f, 2.0f * PI);
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

        // Hold shift for mouse mode
        bool wasMouseMode = mouseMode;
        mouseMode = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if (mouseMode && !wasMouseMode) {
            EnableCursor();
            showInvMenu = false;  // Close menu when entering mouse mode
        } else if (!mouseMode && wasMouseMode) {
            DisableCursor();
            showInvMenu = false;  // Close menu when leaving mouse mode
        }

        // Inventory click handling (mouse mode only)
        if (mouseMode) {
            Vector2 mouse = GetMousePosition();
            int invX = screenWidth - (INV_COLS * (SLOT_SIZE + SLOT_PADDING)) - 20;
            int invY = 60;

            // Handle inventory context menu clicks
            if (showInvMenu && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                const int menuWidth = 80;
                const int menuItemHeight = 20;
                ItemType menuItem = playerState.inventory[invMenuSlot];
                bool isWeapon = (menuItem == ITEM_BRONZE_SHORTSWORD);

                int optionCount = isWeapon ? 4 : 3;  // Use/Equip, Examine, Drop, Cancel vs Examine, Drop, Cancel
                int menuHeight = menuItemHeight * optionCount;

                if (mouse.x >= invMenuX && mouse.x <= invMenuX + menuWidth &&
                    mouse.y >= invMenuY && mouse.y <= invMenuY + menuHeight) {
                    int optionIdx = (int)(mouse.y - invMenuY) / menuItemHeight;

                    if (isWeapon) {
                        // Weapon menu: Use, Examine, Drop, Cancel
                        if (optionIdx == 0) {
                            // Use/Equip
                            if (playerState.equippedWeapon == menuItem) {
                                playerState.equippedWeapon = ITEM_NONE;
                            } else {
                                playerState.equippedWeapon = menuItem;
                            }
                            showInvMenu = false;
                        } else if (optionIdx == 1) {
                            // Examine
                            const char* itemName = ITEM_NAMES[menuItem];
                            snprintf(screenshotMsg, sizeof(screenshotMsg), "It's a %s.", itemName);
                            screenshotMsgTimer = 3.0f;
                            showInvMenu = false;
                        } else if (optionIdx == 2) {
                            // Drop
                            if (worldItemCount < MAX_WORLD_ITEMS) {
                                worldItems[worldItemCount].type = menuItem;
                                worldItems[worldItemCount].position = camera.position;
                                worldItems[worldItemCount].position.y = 0.0f;
                                worldItems[worldItemCount].pickedUp = false;
                                worldItemCount++;

                                if (playerState.equippedWeapon == menuItem) {
                                    playerState.equippedWeapon = ITEM_NONE;
                                }
                                playerState.inventory[invMenuSlot] = ITEM_NONE;
                                playerState.inventoryCount[invMenuSlot] = 0;
                            }
                            showInvMenu = false;
                        } else {
                            // Cancel
                            showInvMenu = false;
                        }
                    } else {
                        // Non-weapon menu: Examine, Drop, Cancel
                        if (optionIdx == 0) {
                            // Examine
                            const char* itemName = ITEM_NAMES[menuItem];
                            if (IsItemStackable(menuItem) && playerState.inventoryCount[invMenuSlot] > 1) {
                                snprintf(screenshotMsg, sizeof(screenshotMsg), "%d x %s.", playerState.inventoryCount[invMenuSlot], itemName);
                            } else {
                                snprintf(screenshotMsg, sizeof(screenshotMsg), "It's a %s.", itemName);
                            }
                            screenshotMsgTimer = 3.0f;
                            showInvMenu = false;
                        } else if (optionIdx == 1) {
                            // Drop (drop one at a time for stackable items)
                            if (worldItemCount < MAX_WORLD_ITEMS) {
                                worldItems[worldItemCount].type = menuItem;
                                worldItems[worldItemCount].position = camera.position;
                                worldItems[worldItemCount].position.y = 0.0f;
                                worldItems[worldItemCount].pickedUp = false;
                                worldItemCount++;

                                if (IsItemStackable(menuItem) && playerState.inventoryCount[invMenuSlot] > 1) {
                                    playerState.inventoryCount[invMenuSlot]--;
                                } else {
                                    playerState.inventory[invMenuSlot] = ITEM_NONE;
                                    playerState.inventoryCount[invMenuSlot] = 0;
                                }
                            }
                            showInvMenu = false;
                        } else {
                            // Cancel
                            showInvMenu = false;
                        }
                    }
                } else {
                    // Clicked outside menu, close it
                    showInvMenu = false;
                }
            }
            // Right-click to open context menu
            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                bool clickedSlot = false;
                for (int row = 0; row < INV_ROWS && !clickedSlot; row++) {
                    for (int col = 0; col < INV_COLS && !clickedSlot; col++) {
                        int slotIdx = row * INV_COLS + col;
                        int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
                        int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);

                        if (mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
                            mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE) {
                            if (playerState.inventory[slotIdx] != ITEM_NONE) {
                                showInvMenu = true;
                                invMenuSlot = slotIdx;
                                invMenuX = (int)mouse.x;
                                invMenuY = (int)mouse.y;
                                clickedSlot = true;
                            }
                        }
                    }
                }
                if (!clickedSlot) {
                    showInvMenu = false;
                }
            }
            // Left-click on inventory (quick equip for weapons)
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !showInvMenu) {
                for (int row = 0; row < INV_ROWS; row++) {
                    for (int col = 0; col < INV_COLS; col++) {
                        int slotIdx = row * INV_COLS + col;
                        int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
                        int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);

                        if (mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
                            mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE) {
                            ItemType clickedItem = playerState.inventory[slotIdx];
                            if (clickedItem == ITEM_BRONZE_SHORTSWORD) {
                                if (playerState.equippedWeapon == clickedItem) {
                                    playerState.equippedWeapon = ITEM_NONE;
                                } else {
                                    playerState.equippedWeapon = clickedItem;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!mouseMode && !playerDead) {
            UpdateCamera(&camera, CAMERA_FIRST_PERSON);

            const float PLAYER_RADIUS = 0.3f;
            for (int i = 0; i < wallCount; i++) {
                if (PointInWall(camera.position, walls[i], PLAYER_RADIUS)) {
                    Vector3 oldPos = camera.position;
                    camera.position = ResolveWallCollision(camera.position, walls[i], PLAYER_RADIUS);
                    camera.target.x += camera.position.x - oldPos.x;
                    camera.target.z += camera.position.z - oldPos.z;
                }
            }
        }

        if (attackCooldown > 0) {
            attackCooldown -= dt;
        }

        if (swingTimer > 0) {
            swingTimer -= dt;
        }

        // Update enemies
        for (int i = 0; i < enemyCount; i++) {
            const EnemyConfig& config = ENEMY_CONFIGS[enemies[i].type];

            if (enemies[i].attackCooldown > 0) {
                enemies[i].attackCooldown -= dt;
            }

            if (enemies[i].alive) {
                const float TURN_SPEED = 5.0f;  // Radians per second

                if (enemies[i].hostile && !playerDead) {
                    float dx = camera.position.x - enemies[i].position.x;
                    float dz = camera.position.z - enemies[i].position.z;
                    float dist = sqrtf(dx*dx + dz*dz);

                    // Turn to face player
                    float targetAngle = atan2f(dx, dz);
                    float angleDiff = targetAngle - enemies[i].facingAngle;
                    // Normalize angle difference to [-PI, PI]
                    while (angleDiff > PI) angleDiff -= 2.0f * PI;
                    while (angleDiff < -PI) angleDiff += 2.0f * PI;
                    // Smooth rotation
                    float maxTurn = TURN_SPEED * dt;
                    if (fabsf(angleDiff) < maxTurn) {
                        enemies[i].facingAngle = targetAngle;
                    } else {
                        enemies[i].facingAngle += (angleDiff > 0 ? maxTurn : -maxTurn);
                    }

                    if (dist > config.attackRange) {
                        float speed = config.chaseSpeed * dt;
                        enemies[i].position.x += (dx / dist) * speed;
                        enemies[i].position.z += (dz / dist) * speed;
                    } else if (enemies[i].attackCooldown <= 0) {
                        int damage = GetRandomValue(0, config.maxHit);
                        playerState.currentHP -= damage;
                        SpawnDamageIndicator(damageIndicators, camera.position, damage);
                        enemies[i].attackCooldown = config.attackCooldown;
                        if (damage > 0) PlaySoundEffect(SFX_PLAYER_HURT);

                        if (playerState.currentHP <= 0) {
                            playerState.currentHP = 0;
                            playerDead = true;
                            deathFadeTimer = DEATH_FADE_DURATION;
                            deathPosition = camera.position;
                        }
                    }
                } else {
                    enemies[i].wanderTimer -= dt;
                    if (enemies[i].wanderTimer <= 0) {
                        enemies[i].wanderTarget.x = enemies[i].spawnPoint.x + RandomFloat(-3.0f, 3.0f);
                        enemies[i].wanderTarget.z = enemies[i].spawnPoint.z + RandomFloat(-3.0f, 3.0f);
                        enemies[i].wanderTimer = RandomFloat(2.0f, 5.0f);
                    }

                    float dx = enemies[i].wanderTarget.x - enemies[i].position.x;
                    float dz = enemies[i].wanderTarget.z - enemies[i].position.z;
                    float dist = sqrtf(dx*dx + dz*dz);
                    if (dist > 0.5f) {
                        // Turn to face wander target
                        float targetAngle = atan2f(dx, dz);
                        float angleDiff = targetAngle - enemies[i].facingAngle;
                        while (angleDiff > PI) angleDiff -= 2.0f * PI;
                        while (angleDiff < -PI) angleDiff += 2.0f * PI;
                        float maxTurn = TURN_SPEED * 0.5f * dt;  // Turn slower when wandering
                        if (fabsf(angleDiff) < maxTurn) {
                            enemies[i].facingAngle = targetAngle;
                        } else {
                            enemies[i].facingAngle += (angleDiff > 0 ? maxTurn : -maxTurn);
                        }

                        float speed = 1.0f * dt;
                        enemies[i].position.x += (dx / dist) * speed;
                        enemies[i].position.z += (dz / dist) * speed;
                    }
                }
            } else {
                enemies[i].respawnTimer -= dt;
                if (enemies[i].respawnTimer <= 0) {
                    enemies[i].position.x = enemies[i].spawnPoint.x + RandomFloat(-5.0f, 5.0f);
                    enemies[i].position.z = enemies[i].spawnPoint.z + RandomFloat(-5.0f, 5.0f);
                    enemies[i].position.y = 0.0f;
                    enemies[i].health = config.maxHealth;
                    enemies[i].alive = true;
                    enemies[i].wanderTimer = 0.0f;
                    enemies[i].hostile = false;
                    enemies[i].attackCooldown = 0.0f;
                    enemies[i].facingAngle = RandomFloat(0.0f, 2.0f * PI);
                }
            }
        }

        // Attack with mouse click
        if (!mouseMode && !playerDead && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && attackCooldown <= 0 && playerState.equippedWeapon != ITEM_NONE) {
            swingTimer = SWING_DURATION;
            attackCooldown = PLAYER_ATTACK_COOLDOWN;

            int combatLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
            int maxHit = CalculateMaxHit(combatLevel);

            Enemy* target = nullptr;
            float closestDist = PLAYER_ATTACK_RANGE + 1.0f;

            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i].alive) {
                    float dist = Distance3D(camera.position, enemies[i].position);
                    if (dist <= PLAYER_ATTACK_RANGE && dist < closestDist && IsFacing(camera, enemies[i].position)) {
                        target = &enemies[i];
                        closestDist = dist;
                    }
                }
            }

            if (target != nullptr) {
                const EnemyConfig& config = ENEMY_CONFIGS[target->type];
                target->hostile = true;

                int damage = RollDamage(maxHit);
                target->health -= damage;
                SpawnDamageIndicator(damageIndicators, target->position, damage);

                // Play hit or miss sound
                if (damage > 0) {
                    PlaySoundEffect(SFX_HIT);
                } else {
                    PlaySoundEffect(SFX_MISS);
                }

                if (target->health <= 0) {
                    target->alive = false;
                    target->respawnTimer = config.respawnTime;
                    PlaySoundEffect(SFX_ENEMY_DEATH);

                    // Spawn drops
                    SpawnEnemyDrops(config, target->position, worldItems, worldItemCount);

                    // Award XP for kill (4 XP per hitpoint, like OSRS)
                    int xpGain = config.maxHealth * 4;

                    int oldLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
                    playerState.skillXP[SKILL_COMBAT] += xpGain;
                    int newLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
                    SpawnXPPopup(xpPopups, xpGain, SKILL_COMBAT);
                    PlaySoundEffect(SFX_XP_GAIN);

                    if (newLevel > oldLevel) {
                        levelUpNotif.skillIndex = SKILL_COMBAT;
                        levelUpNotif.newLevel = newLevel;
                        levelUpNotif.timer = LEVEL_UP_DURATION;
                        levelUpNotif.active = true;
                        PlaySoundEffect(SFX_LEVEL_UP);
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

        // HP regeneration
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

            if (deathFadeTimer > DEATH_FADE_DURATION - dt - 0.01f) {
                for (int i = 0; i < INV_SLOTS; i++) {
                    if (playerState.inventory[i] != ITEM_NONE) {
                        // Drop items based on stack count
                        int dropCount = playerState.inventoryCount[i];
                        for (int j = 0; j < dropCount && worldItemCount < MAX_WORLD_ITEMS; j++) {
                            worldItems[worldItemCount].type = playerState.inventory[i];
                            worldItems[worldItemCount].position = deathPosition;
                            worldItems[worldItemCount].position.x += RandomFloat(-1.0f, 1.0f);
                            worldItems[worldItemCount].position.z += RandomFloat(-1.0f, 1.0f);
                            worldItems[worldItemCount].position.y = 0.0f;
                            worldItems[worldItemCount].pickedUp = false;
                            worldItemCount++;
                        }

                        playerState.inventory[i] = ITEM_NONE;
                        playerState.inventoryCount[i] = 0;
                    }
                }
                playerState.equippedWeapon = ITEM_NONE;
            }

            if (deathFadeTimer <= 0) {
                playerDead = false;
                camera.position = mapData.playerSpawn;
                camera.target = (Vector3){ mapData.playerSpawn.x, mapData.playerSpawn.y, mapData.playerSpawn.z + 1.0f };
                playerState.maxHP = GetLevelFromXP(playerState.skillXP[SKILL_HITPOINTS]);
                playerState.currentHP = playerState.maxHP;
                for (int i = 0; i < enemyCount; i++) {
                    enemies[i].hostile = false;
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
                    break;
                }
            }
        }

        // Handle action menu input
        if (showActionMenu && targetItem != nullptr) {
            if (IsKeyPressed(KEY_ONE)) {
                bool pickedUp = false;
                ItemType itemType = targetItem->type;

                // For stackable items, try to add to existing stack first
                if (IsItemStackable(itemType)) {
                    for (int i = 0; i < INV_SLOTS; i++) {
                        if (playerState.inventory[i] == itemType) {
                            playerState.inventoryCount[i]++;
                            pickedUp = true;
                            break;
                        }
                    }
                }

                // If not stacked, find an empty slot
                if (!pickedUp) {
                    for (int i = 0; i < INV_SLOTS; i++) {
                        if (playerState.inventory[i] == ITEM_NONE) {
                            playerState.inventory[i] = itemType;
                            playerState.inventoryCount[i] = 1;
                            pickedUp = true;
                            break;
                        }
                    }
                }

                if (pickedUp) {
                    targetItem->pickedUp = true;
                    showActionMenu = false;
                    targetItem = nullptr;
                    PlaySoundEffect(SFX_PICKUP);
                }
            } else if (IsKeyPressed(KEY_TWO)) {
                // Examine item
                const char* itemName = ITEM_NAMES[targetItem->type];
                snprintf(screenshotMsg, sizeof(screenshotMsg), "It's a %s.", itemName);
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

                // Draw world items
                for (int i = 0; i < worldItemCount; i++) {
                    if (!worldItems[i].pickedUp) {
                        DrawWorldItem(worldItems[i].type, worldItems[i].position);
                    }
                }

                // Draw enemies
                for (int i = 0; i < enemyCount; i++) {
                    if (enemies[i].alive) {
                        float dist = Distance3D(camera.position, enemies[i].position);
                        bool inRange = (dist <= PLAYER_ATTACK_RANGE) && IsFacing(camera, enemies[i].position);
                        DrawEnemy(enemies[i], inRange);
                    }
                }

                for (int i = 0; i < wallCount; i++) {
                    Vector3 pos = walls[i].position;
                    pos.y += walls[i].height / 2.0f;
                    DrawModel(wallModels[i], pos, 1.0f, WHITE);
                }
            EndMode3D();

            // HUD
            DrawText("WASD to move, Mouse to look, Hold SHIFT for inventory, LMB to attack", 10, 10, 20, WHITE);
            if (mouseMode) {
                DrawText("[INVENTORY MODE - Release SHIFT to resume]", 10, 35, 16, YELLOW);
            }
            DrawFPS(screenWidth - 100, 10);

            // Player HP bar
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
                int cdWidth = (int)(100 * (attackCooldown / PLAYER_ATTACK_COOLDOWN));
                DrawRectangle(screenWidth/2 - 50, screenHeight - 40, 100, 10, DARKGRAY);
                DrawRectangle(screenWidth/2 - 50, screenHeight - 40, cdWidth, 10, RED);
            }

            // Draw damage indicators
            for (int i = 0; i < MAX_DAMAGE_INDICATORS; i++) {
                if (damageIndicators[i].active) {
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
                    if (Dot3D(toIndicator, camForward) <= 0) continue;

                    Vector2 screenPos = GetWorldToScreen(damageIndicators[i].position, camera);
                    if (screenPos.x > 0 && screenPos.x < screenWidth &&
                        screenPos.y > 0 && screenPos.y < screenHeight) {
                        char dmgText[16];
                        snprintf(dmgText, sizeof(dmgText), "%d", damageIndicators[i].damage);
                        float timeRatio = damageIndicators[i].timer / DAMAGE_INDICATOR_DURATION;
                        float alpha = (timeRatio > 0.2f) ? 1.0f : (timeRatio / 0.2f);
                        int fontSize = 48;
                        int textWidth = MeasureText(dmgText, fontSize);
                        int tx = (int)screenPos.x - textWidth/2;
                        int ty = (int)screenPos.y - fontSize/2;

                        Color outlineColor = BLACK;
                        outlineColor.a = (unsigned char)(255 * alpha);
                        for (int ox = -2; ox <= 2; ox++) {
                            for (int oy = -2; oy <= 2; oy++) {
                                if (ox != 0 || oy != 0) {
                                    DrawText(dmgText, tx + ox, ty + oy, fontSize, outlineColor);
                                }
                            }
                        }

                        Color dmgColor = (damageIndicators[i].damage == 0) ? BLUE : RED;
                        dmgColor.a = (unsigned char)(255 * alpha);
                        DrawText(dmgText, tx, ty, fontSize, dmgColor);
                    }
                }
            }

            // Draw enemy health bars
            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i].alive) {
                    const EnemyConfig& config = ENEMY_CONFIGS[enemies[i].type];

                    Vector3 toEnemy = {
                        enemies[i].position.x - camera.position.x,
                        0,
                        enemies[i].position.z - camera.position.z
                    };
                    Vector3 camForward = {
                        camera.target.x - camera.position.x,
                        0,
                        camera.target.z - camera.position.z
                    };
                    if (Dot3D(toEnemy, camForward) <= 0) continue;

                    Vector3 healthBarPos = { enemies[i].position.x, enemies[i].position.y + 2.0f, enemies[i].position.z };
                    Vector2 screenPos = GetWorldToScreen(healthBarPos, camera);
                    if (screenPos.x > 0 && screenPos.x < screenWidth &&
                        screenPos.y > 0 && screenPos.y < screenHeight) {
                        int barWidth = 40;
                        int barHeight = 6;
                        int healthWidth = (int)(barWidth * enemies[i].health / (float)config.maxHealth);
                        DrawRectangle((int)screenPos.x - barWidth/2, (int)screenPos.y, barWidth, barHeight, DARKGRAY);
                        DrawRectangle((int)screenPos.x - barWidth/2, (int)screenPos.y, healthWidth, barHeight, GREEN);
                        DrawRectangleLines((int)screenPos.x - barWidth/2, (int)screenPos.y, barWidth, barHeight, BLACK);

                        // Draw enemy name above health bar
                        int nameWidth = MeasureText(config.name, 12);
                        DrawText(config.name, (int)screenPos.x - nameWidth/2, (int)screenPos.y - 14, 12, WHITE);
                    }
                }
            }

            // Crosshair
            if (!mouseMode) {
                int cx = screenWidth / 2;
                int cy = screenHeight / 2;
                DrawLine(cx - 10, cy, cx + 10, cy, WHITE);
                DrawLine(cx, cy - 10, cx, cy + 10, WHITE);
            }

            // FPS weapon view
            if (playerState.equippedWeapon == ITEM_BRONZE_SHORTSWORD) {
                float weaponBaseX = screenWidth - 150.0f;
                float weaponBaseY = screenHeight - 100.0f;

                float swingAngle = 0.0f;
                float swingOffsetX = 0.0f;
                float swingOffsetY = 0.0f;
                if (swingTimer > 0) {
                    float swingProgress = swingTimer / SWING_DURATION;
                    swingAngle = sinf(swingProgress * PI) * 60.0f;
                    swingOffsetX = -sinf(swingProgress * PI) * 80.0f;
                    swingOffsetY = -sinf(swingProgress * PI) * 40.0f;
                }

                float wpnX = weaponBaseX + swingOffsetX;
                float wpnY = weaponBaseY + swingOffsetY;

                Color bronzeBlade = { 205, 127, 50, 255 };
                Color bronzeHandle = { 139, 90, 43, 255 };

                float radAngle = swingAngle * DEG2RAD;
                float cosA = cosf(radAngle);
                float sinA = sinf(radAngle);

                float bladeLen = 120.0f;
                float bladeWidth = 12.0f;

                Vector2 bladeTip = { wpnX + (-bladeLen * sinA), wpnY + (-bladeLen * cosA) };
                Vector2 bladeBase = { wpnX, wpnY };

                DrawLineEx(bladeBase, bladeTip, bladeWidth, bronzeBlade);
                DrawLineEx(bladeBase, bladeTip, bladeWidth + 2, DARKGRAY);
                DrawLineEx(bladeBase, bladeTip, bladeWidth, bronzeBlade);

                Vector2 handleEnd = { wpnX + (30.0f * sinA), wpnY + (30.0f * cosA) };
                DrawLineEx(bladeBase, handleEnd, 10.0f, bronzeHandle);

                Vector2 guardLeft = { wpnX + (-15.0f * cosA), wpnY + (15.0f * sinA) };
                Vector2 guardRight = { wpnX + (15.0f * cosA), wpnY + (-15.0f * sinA) };
                DrawLineEx(guardLeft, guardRight, 6.0f, bronzeHandle);
            }

            // XP popups
            int xpPopupY = screenHeight / 3;
            for (int i = 0; i < MAX_XP_POPUPS; i++) {
                if (xpPopups[i].active) {
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

                    char xpText[64];
                    snprintf(xpText, sizeof(xpText), "+%d %s", xpPopups[i].xpAmount, SKILL_NAMES[skillIdx]);
                    int textWidth = MeasureText(xpText, 28);
                    int popupX = (screenWidth - textWidth) / 2;

                    Color outlineColor = { 0, 0, 0, (unsigned char)(200 * alpha) };
                    for (int ox = -2; ox <= 2; ox++) {
                        for (int oy = -2; oy <= 2; oy++) {
                            if (ox != 0 || oy != 0) {
                                DrawText(xpText, popupX + ox, xpPopupY + oy, 28, outlineColor);
                            }
                        }
                    }

                    Color xpColor = { 255, 215, 0, (unsigned char)(255 * alpha) };
                    DrawText(xpText, popupX, xpPopupY, 28, xpColor);

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

                    char levelText[32];
                    snprintf(levelText, sizeof(levelText), "Lv %d", currentLevel);
                    Color levelColor = { 255, 255, 255, (unsigned char)(255 * alpha) };
                    DrawText(levelText, barX + barWidth + 10, barY - 2, 14, levelColor);

                    xpPopupY += 55;
                }
            }

            // Skills display
            int skillY = 60;
            DrawText("Skills:", 10, skillY, 18, GOLD);
            skillY += 22;
            for (int i = 0; i < SKILL_COUNT; i++) {
                int level = GetLevelFromXP(playerState.skillXP[i]);
                int xpForNext = (level < 99) ? XP_TABLE[level] : XP_TABLE[98];
                char skillText[64];
                snprintf(skillText, sizeof(skillText), "%s: %d (%d/%d)",
                    SKILL_NAMES[i], level, playerState.skillXP[i], xpForNext);
                DrawText(skillText, 10, skillY, 14, WHITE);
                skillY += 18;
            }

            // Action menu overlay
            if (showActionMenu && targetItem != nullptr) {
                int menuX = screenWidth / 2 - 100;
                int menuY = screenHeight / 2 - 60;
                int menuW = 200;
                int menuH = 120;

                DrawRectangle(menuX, menuY, menuW, menuH, (Color){0, 0, 0, 180});
                DrawRectangleLines(menuX, menuY, menuW, menuH, GOLD);

                const char* itemName = ITEM_NAMES[targetItem->type];
                DrawText(itemName, menuX + 10, menuY + 10, 18, GOLD);
                DrawText("1. Pickup", menuX + 10, menuY + 40, 16, WHITE);
                DrawText("2. Examine", menuX + 10, menuY + 60, 16, WHITE);
                DrawText("3. Cancel", menuX + 10, menuY + 80, 16, GRAY);
            }

            // Inventory UI
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

                    bool isEquipped = (playerState.inventory[slotIdx] != ITEM_NONE &&
                                       playerState.inventory[slotIdx] == playerState.equippedWeapon);
                    if (isEquipped) {
                        DrawRectangleLines(slotX, slotY, SLOT_SIZE, SLOT_SIZE, (Color){255, 215, 0, 255});
                        DrawRectangleLines(slotX+1, slotY+1, SLOT_SIZE-2, SLOT_SIZE-2, (Color){255, 215, 0, 255});
                    } else {
                        DrawRectangleLines(slotX, slotY, SLOT_SIZE, SLOT_SIZE, (Color){86, 74, 57, 255});
                    }

                    // Draw inventory item icons
                    ItemType item = playerState.inventory[slotIdx];
                    int cx = slotX + SLOT_SIZE / 2;
                    int cy = slotY + SLOT_SIZE / 2;

                    if (item == ITEM_BRONZE_SHORTSWORD) {
                        Color bronzeColor = { 205, 127, 50, 255 };
                        DrawRectangle(cx - 2, cy - 14, 4, 24, bronzeColor);
                        DrawRectangle(cx - 2, cy + 10, 4, 8, BROWN);
                        DrawRectangle(cx - 8, cy + 8, 16, 3, BROWN);
                    } else if (item == ITEM_COW_HIDE) {
                        Color hideColor = { 139, 90, 43, 255 };
                        DrawRectangle(cx - 12, cy - 8, 24, 16, hideColor);
                        DrawRectangle(cx - 4, cy - 4, 6, 6, DARKBROWN);
                    } else if (item == ITEM_BONES) {
                        Color boneColor = { 230, 220, 200, 255 };
                        DrawRectangle(cx - 2, cy - 10, 4, 20, boneColor);
                        DrawCircle(cx, cy - 10, 4, boneColor);
                        DrawCircle(cx, cy + 10, 4, boneColor);
                    } else if (item == ITEM_GIL) {
                        Color goldColor = { 255, 215, 0, 255 };
                        DrawCircle(cx, cy, 10, goldColor);
                        DrawCircle(cx, cy, 6, GOLD);
                    }

                    // Draw stack count for stackable items
                    if (item != ITEM_NONE && IsItemStackable(item) && playerState.inventoryCount[slotIdx] > 1) {
                        char countText[16];
                        snprintf(countText, sizeof(countText), "%d", playerState.inventoryCount[slotIdx]);
                        DrawText(countText, slotX + 2, slotY + 2, 10, YELLOW);
                    }
                }
            }

            // Draw inventory context menu
            if (showInvMenu && invMenuSlot >= 0 && playerState.inventory[invMenuSlot] != ITEM_NONE) {
                const int menuWidth = 80;
                const int menuItemHeight = 20;
                const int menuPadding = 4;
                ItemType menuItem = playerState.inventory[invMenuSlot];
                bool isWeapon = (menuItem == ITEM_BRONZE_SHORTSWORD);

                const char* options[4];
                int optionCount;
                if (isWeapon) {
                    bool equipped = (playerState.equippedWeapon == menuItem);
                    options[0] = equipped ? "Unequip" : "Equip";
                    options[1] = "Examine";
                    options[2] = "Drop";
                    options[3] = "Cancel";
                    optionCount = 4;
                } else {
                    options[0] = "Examine";
                    options[1] = "Drop";
                    options[2] = "Cancel";
                    optionCount = 3;
                }

                int menuHeight = menuItemHeight * optionCount;

                // Clamp menu position to screen
                int menuX = invMenuX;
                int menuY = invMenuY;
                if (menuX + menuWidth > screenWidth) menuX = screenWidth - menuWidth;
                if (menuY + menuHeight > screenHeight) menuY = screenHeight - menuHeight;

                // Update stored position for click detection
                invMenuX = menuX;
                invMenuY = menuY;

                // Draw menu background
                DrawRectangle(menuX, menuY, menuWidth, menuHeight, (Color){40, 35, 28, 240});
                DrawRectangleLines(menuX, menuY, menuWidth, menuHeight, (Color){86, 74, 57, 255});

                // Draw menu options
                Vector2 mouse = GetMousePosition();
                for (int i = 0; i < optionCount; i++) {
                    int optY = menuY + i * menuItemHeight;

                    // Highlight on hover
                    if (mouse.x >= menuX && mouse.x <= menuX + menuWidth &&
                        mouse.y >= optY && mouse.y <= optY + menuItemHeight) {
                        DrawRectangle(menuX + 1, optY + 1, menuWidth - 2, menuItemHeight - 2, (Color){60, 55, 45, 255});
                    }

                    DrawText(options[i], menuX + menuPadding, optY + 4, 12, (Color){255, 204, 0, 255});
                }
            }

            if (screenshotMsgTimer > 0.0f) {
                DrawText(screenshotMsg, 10, screenHeight - 80, 20, YELLOW);
            }

            // Death blackout overlay
            if (playerDead) {
                float fadeProgress = 1.0f - (deathFadeTimer / DEATH_FADE_DURATION);
                unsigned char alpha = (unsigned char)(255 * fadeProgress);
                if (fadeProgress > 0.5f) alpha = 255;
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, alpha });

                if (fadeProgress > 0.3f) {
                    const char* deathText = "You died!";
                    int textWidth = MeasureText(deathText, 48);
                    unsigned char textAlpha = (unsigned char)(255 * ((fadeProgress - 0.3f) / 0.7f));
                    DrawText(deathText, (screenWidth - textWidth) / 2, screenHeight / 2 - 24, 48, (Color){ 200, 0, 0, textAlpha });
                }
            }

            // Level up parchment banner
            if (levelUpNotif.active) {
                Color parchmentBg = { 222, 198, 158, 240 };
                Color parchmentBorder = { 139, 90, 43, 255 };
                Color parchmentDark = { 180, 150, 100, 255 };
                Color textColor = { 60, 40, 20, 255 };

                int bannerW = 400;
                int bannerH = 100;
                int bannerX = (screenWidth - bannerW) / 2;
                int bannerY = screenHeight - bannerH - 20;

                float alpha = 1.0f;
                if (levelUpNotif.timer > LEVEL_UP_DURATION - 0.3f) {
                    alpha = (LEVEL_UP_DURATION - levelUpNotif.timer) / 0.3f;
                } else if (levelUpNotif.timer < 0.5f) {
                    alpha = levelUpNotif.timer / 0.5f;
                }

                parchmentBg.a = (unsigned char)(240 * alpha);
                parchmentBorder.a = (unsigned char)(255 * alpha);
                parchmentDark.a = (unsigned char)(255 * alpha);
                textColor.a = (unsigned char)(255 * alpha);

                DrawRectangle(bannerX, bannerY, bannerW, bannerH, parchmentBg);

                for (int i = 0; i < 5; i++) {
                    int lineY = bannerY + 15 + i * 18;
                    DrawLine(bannerX + 10, lineY, bannerX + bannerW - 10, lineY, parchmentDark);
                }

                DrawRectangleLinesEx((Rectangle){(float)bannerX, (float)bannerY, (float)bannerW, (float)bannerH}, 3, parchmentBorder);
                DrawRectangleLinesEx((Rectangle){(float)bannerX + 5, (float)bannerY + 5, (float)bannerW - 10, (float)bannerH - 10}, 1, parchmentBorder);

                int cornerSize = 12;
                DrawRectangle(bannerX, bannerY, cornerSize, cornerSize, parchmentBorder);
                DrawRectangle(bannerX + bannerW - cornerSize, bannerY, cornerSize, cornerSize, parchmentBorder);
                DrawRectangle(bannerX, bannerY + bannerH - cornerSize, cornerSize, cornerSize, parchmentBorder);
                DrawRectangle(bannerX + bannerW - cornerSize, bannerY + bannerH - cornerSize, cornerSize, cornerSize, parchmentBorder);

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

    // Unload wall models and shaders
    for (int i = 0; i < wallCount; i++) {
        UnloadModel(wallModels[i]);
    }
    for (int i = 0; i < WALL_MATERIAL_COUNT; i++) {
        UnloadShader(wallShaders[i]);
    }

    // Cleanup audio
    UnloadSoundSystem();
    CloseAudioDevice();

    CloseWindow();
    return 0;
}
