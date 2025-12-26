#include "player.h"
#include "math_utils.h"
#include "collision.h"
#include "xp_system.h"

void UpdatePlayerMovement(Camera3D* camera, PlayerRuntime* runtime,
                          const Wall* walls, int wallCount,
                          const std::vector<int>& nearbyWalls, float dt) {
    // Toggle run mode with R key
    if (IsKeyPressed(KEY_R)) {
        ToggleRun(runtime);
    }

    // Auto-disable running if out of energy
    if (runtime->runEnergy <= 0.0f) {
        runtime->isRunning = false;
    }

    // Get forward and right vectors (horizontal only)
    Vector3 forward = {
        camera->target.x - camera->position.x,
        0,
        camera->target.z - camera->position.z
    };
    forward = Normalize3D(forward);
    Vector3 right = { -forward.z, 0, forward.x };

    // Calculate movement based on WASD input
    float moveSpeed = (runtime->isRunning && runtime->runEnergy > 0.0f) ? RUN_SPEED : WALK_SPEED;
    Vector3 movement = { 0, 0, 0 };
    bool isMoving = false;

    if (IsKeyDown(KEY_W)) { movement.x += forward.x; movement.z += forward.z; isMoving = true; }
    if (IsKeyDown(KEY_S)) { movement.x -= forward.x; movement.z -= forward.z; isMoving = true; }
    if (IsKeyDown(KEY_D)) { movement.x += right.x; movement.z += right.z; isMoving = true; }
    if (IsKeyDown(KEY_A)) { movement.x -= right.x; movement.z -= right.z; isMoving = true; }

    // Normalize diagonal movement and apply speed
    if (isMoving) {
        movement = Normalize3D(movement);
        camera->position.x += movement.x * moveSpeed * dt;
        camera->position.z += movement.z * moveSpeed * dt;
        camera->target.x += movement.x * moveSpeed * dt;
        camera->target.z += movement.z * moveSpeed * dt;
    }

    // Update energy
    UpdateRunEnergy(runtime, isMoving, dt);

    // Mouse look (pitch and yaw)
    Vector2 mouseDelta = GetMouseDelta();

    // Yaw (rotate around Y axis)
    float yaw = -mouseDelta.x * MOUSE_SENSITIVITY;
    Vector3 toTarget = {
        camera->target.x - camera->position.x,
        camera->target.y - camera->position.y,
        camera->target.z - camera->position.z
    };
    float cosYaw = cosf(yaw);
    float sinYaw = sinf(yaw);
    float newX = toTarget.x * cosYaw + toTarget.z * sinYaw;
    float newZ = -toTarget.x * sinYaw + toTarget.z * cosYaw;
    toTarget.x = newX;
    toTarget.z = newZ;

    // Pitch (rotate up/down, clamped)
    float pitch = mouseDelta.y * MOUSE_SENSITIVITY;
    float currentPitch = asinf(toTarget.y / sqrtf(toTarget.x*toTarget.x + toTarget.y*toTarget.y + toTarget.z*toTarget.z));
    float newPitch = currentPitch - pitch;
    newPitch = fmaxf(-1.4f, fminf(1.4f, newPitch));

    float horizDist = sqrtf(toTarget.x*toTarget.x + toTarget.z*toTarget.z);
    float totalDist = sqrtf(toTarget.x*toTarget.x + toTarget.y*toTarget.y + toTarget.z*toTarget.z);
    toTarget.y = sinf(newPitch) * totalDist;
    float newHorizDist = cosf(newPitch) * totalDist;
    if (horizDist > 0.001f) {
        float scale = newHorizDist / horizDist;
        toTarget.x *= scale;
        toTarget.z *= scale;
    }

    camera->target.x = camera->position.x + toTarget.x;
    camera->target.y = camera->position.y + toTarget.y;
    camera->target.z = camera->position.z + toTarget.z;

    // Wall collision
    float currentTerrainY = GetTerrainHeight(camera->position.x, camera->position.z);
    float playerFeetY = camera->position.y - PLAYER_EYE_HEIGHT;

    for (int idx : nearbyWalls) {
        float wallTerrainY = GetTerrainHeight(walls[idx].position.x, walls[idx].position.z);
        float wallTop = wallTerrainY + walls[idx].position.y + walls[idx].height;

        if (playerFeetY < wallTop - 0.1f) {
            Wall w = walls[idx];  // Make non-const copy for collision functions
            if (PointInWall(camera->position, w, PLAYER_RADIUS)) {
                Vector3 oldPos = camera->position;
                camera->position = ResolveWallCollision(camera->position, w, PLAYER_RADIUS);
                camera->target.x += camera->position.x - oldPos.x;
                camera->target.z += camera->position.z - oldPos.z;
            }
        }
    }

    // Jump input
    if (IsKeyPressed(KEY_SPACE) && !runtime->isJumping) {
        runtime->jumpVelocity = JUMP_FORCE;
        runtime->isJumping = true;
    }

    // Apply gravity and update jump
    if (runtime->isJumping) {
        runtime->jumpVelocity -= GRAVITY * dt;
        runtime->jumpHeight += runtime->jumpVelocity * dt;

        if (runtime->jumpHeight <= 0.0f) {
            runtime->jumpHeight = 0.0f;
            runtime->jumpVelocity = 0.0f;
            runtime->isJumping = false;
        }
    }

    // Apply terrain height + jump to player
    float terrainY = GetTerrainHeight(camera->position.x, camera->position.z);
    float groundY = GetGroundHeight(camera->position, walls, nearbyWalls);

    // Land on highest surface
    if (runtime->isJumping && runtime->jumpVelocity < 0) {
        float playerFeet = terrainY + runtime->jumpHeight;
        if (playerFeet <= groundY) {
            runtime->jumpHeight = groundY - terrainY;
            if (runtime->jumpHeight < 0.01f) runtime->jumpHeight = 0.0f;
            runtime->jumpVelocity = 0.0f;
            runtime->isJumping = false;
        }
    } else if (!runtime->isJumping) {
        runtime->jumpHeight = groundY - terrainY;
        if (runtime->jumpHeight < 0.01f) runtime->jumpHeight = 0.0f;
    }

    float newY = terrainY + PLAYER_EYE_HEIGHT + runtime->jumpHeight;
    float yDelta = newY - camera->position.y;
    camera->position.y = newY;
    camera->target.y += yDelta;
}

void UpdateRunEnergy(PlayerRuntime* runtime, bool isMoving, float dt) {
    if (runtime->isRunning && isMoving) {
        runtime->runEnergy -= ENERGY_DRAIN_RATE * dt;
        if (runtime->runEnergy < 0.0f) runtime->runEnergy = 0.0f;
    } else {
        runtime->runEnergy += ENERGY_REGEN_RATE * dt;
        if (runtime->runEnergy > 100.0f) runtime->runEnergy = 100.0f;
    }
}

void UpdateDuckAnimation(Camera3D* camera, PlayerRuntime* runtime, float dt) {
    float targetDuckOffset = 0.0f;
    if (runtime->isDucking) {
        runtime->duckTimer -= dt;
        if (runtime->duckTimer <= 0.0f) {
            runtime->isDucking = false;
            runtime->duckTimer = 0.0f;
        } else {
            float progress = 1.0f - (runtime->duckTimer / DUCK_DURATION);
            if (progress < 0.5f) {
                targetDuckOffset = DUCK_DEPTH * (progress * 2.0f);
            } else {
                targetDuckOffset = DUCK_DEPTH * ((1.0f - progress) * 2.0f);
            }
        }
    }

    float duckDelta = targetDuckOffset - runtime->currentDuckOffset;
    if (duckDelta != 0.0f) {
        camera->position.y -= duckDelta;
        camera->target.y -= duckDelta;
        runtime->currentDuckOffset = targetDuckOffset;
    }
}

void UpdateHPRegen(PlayerState* state, PlayerRuntime* runtime, float dt) {
    if (!runtime->isDead && state->currentHP < state->maxHP) {
        runtime->hpRegenTimer += dt;
        if (runtime->hpRegenTimer >= HP_REGEN_INTERVAL) {
            state->currentHP++;
            runtime->hpRegenTimer = 0.0f;
        }
    } else {
        runtime->hpRegenTimer = 0.0f;
    }
}

void UpdatePlayerDeath(Camera3D* camera, PlayerState* state, PlayerRuntime* runtime,
                       WorldItem* worldItems, int* worldItemCount,
                       Enemy* enemies, int enemyCount,
                       Vector3 spawnPoint, float dt) {
    if (!runtime->isDead) return;

    runtime->deathFadeTimer -= dt;

    // Drop items on first frame of death
    if (runtime->deathFadeTimer > DEATH_FADE_DURATION - dt - 0.01f) {
        for (int i = 0; i < INV_SLOTS; i++) {
            if (state->inventory[i] != ITEM_NONE && *worldItemCount < MAX_WORLD_ITEMS) {
                int dropCount = state->inventoryCount[i];
                // Drop as a single stacked item
                worldItems[*worldItemCount].type = state->inventory[i];
                worldItems[*worldItemCount].position = runtime->deathPosition;
                worldItems[*worldItemCount].position.x += RandomFloat(-1.0f, 1.0f);
                worldItems[*worldItemCount].position.z += RandomFloat(-1.0f, 1.0f);
                worldItems[*worldItemCount].position.y = 0.0f;
                worldItems[*worldItemCount].pickedUp = false;
                worldItems[*worldItemCount].canRespawn = false;  // Death drops don't respawn
                worldItems[*worldItemCount].respawnTimer = 0.0f;
                worldItems[*worldItemCount].quantity = dropCount;
                (*worldItemCount)++;

                state->inventory[i] = ITEM_NONE;
                state->inventoryCount[i] = 0;
            }
        }
        state->equippedWeapon = ITEM_NONE;
    }

    // Respawn
    if (runtime->deathFadeTimer <= 0) {
        runtime->isDead = false;
        camera->position = spawnPoint;
        camera->target = (Vector3){ spawnPoint.x, spawnPoint.y, spawnPoint.z + 1.0f };
        int combatLevel = GetLevelFromXP(state->skillXP[SKILL_COMBAT]);
        state->maxHP = GetMaxHitpoints(combatLevel);
        state->currentHP = state->maxHP;
        for (int i = 0; i < enemyCount; i++) {
            enemies[i].hostile = false;
        }
    }
}

void ToggleRun(PlayerRuntime* runtime) {
    runtime->isRunning = !runtime->isRunning;
}

float GetGroundHeight(Vector3 pos, const Wall* walls, const std::vector<int>& nearbyWalls) {
    float terrainY = GetTerrainHeight(pos.x, pos.z);
    float groundY = terrainY;

    for (int idx : nearbyWalls) {
        float wallTerrainY = GetTerrainHeight(walls[idx].position.x, walls[idx].position.z);
        float wallTop = wallTerrainY + walls[idx].position.y + walls[idx].height;
        float halfW = walls[idx].width / 2.0f;
        float halfD = walls[idx].depth / 2.0f;

        if (pos.x >= walls[idx].position.x - halfW &&
            pos.x <= walls[idx].position.x + halfW &&
            pos.z >= walls[idx].position.z - halfD &&
            pos.z <= walls[idx].position.z + halfD) {
            if (wallTop > groundY) {
                groundY = wallTop;
            }
        }
    }

    return groundY;
}
