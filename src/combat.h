#ifndef COMBAT_H
#define COMBAT_H

#include "types.h"

// Process player attack (handles both tree chopping and enemy combat)
// Returns true if attack was performed
// outMessage is set to a status message if needed (e.g., level requirement)
bool ProcessPlayerAttack(Camera3D* camera, PlayerState* state,
                         Enemy* enemies, int enemyCount,
                         Tree* trees, int treeCount,
                         WorldItem* worldItems, int* worldItemCount,
                         DamageIndicator* damageIndicators,
                         XPPopup* xpPopups,
                         LevelUpNotification* levelUpNotif,
                         float* swingTimer,
                         const char** outMessage);

// Award XP to a skill and handle level up notification
// Returns true if leveled up
bool AwardSkillXP(PlayerState* state, int skillIndex, int amount,
                  XPPopup* xpPopups, LevelUpNotification* levelUpNotif);

// Update trees (respawning)
void UpdateTrees(Tree* trees, int treeCount, float dt);

#endif
