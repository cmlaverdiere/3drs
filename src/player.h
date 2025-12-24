#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "types.h"
#include <vector>

// Player runtime state (separate from saved PlayerState)
struct PlayerRuntime {
    bool isRunning = false;
    float runEnergy = 100.0f;
    bool isJumping = false;
    float jumpVelocity = 0.0f;
    float jumpHeight = 0.0f;
    bool isDucking = false;
    float duckTimer = 0.0f;
    float currentDuckOffset = 0.0f;
    bool isDead = false;
    float deathFadeTimer = 0.0f;
    Vector3 deathPosition = {0, 0, 0};
    float hpRegenTimer = 0.0f;
};

// Update player movement (WASD, mouse look, jumping)
void UpdatePlayerMovement(Camera3D* camera, PlayerRuntime* runtime,
                          const Wall* walls, int wallCount,
                          const std::vector<int>& nearbyWalls, float dt);

// Update run energy (drain while running, regen otherwise)
void UpdateRunEnergy(PlayerRuntime* runtime, bool isMoving, float dt);

// Update ducking animation
void UpdateDuckAnimation(Camera3D* camera, PlayerRuntime* runtime, float dt);

// Update HP regeneration
void UpdateHPRegen(PlayerState* state, PlayerRuntime* runtime, float dt);

// Handle player death (fade, drop items, respawn)
void UpdatePlayerDeath(Camera3D* camera, PlayerState* state, PlayerRuntime* runtime,
                       WorldItem* worldItems, int* worldItemCount,
                       Enemy* enemies, int enemyCount,
                       Vector3 spawnPoint, float dt);

// Toggle run mode
void ToggleRun(PlayerRuntime* runtime);

// Check if player can stand on wall top (for platforms)
float GetGroundHeight(Vector3 pos, const Wall* walls, const std::vector<int>& nearbyWalls);

#endif
