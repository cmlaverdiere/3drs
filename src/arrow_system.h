#ifndef ARROW_SYSTEM_H
#define ARROW_SYSTEM_H

#include "raylib.h"
#include "types.h"

// Arrow projectile constants
constexpr int MAX_ARROWS_IN_FLIGHT = 16;
constexpr float ARROW_SPEED = 35.0f;
constexpr float ARROW_GRAVITY = 15.0f;
constexpr float ARROW_LIFETIME = 5.0f;
constexpr float ARROW_HIT_RADIUS = 0.5f;
constexpr float ARROW_BREAK_CHANCE = 0.2f;  // 20% break on hit

// Bow draw mechanics
constexpr float BOW_MIN_DRAW_TIME = 0.2f;
constexpr float BOW_MAX_DRAW_TIME = 1.5f;
constexpr float BOW_DAMAGE_MIN_MULT = 0.3f;
constexpr float BOW_DAMAGE_MAX_MULT = 1.5f;

// Arrow projectile
struct ArrowProjectile {
    Vector3 position;
    Vector3 velocity;
    Vector3 direction;  // For rotation rendering
    float lifetime;
    bool active;
    int damage;  // Max damage this arrow can deal
};

// Arrow system (manages all in-flight arrows)
struct ArrowSystem {
    ArrowProjectile arrows[MAX_ARROWS_IN_FLIGHT];
};

// Bow draw state
struct BowState {
    bool isDrawing;
    float drawTime;
    float cooldown;
};

// Forward declarations
struct EntityModels;
struct BloodSplatterSystem;

// Initialize arrow system
void InitArrowSystem(ArrowSystem* system);

// Handle bow input (draw, release)
void HandleBowInput(Camera3D* camera, PlayerState* state, BowState* bow,
                    ArrowSystem* arrows, DamageIndicator* damageIndicators,
                    XPPopup* xpPopups, LevelUpNotification* levelUpNotif,
                    Enemy* enemies, int enemyCount,
                    WorldItem* worldItems, int* worldItemCount,
                    float dt);

// Fire an arrow
void FireArrow(Camera3D* camera, PlayerState* state, BowState* bow,
               ArrowSystem* arrows);

// Update all arrows (physics, collision)
void UpdateArrows(ArrowSystem* arrows, Enemy* enemies, int enemyCount,
                  const CustomMonster* customMonsters, int customMonsterCount,
                  WorldItem* worldItems, int* worldItemCount,
                  DamageIndicator* damageIndicators,
                  XPPopup* xpPopups, LevelUpNotification* levelUpNotif,
                  PlayerState* state, BloodSplatterSystem* bloodSystem, float dt);

// Draw all arrows in flight
void DrawArrows(const EntityModels* models, const ArrowSystem* arrows);

// Spawn arrow item on ground
void SpawnArrowItem(WorldItem* worldItems, int* worldItemCount, Vector3 pos);

// Get enemy dimensions for collision
float GetEnemyRadius(EnemyType type);
float GetEnemyHeight(EnemyType type);

#endif
