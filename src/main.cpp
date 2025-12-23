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
    for (int i = 0; i < SKILL_COUNT; i++) {
        playerState.skillXP[i] = 0;
    }
    playerState.skillXP[SKILL_HITPOINTS] = XP_TABLE[9];
    for (int i = 0; i < INV_SLOTS; i++) {
        playerState.inventory[i] = ITEM_NONE;
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

        // Update trolls
        for (int i = 0; i < trollCount; i++) {
            if (trolls[i].attackCooldown > 0) {
                trolls[i].attackCooldown -= dt;
            }

            if (trolls[i].alive) {
                if (trolls[i].hostile && !playerDead) {
                    float dx = camera.position.x - trolls[i].position.x;
                    float dz = camera.position.z - trolls[i].position.z;
                    float dist = sqrtf(dx*dx + dz*dz);

                    if (dist > TROLL_ATTACK_RANGE) {
                        float speed = TROLL_CHASE_SPEED * dt;
                        trolls[i].position.x += (dx / dist) * speed;
                        trolls[i].position.z += (dz / dist) * speed;
                    } else if (trolls[i].attackCooldown <= 0) {
                        int damage = GetRandomValue(0, TROLL_MAX_HIT);
                        playerState.currentHP -= damage;
                        SpawnDamageIndicator(damageIndicators, camera.position, damage);
                        trolls[i].attackCooldown = TROLL_ATTACK_COOLDOWN;

                        if (playerState.currentHP <= 0) {
                            playerState.currentHP = 0;
                            playerDead = true;
                            deathFadeTimer = DEATH_FADE_DURATION;
                            deathPosition = camera.position;
                        }
                    }
                } else {
                    trolls[i].wanderTimer -= dt;
                    if (trolls[i].wanderTimer <= 0) {
                        trolls[i].wanderTarget.x = trolls[i].spawnPoint.x + RandomFloat(-3.0f, 3.0f);
                        trolls[i].wanderTarget.z = trolls[i].spawnPoint.z + RandomFloat(-3.0f, 3.0f);
                        trolls[i].wanderTimer = RandomFloat(2.0f, 5.0f);
                    }

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
                trolls[i].respawnTimer -= dt;
                if (trolls[i].respawnTimer <= 0) {
                    trolls[i].position.x = trolls[i].spawnPoint.x + RandomFloat(-5.0f, 5.0f);
                    trolls[i].position.z = trolls[i].spawnPoint.z + RandomFloat(-5.0f, 5.0f);
                    trolls[i].position.y = 0.0f;
                    trolls[i].health = TROLL_MAX_HEALTH;
                    trolls[i].alive = true;
                    trolls[i].wanderTimer = 0.0f;
                    trolls[i].hostile = false;
                    trolls[i].attackCooldown = 0.0f;
                }
            }
        }

        // Attack with mouse click
        if (!mouseMode && !playerDead && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && attackCooldown <= 0 && playerState.equippedWeapon != ITEM_NONE) {
            swingTimer = SWING_DURATION;
            attackCooldown = ATTACK_COOLDOWN;

            int combatLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
            int maxHit = CalculateMaxHit(combatLevel);

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
                target->hostile = true;

                int damage = RollDamage(maxHit);
                target->health -= damage;
                SpawnDamageIndicator(damageIndicators, target->position, damage);

                if (target->health <= 0) {
                    target->alive = false;
                    target->respawnTimer = TROLL_RESPAWN_TIME;

                    int xpGain = TROLL_MAX_HEALTH * 4;

                    int oldLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
                    playerState.skillXP[SKILL_COMBAT] += xpGain;
                    int newLevel = GetLevelFromXP(playerState.skillXP[SKILL_COMBAT]);
                    SpawnXPPopup(xpPopups, xpGain, SKILL_COMBAT);

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
                    if (playerState.inventory[i] != ITEM_NONE && worldItemCount < MAX_WORLD_ITEMS) {
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
                playerState.equippedWeapon = ITEM_NONE;
            }

            if (deathFadeTimer <= 0) {
                playerDead = false;
                camera.position = mapData.playerSpawn;
                camera.target = (Vector3){ mapData.playerSpawn.x, mapData.playerSpawn.y, mapData.playerSpawn.z + 1.0f };
                playerState.maxHP = GetLevelFromXP(playerState.skillXP[SKILL_HITPOINTS]);
                playerState.currentHP = playerState.maxHP;
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
                    break;
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

                for (int i = 0; i < worldItemCount; i++) {
                    if (!worldItems[i].pickedUp) {
                        if (worldItems[i].type == ITEM_BRONZE_SHORTSWORD) {
                            Color bronzeBlade = { 205, 127, 50, 255 };
                            Color bronzeHandle = { 139, 90, 43, 255 };
                            DrawSword(worldItems[i].position, bronzeBlade, bronzeHandle);
                        }
                    }
                }

                for (int i = 0; i < trollCount; i++) {
                    if (trolls[i].alive) {
                        float dist = Distance3D(camera.position, trolls[i].position);
                        bool inRange = (dist <= ATTACK_RANGE) && IsFacing(camera, trolls[i].position);
                        DrawTroll(trolls[i].position, inRange);
                    }
                }

                for (int i = 0; i < wallCount; i++) {
                    Vector3 pos = walls[i].position;
                    pos.y += walls[i].height / 2.0f;
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
                int cdWidth = (int)(100 * (attackCooldown / ATTACK_COOLDOWN));
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

            // Draw troll health bars
            for (int i = 0; i < trollCount; i++) {
                if (trolls[i].alive) {
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
                    if (Dot3D(toTroll, camForward) <= 0) continue;

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
    CloseWindow();
    return 0;
}
