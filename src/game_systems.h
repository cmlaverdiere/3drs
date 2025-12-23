#ifndef GAME_SYSTEMS_H
#define GAME_SYSTEMS_H

#include "types.h"

// Spawn a damage indicator
void SpawnDamageIndicator(DamageIndicator* indicators, Vector3 pos, int damage);

// Spawn an XP popup
void SpawnXPPopup(XPPopup* popups, int xpAmount, int skillIndex);

// Spawn drops from an enemy death
void SpawnEnemyDrops(const EnemyConfig& config, Vector3 pos, WorldItem* worldItems, int& worldItemCount);

#endif
