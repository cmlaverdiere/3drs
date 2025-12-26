#ifndef ENEMY_AI_H
#define ENEMY_AI_H

#include "types.h"

// Update all enemies (AI behavior, attacks, respawning)
// Returns damage dealt to player this frame (0 if none)
int UpdateEnemies(Enemy* enemies, int enemyCount,
                  const CustomMonster* customMonsters, int customMonsterCount,
                  Vector3 playerPos, bool playerDead,
                  DamageIndicator* damageIndicators, float dt);

// Initialize enemy from spawn data
void InitEnemy(Enemy* enemy, EnemyType type, Vector3 spawnPoint);

#endif
