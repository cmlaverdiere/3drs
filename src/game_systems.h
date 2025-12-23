#ifndef GAME_SYSTEMS_H
#define GAME_SYSTEMS_H

#include "types.h"

// Spawn a damage indicator
void SpawnDamageIndicator(DamageIndicator* indicators, Vector3 pos, int damage);

// Spawn an XP popup
void SpawnXPPopup(XPPopup* popups, int xpAmount, int skillIndex);

#endif
