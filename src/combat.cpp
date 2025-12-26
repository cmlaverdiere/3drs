#include "combat.h"
#include "math_utils.h"
#include "xp_system.h"
#include "game_systems.h"
#include "game_init.h"
#include "sound_system.h"

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
                         LeafBurstSystem* leafBurstSystem,
                         BloodSplatterSystem* bloodSystem) {
    if (state->equippedWeapon == ITEM_NONE) return false;

    *swingTimer = SWING_DURATION;
    bool actionTaken = false;
    if (outMessage) *outMessage = nullptr;

    // Play swing sound based on weapon type
    if (state->equippedWeapon == ITEM_IRON_2H_SWORD) {
        PlaySoundEffect(SFX_SWING_HEAVY);
    } else if (state->equippedWeapon == ITEM_STEEL_SCIMITAR ||
               state->equippedWeapon == ITEM_MITHRIL_SCIMITAR ||
               state->equippedWeapon == ITEM_ADAMANT_SCIMITAR) {
        // Light whoosh for fast scimitar swings
        PlaySoundEffect(SFX_MISS);
    }

    // If wielding axe, check for trees first
    if (state->equippedWeapon == ITEM_BRONZE_AXE) {
        Tree* targetTree = nullptr;
        float closestTreeDist = CHOP_RANGE + 1.0f;

        for (int i = 0; i < treeCount; i++) {
            if (trees[i].alive) {
                Vector3 treePos = trees[i].position;
                treePos.y = GetTerrainHeight(treePos.x, treePos.z);
                float dist = Distance3D(camera->position, treePos);
                if (dist <= CHOP_RANGE && dist < closestTreeDist && IsFacing(*camera, treePos)) {
                    targetTree = &trees[i];
                    closestTreeDist = dist;
                }
            }
        }

        if (targetTree != nullptr) {
            // Check level requirement for oak trees
            if (targetTree->type == TREE_OAK) {
                int woodcuttingLevel = GetLevelFromXP(state->skillXP[SKILL_WOODCUTTING]);
                if (woodcuttingLevel < OAK_TREE_LEVEL) {
                    if (outMessage) *outMessage = "You need level 10 Woodcutting to chop oak trees.";
                    PlaySoundEffect(SFX_MISS);
                    return true;  // Swing happened, but no damage
                }
            }

            actionTaken = true;
            targetTree->health--;
            PlaySoundEffect(SFX_HIT);

            // Spawn leaf burst in autumn when chopping trees
            if (leafBurstSystem != nullptr && g_currentSeason == SEASON_AUTUMN) {
                float treeHeight = (targetTree->type == TREE_OAK) ? 7.0f : 5.0f;
                SpawnLeafBurst(leafBurstSystem, targetTree->position, treeHeight);
            }

            if (targetTree->health <= 0) {
                targetTree->alive = false;
                targetTree->respawnTimer = TREE_RESPAWN_TIME;

                // Spawn logs (oak logs for oak trees)
                if (*worldItemCount < MAX_WORLD_ITEMS) {
                    worldItems[*worldItemCount].type =
                        (targetTree->type == TREE_OAK) ? ITEM_OAK_LOGS : ITEM_LOGS;
                    worldItems[*worldItemCount].position = targetTree->position;
                    worldItems[*worldItemCount].position.x += RandomFloat(-0.5f, 0.5f);
                    worldItems[*worldItemCount].position.z += RandomFloat(-0.5f, 0.5f);
                    worldItems[*worldItemCount].position.y = 0.0f;
                    worldItems[*worldItemCount].pickedUp = false;
                    worldItems[*worldItemCount].canRespawn = false;  // Logs don't respawn
                    worldItems[*worldItemCount].respawnTimer = 0.0f;
                    (*worldItemCount)++;
                }

                // Award XP (more for oak trees)
                int xp = (targetTree->type == TREE_OAK) ? OAK_WOODCUTTING_XP : WOODCUTTING_XP;
                AwardSkillXP(state, SKILL_WOODCUTTING, xp, xpPopups, levelUpNotif);
            }
        }
    }

    // If wielding pickaxe, check for rocks
    if (!actionTaken && state->equippedWeapon == ITEM_BRONZE_PICKAXE) {
        Rock* targetRock = nullptr;
        float closestRockDist = MINE_RANGE + 1.0f;

        for (int i = 0; i < rockCount; i++) {
            if (rocks[i].alive) {
                Vector3 rockPos = rocks[i].position;
                rockPos.y = GetTerrainHeight(rockPos.x, rockPos.z);
                float dist = Distance3D(camera->position, rockPos);
                if (dist <= MINE_RANGE && dist < closestRockDist && IsFacing(*camera, rockPos)) {
                    targetRock = &rocks[i];
                    closestRockDist = dist;
                }
            }
        }

        if (targetRock != nullptr) {
            actionTaken = true;
            targetRock->health--;
            PlaySoundEffect(SFX_HIT);

            if (targetRock->health <= 0) {
                targetRock->alive = false;
                targetRock->respawnTimer = ROCK_RESPAWN_TIME;

                // Spawn ore based on rock type
                if (*worldItemCount < MAX_WORLD_ITEMS) {
                    worldItems[*worldItemCount].type =
                        (targetRock->type == ROCK_COPPER) ? ITEM_COPPER_ORE : ITEM_TIN_ORE;
                    worldItems[*worldItemCount].position = targetRock->position;
                    worldItems[*worldItemCount].position.x += RandomFloat(-0.3f, 0.3f);
                    worldItems[*worldItemCount].position.z += RandomFloat(-0.3f, 0.3f);
                    worldItems[*worldItemCount].position.y = 0.0f;
                    worldItems[*worldItemCount].pickedUp = false;
                    worldItems[*worldItemCount].canRespawn = false;  // Ore doesn't respawn as ground item
                    worldItems[*worldItemCount].respawnTimer = 0.0f;
                    (*worldItemCount)++;
                }

                // Award Mining XP
                AwardSkillXP(state, SKILL_MINING, MINING_XP, xpPopups, levelUpNotif);
            }
        }
    }

    // If no tree was chopped or rock was mined, try attacking enemies
    if (!actionTaken) {
        int combatLevel = GetLevelFromXP(state->skillXP[SKILL_COMBAT]);
        int maxHit = CalculateMaxHit(combatLevel);

        Enemy* target = nullptr;
        float closestDist = PLAYER_ATTACK_RANGE + 1.0f;

        for (int i = 0; i < enemyCount; i++) {
            if (enemies[i].alive) {
                Vector3 enemyPos = enemies[i].position;
                enemyPos.y = GetTerrainHeight(enemyPos.x, enemyPos.z);
                float dist = Distance3D(camera->position, enemyPos);
                if (dist <= PLAYER_ATTACK_RANGE && dist < closestDist && IsFacing(*camera, enemyPos)) {
                    target = &enemies[i];
                    closestDist = dist;
                }
            }
        }

        if (target != nullptr) {
            const EnemyConfig& config = ENEMY_CONFIGS[target->type];
            target->hostile = true;

            int damage = (int)(RollDamage(maxHit) * GetWeaponDamageMultiplier(state->equippedWeapon));
            target->health -= damage;
            Vector3 dmgPos = target->position;
            dmgPos.y = GetTerrainHeight(dmgPos.x, dmgPos.z);
            SpawnDamageIndicator(damageIndicators, dmgPos, damage);

            if (damage > 0) {
                PlaySoundEffect(SFX_HIT);
                // Spawn blood splatter at enemy position
                if (bloodSystem) {
                    Vector3 bloodPos = target->position;
                    bloodPos.y = GetTerrainHeight(bloodPos.x, bloodPos.z) + 1.0f;  // Hit height
                    Vector3 hitDir = {
                        target->position.x - camera->position.x,
                        0.0f,
                        target->position.z - camera->position.z
                    };
                    SpawnBloodSplatter(bloodSystem, bloodPos, hitDir);
                }
            } else {
                PlaySoundEffect(SFX_MISS);
            }

            if (target->health <= 0) {
                target->alive = false;
                target->respawnTimer = config.respawnTime;
                PlaySoundEffect(SFX_ENEMY_DEATH);

                SpawnEnemyDrops(config, target->position, worldItems, *worldItemCount);

                // Award XP for kill (4 XP per hitpoint, like OSRS)
                int xpGain = config.maxHealth * 4;
                AwardSkillXP(state, SKILL_COMBAT, xpGain, xpPopups, levelUpNotif);
            }
        }
    }

    return true;
}

bool AwardSkillXP(PlayerState* state, int skillIndex, int amount,
                  XPPopup* xpPopups, LevelUpNotification* levelUpNotif) {
    int oldLevel = GetLevelFromXP(state->skillXP[skillIndex]);
    state->skillXP[skillIndex] += amount;
    int newLevel = GetLevelFromXP(state->skillXP[skillIndex]);

    SpawnXPPopup(xpPopups, amount, skillIndex);
    PlaySoundEffect(SFX_XP_GAIN);

    if (newLevel > oldLevel) {
        levelUpNotif->skillIndex = skillIndex;
        levelUpNotif->newLevel = newLevel;
        levelUpNotif->timer = LEVEL_UP_DURATION;
        levelUpNotif->active = true;
        PlaySoundEffect(SFX_LEVEL_UP);

        // Combat level up increases max HP (+1 per level)
        if (skillIndex == SKILL_COMBAT) {
            int hpGain = newLevel - oldLevel;
            state->maxHP += hpGain;
            state->currentHP += hpGain;  // Heal on level up
        }
        return true;
    }
    return false;
}

void UpdateTrees(Tree* trees, int treeCount, float dt) {
    for (int i = 0; i < treeCount; i++) {
        if (!trees[i].alive) {
            trees[i].respawnTimer -= dt;
            if (trees[i].respawnTimer <= 0) {
                trees[i].health = TREE_MAX_HEALTH;
                trees[i].alive = true;
            }
        }
    }
}

void UpdateRocks(Rock* rocks, int rockCount, float dt) {
    for (int i = 0; i < rockCount; i++) {
        if (!rocks[i].alive) {
            rocks[i].respawnTimer -= dt;
            if (rocks[i].respawnTimer <= 0) {
                rocks[i].health = ROCK_MAX_HEALTH;
                rocks[i].alive = true;
            }
        }
    }
}
