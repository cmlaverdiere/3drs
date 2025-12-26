#ifndef COMBAT_H
#define COMBAT_H

#include "types.h"

// Forward declaration
struct LeafBurstSystem;

// Process player attack (handles tree chopping, rock mining, and enemy combat)
// Returns true if attack was performed
// outMessage is set to a status message if needed (e.g., level requirement)
// leafBurstSystem can be null if not in autumn
bool ProcessPlayerAttack(Camera3D* camera, PlayerState* state,
                         Enemy* enemies, int enemyCount,
                         Tree* trees, int treeCount,
                         Rock* rocks, int rockCount,
                         WorldItem* worldItems, int* worldItemCount,
                         DamageIndicator* damageIndicators,
                         XPPopup* xpPopups,
                         LevelUpNotification* levelUpNotif,
                         float* swingTimer,
                         const char** outMessage,
                         LeafBurstSystem* leafBurstSystem);

// Award XP to a skill and handle level up notification
// Returns true if leveled up
bool AwardSkillXP(PlayerState* state, int skillIndex, int amount,
                  XPPopup* xpPopups, LevelUpNotification* levelUpNotif);

// Update trees (respawning)
void UpdateTrees(Tree* trees, int treeCount, float dt);

// Update rocks (respawning)
void UpdateRocks(Rock* rocks, int rockCount, float dt);

#endif
