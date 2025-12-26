#include "arrow_system.h"
#include "math_utils.h"
#include "xp_system.h"
#include "game_systems.h"
#include "inventory.h"
#include "rendering.h"
#include "game_init.h"
#include "sound_system.h"
#include "rlgl.h"
#include <cmath>

void InitArrowSystem(ArrowSystem* system) {
    for (int i = 0; i < MAX_ARROWS_IN_FLIGHT; i++) {
        system->arrows[i].active = false;
    }
}

void HandleBowInput(Camera3D* camera, PlayerState* state, BowState* bow,
                    ArrowSystem* arrows, DamageIndicator* damageIndicators,
                    XPPopup* xpPopups, LevelUpNotification* levelUpNotif,
                    Enemy* enemies, int enemyCount,
                    WorldItem* worldItems, int* worldItemCount,
                    float dt) {
    // Update cooldown
    if (bow->cooldown > 0) {
        bow->cooldown -= dt;
    }

    // Check for arrows in inventory
    bool hasArrows = HasItem(state, ITEM_ARROW);

    // Start drawing
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && bow->cooldown <= 0 && hasArrows && !bow->isDrawing) {
        bow->isDrawing = true;
        bow->drawTime = 0.0f;
        PlaySoundEffect(SFX_SWING_HEAVY);  // Use heavy swing as bow draw sound
    }

    // Continue drawing
    if (bow->isDrawing && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        bow->drawTime += dt;
        if (bow->drawTime > BOW_MAX_DRAW_TIME) {
            bow->drawTime = BOW_MAX_DRAW_TIME;
        }
    }

    // Release arrow
    if (bow->isDrawing && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        bow->isDrawing = false;

        if (bow->drawTime >= BOW_MIN_DRAW_TIME && hasArrows) {
            FireArrow(camera, state, bow, arrows);
            bow->cooldown = BOW_ATTACK_COOLDOWN;
        }
        bow->drawTime = 0.0f;
    }

    // Cancel if released too early or lost arrows
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        bow->isDrawing = false;
    }
}

void FireArrow(Camera3D* camera, PlayerState* state, BowState* bow,
               ArrowSystem* arrows) {
    // Find free arrow slot
    int slot = -1;
    for (int i = 0; i < MAX_ARROWS_IN_FLIGHT; i++) {
        if (!arrows->arrows[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;  // All slots full

    // Consume arrow from inventory
    RemoveItem(state, ITEM_ARROW);

    // Calculate damage based on draw time
    float drawRatio = (bow->drawTime - BOW_MIN_DRAW_TIME) /
                      (BOW_MAX_DRAW_TIME - BOW_MIN_DRAW_TIME);
    if (drawRatio < 0.0f) drawRatio = 0.0f;
    if (drawRatio > 1.0f) drawRatio = 1.0f;
    float damageMult = BOW_DAMAGE_MIN_MULT + drawRatio * (BOW_DAMAGE_MAX_MULT - BOW_DAMAGE_MIN_MULT);

    int rangedLevel = GetLevelFromXP(state->skillXP[SKILL_RANGED]);
    int maxHit = CalculateMaxHit(rangedLevel);
    int arrowDamage = (int)(maxHit * damageMult * GetWeaponDamageMultiplier(ITEM_BOW));
    if (arrowDamage < 1) arrowDamage = 1;

    // Calculate direction from camera
    Vector3 direction = {
        camera->target.x - camera->position.x,
        camera->target.y - camera->position.y,
        camera->target.z - camera->position.z
    };
    direction = Normalize3D(direction);

    // Spawn position (slightly in front of camera)
    Vector3 spawnPos = {
        camera->position.x + direction.x * 0.5f,
        camera->position.y + direction.y * 0.5f,
        camera->position.z + direction.z * 0.5f
    };

    // Initialize arrow
    ArrowProjectile* arrow = &arrows->arrows[slot];
    arrow->active = true;
    arrow->position = spawnPos;
    arrow->direction = direction;

    // Apply speed based on draw time (more draw = much faster/further)
    // Min draw: 40% speed, Max draw: 140% speed (3.5x difference)
    float speedMult = 0.4f + 1.0f * drawRatio;
    arrow->velocity = {
        direction.x * ARROW_SPEED * speedMult,
        direction.y * ARROW_SPEED * speedMult,
        direction.z * ARROW_SPEED * speedMult
    };
    arrow->lifetime = ARROW_LIFETIME;
    arrow->damage = arrowDamage;

    PlaySoundEffect(SFX_MISS);  // Whoosh sound for arrow release
}

void UpdateArrows(ArrowSystem* arrows, Enemy* enemies, int enemyCount,
                  WorldItem* worldItems, int* worldItemCount,
                  DamageIndicator* damageIndicators,
                  XPPopup* xpPopups, LevelUpNotification* levelUpNotif,
                  PlayerState* state, BloodSplatterSystem* bloodSystem, float dt) {
    for (int i = 0; i < MAX_ARROWS_IN_FLIGHT; i++) {
        ArrowProjectile* arrow = &arrows->arrows[i];
        if (!arrow->active) continue;

        // Apply gravity
        arrow->velocity.y -= ARROW_GRAVITY * dt;

        // Update position
        arrow->position.x += arrow->velocity.x * dt;
        arrow->position.y += arrow->velocity.y * dt;
        arrow->position.z += arrow->velocity.z * dt;

        // Update direction to match velocity (for rotation)
        float velLen = sqrtf(arrow->velocity.x * arrow->velocity.x +
                             arrow->velocity.y * arrow->velocity.y +
                             arrow->velocity.z * arrow->velocity.z);
        if (velLen > 0.1f) {
            arrow->direction = {
                arrow->velocity.x / velLen,
                arrow->velocity.y / velLen,
                arrow->velocity.z / velLen
            };
        }

        // Check lifetime
        arrow->lifetime -= dt;
        if (arrow->lifetime <= 0) {
            arrow->active = false;
            continue;
        }

        // Ground collision
        float groundY = GetTerrainHeight(arrow->position.x, arrow->position.z);
        if (arrow->position.y < groundY + 0.1f) {
            arrow->active = false;
            // 80% chance arrow drops on ground
            if (RandomFloat(0.0f, 1.0f) > ARROW_BREAK_CHANCE) {
                SpawnArrowItem(worldItems, worldItemCount, arrow->position);
            }
            PlaySoundEffect(SFX_HIT);  // Thud sound
            continue;
        }

        // Enemy collision
        for (int e = 0; e < enemyCount; e++) {
            if (!enemies[e].alive) continue;

            Vector3 enemyPos = enemies[e].position;
            float enemyGroundY = GetTerrainHeight(enemyPos.x, enemyPos.z);

            float enemyRadius = GetEnemyRadius(enemies[e].type);
            float enemyHeight = GetEnemyHeight(enemies[e].type);

            // Horizontal distance check
            float dx = arrow->position.x - enemyPos.x;
            float dz = arrow->position.z - enemyPos.z;
            float horizDist = sqrtf(dx * dx + dz * dz);

            // Vertical check
            float dy = arrow->position.y - enemyGroundY;

            if (horizDist < enemyRadius + ARROW_HIT_RADIUS &&
                dy > 0 && dy < enemyHeight) {
                // Hit!
                const EnemyConfig& config = ENEMY_CONFIGS[enemies[e].type];
                enemies[e].hostile = true;

                int damage = RollDamage(arrow->damage);
                enemies[e].health -= damage;

                Vector3 dmgPos = enemyPos;
                dmgPos.y = enemyGroundY + enemyHeight * 0.7f;
                SpawnDamageIndicator(damageIndicators, dmgPos, damage);

                if (damage > 0) {
                    PlaySoundEffect(SFX_HIT);
                    // Spawn blood splatter at hit position
                    if (bloodSystem) {
                        Vector3 bloodPos = arrow->position;
                        SpawnBloodSplatter(bloodSystem, bloodPos, arrow->direction);
                    }
                }

                // 80% chance arrow drops near enemy, 20% breaks
                if (RandomFloat(0.0f, 1.0f) > ARROW_BREAK_CHANCE) {
                    Vector3 dropPos = enemyPos;
                    dropPos.x += RandomFloat(-0.5f, 0.5f);
                    dropPos.z += RandomFloat(-0.5f, 0.5f);
                    SpawnArrowItem(worldItems, worldItemCount, dropPos);
                }

                if (enemies[e].health <= 0) {
                    enemies[e].alive = false;
                    enemies[e].respawnTimer = config.respawnTime;
                    PlaySoundEffect(SFX_ENEMY_DEATH);

                    SpawnEnemyDrops(config, enemyPos, worldItems, *worldItemCount);

                    // Award Ranged XP (4 XP per hitpoint, like combat)
                    int xpGain = config.maxHealth * 4;
                    state->skillXP[SKILL_RANGED] += xpGain;
                    SpawnXPPopup(xpPopups, xpGain, SKILL_RANGED);

                    // Check for level up
                    int oldLevel = GetLevelFromXP(state->skillXP[SKILL_RANGED] - xpGain);
                    int newLevel = GetLevelFromXP(state->skillXP[SKILL_RANGED]);
                    if (newLevel > oldLevel && levelUpNotif) {
                        levelUpNotif->skillIndex = SKILL_RANGED;
                        levelUpNotif->newLevel = newLevel;
                        levelUpNotif->timer = LEVEL_UP_DURATION;
                        levelUpNotif->active = true;
                        PlaySoundEffect(SFX_LEVEL_UP);
                    }
                }

                arrow->active = false;
                break;
            }
        }
    }
}

void DrawArrows(const EntityModels* models, const ArrowSystem* arrows) {
    Color shaftColor = { 160, 140, 100, 255 };
    Color tipColor = { 100, 100, 110, 255 };
    Color fletchColor = { 200, 50, 50, 255 };

    for (int i = 0; i < MAX_ARROWS_IN_FLIGHT; i++) {
        const ArrowProjectile* arrow = &arrows->arrows[i];
        if (!arrow->active) continue;

        // Calculate rotation from direction
        float yaw = atan2f(arrow->direction.x, arrow->direction.z);
        float pitch = asinf(-arrow->direction.y);

        rlPushMatrix();
        rlTranslatef(arrow->position.x, arrow->position.y, arrow->position.z);
        rlRotatef(yaw * RAD2DEG, 0, 1, 0);
        rlRotatef(pitch * RAD2DEG, 1, 0, 0);

        // Arrow shaft (pointing +Z direction)
        DrawModelCube(models, (Vector3){0, 0, 0}, 0.03f, 0.03f, 0.5f, shaftColor);
        // Arrowhead
        DrawModelCube(models, (Vector3){0, 0, 0.28f}, 0.06f, 0.03f, 0.08f, tipColor);
        // Fletching
        DrawModelCube(models, (Vector3){0.025f, 0, -0.2f}, 0.04f, 0.01f, 0.08f, fletchColor);
        DrawModelCube(models, (Vector3){-0.025f, 0, -0.2f}, 0.04f, 0.01f, 0.08f, fletchColor);

        rlPopMatrix();
    }
}

void SpawnArrowItem(WorldItem* worldItems, int* worldItemCount, Vector3 pos) {
    if (*worldItemCount >= MAX_WORLD_ITEMS) return;

    worldItems[*worldItemCount].type = ITEM_ARROW;
    worldItems[*worldItemCount].position = pos;
    // Y is relative to terrain - rendering adds terrain height
    worldItems[*worldItemCount].position.y = 0.0f;
    worldItems[*worldItemCount].pickedUp = false;
    worldItems[*worldItemCount].canRespawn = false;
    worldItems[*worldItemCount].respawnTimer = 0.0f;
    (*worldItemCount)++;
}

float GetEnemyRadius(EnemyType type) {
    switch (type) {
        case ENEMY_TROLL: return 0.6f;
        case ENEMY_COW: return 0.7f;
        case ENEMY_SCORPION: return 0.5f;
        case ENEMY_BANDIT: return 0.5f;
        case ENEMY_SAND_GOLEM: return 0.8f;
        case ENEMY_DEMON: return 0.7f;
        case ENEMY_DRAGON: return 1.5f;
        default: return 0.5f;
    }
}

float GetEnemyHeight(EnemyType type) {
    switch (type) {
        case ENEMY_TROLL: return 2.0f;
        case ENEMY_COW: return 1.4f;
        case ENEMY_SCORPION: return 0.6f;
        case ENEMY_BANDIT: return 2.0f;
        case ENEMY_SAND_GOLEM: return 2.5f;
        case ENEMY_DEMON: return 2.5f;
        case ENEMY_DRAGON: return 3.5f;
        default: return 2.0f;
    }
}
