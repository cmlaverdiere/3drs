#include "enemy_ai.h"
#include "math_utils.h"
#include "game_systems.h"
#include "sound_system.h"

static constexpr float TURN_SPEED = 5.0f;  // Radians per second

int UpdateEnemies(Enemy* enemies, int enemyCount,
                  const CustomMonster* customMonsters, int customMonsterCount,
                  Vector3 playerPos, bool playerDead,
                  DamageIndicator* damageIndicators, float dt) {
    int totalDamage = 0;

    for (int i = 0; i < enemyCount; i++) {
        // Get config values based on enemy type
        float attackRange, chaseSpeed, atkCooldown;
        int maxHit, maxHealth;

        if (enemies[i].customMonsterIndex >= 0 && enemies[i].customMonsterIndex < customMonsterCount) {
            const CustomMonster& cm = customMonsters[enemies[i].customMonsterIndex];
            attackRange = cm.attackRange;
            chaseSpeed = cm.chaseSpeed;
            atkCooldown = cm.attackCooldown;
            maxHit = cm.maxDamage;
            maxHealth = cm.maxHealth;
        } else {
            const EnemyConfig& config = ENEMY_CONFIGS[enemies[i].type];
            attackRange = config.attackRange;
            chaseSpeed = config.chaseSpeed;
            atkCooldown = config.attackCooldown;
            maxHit = config.maxHit;
            maxHealth = config.maxHealth;
        }

        if (enemies[i].attackCooldown > 0) {
            enemies[i].attackCooldown -= dt;
        }

        if (enemies[i].alive) {
            if (enemies[i].hostile && !playerDead) {
                // Chase behavior
                float dx = playerPos.x - enemies[i].position.x;
                float dz = playerPos.z - enemies[i].position.z;
                float horizDist = sqrtf(dx*dx + dz*dz);

                // Calculate enemy's actual Y position based on terrain
                float enemyY = GetTerrainHeight(enemies[i].position.x, enemies[i].position.z);
                float dy = playerPos.y - enemyY;
                float dist3D = sqrtf(dx*dx + dy*dy + dz*dz);

                // Turn to face player
                float targetAngle = atan2f(dx, dz);
                enemies[i].facingAngle = SmoothTurn(enemies[i].facingAngle, targetAngle, TURN_SPEED * dt);

                if (horizDist > attackRange) {
                    // Move toward player (use horizontal distance for movement)
                    float speed = chaseSpeed * dt;
                    enemies[i].position.x += (dx / horizDist) * speed;
                    enemies[i].position.z += (dz / horizDist) * speed;
                } else if (dist3D <= attackRange && enemies[i].attackCooldown <= 0) {
                    // Attack player (use 3D distance to prevent attacking through floors)
                    int damage = GetRandomValue(0, maxHit);
                    totalDamage += damage;
                    SpawnDamageIndicator(damageIndicators, playerPos, damage);
                    enemies[i].attackCooldown = atkCooldown;
                    if (damage > 0) PlaySoundEffect(SFX_PLAYER_HURT);
                }
            } else {
                // Wander behavior
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
                    // Turn to face wander target (slower)
                    float targetAngle = atan2f(dx, dz);
                    enemies[i].facingAngle = SmoothTurn(enemies[i].facingAngle, targetAngle, TURN_SPEED * 0.5f * dt);

                    float speed = 1.0f * dt;
                    enemies[i].position.x += (dx / dist) * speed;
                    enemies[i].position.z += (dz / dist) * speed;
                }
            }
        } else {
            // Dead - check respawn (custom monsters don't respawn)
            if (enemies[i].customMonsterIndex < 0) {
                enemies[i].respawnTimer -= dt;
                if (enemies[i].respawnTimer <= 0) {
                    enemies[i].position.x = enemies[i].spawnPoint.x + RandomFloat(-5.0f, 5.0f);
                    enemies[i].position.z = enemies[i].spawnPoint.z + RandomFloat(-5.0f, 5.0f);
                    enemies[i].position.y = 0.0f;
                    enemies[i].health = maxHealth;
                    enemies[i].alive = true;
                    enemies[i].wanderTimer = 0.0f;
                    enemies[i].hostile = false;
                    enemies[i].attackCooldown = 0.0f;
                    enemies[i].facingAngle = RandomFloat(0.0f, 2.0f * PI);
                }
            }
        }
    }

    return totalDamage;
}

void InitEnemy(Enemy* enemy, EnemyType type, Vector3 spawnPoint) {
    enemy->type = type;
    enemy->spawnPoint = spawnPoint;
    enemy->position = spawnPoint;
    enemy->health = ENEMY_CONFIGS[type].maxHealth;
    enemy->alive = true;
    enemy->respawnTimer = 0.0f;
    enemy->wanderTimer = 0.0f;
    enemy->wanderTarget = spawnPoint;
    enemy->hostile = false;
    enemy->attackCooldown = 0.0f;
    enemy->facingAngle = RandomFloat(0.0f, 2.0f * PI);
    enemy->customMonsterIndex = -1;  // Built-in enemy type, not a custom monster
}
