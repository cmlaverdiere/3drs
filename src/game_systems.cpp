#include "game_systems.h"
#include "math_utils.h"
#include "raylib.h"

void SpawnDamageIndicator(DamageIndicator* indicators, Vector3 pos, int damage) {
    for (int i = 0; i < MAX_DAMAGE_INDICATORS; i++) {
        if (!indicators[i].active) {
            indicators[i].position = pos;
            indicators[i].position.y += 1.5f;
            indicators[i].damage = damage;
            indicators[i].timer = DAMAGE_INDICATOR_DURATION;
            indicators[i].active = true;
            break;
        }
    }
}

void SpawnXPPopup(XPPopup* popups, int xpAmount, int skillIndex) {
    for (int i = 0; i < MAX_XP_POPUPS; i++) {
        if (!popups[i].active) {
            popups[i].xpAmount = xpAmount;
            popups[i].skillIndex = skillIndex;
            popups[i].timer = XP_POPUP_DURATION;
            popups[i].active = true;
            break;
        }
    }
}

void SpawnEnemyDrops(const EnemyConfig& config, Vector3 pos, WorldItem* worldItems, int& worldItemCount) {
    for (int i = 0; i < config.dropCount; i++) {
        const DropEntry& drop = config.drops[i];

        // Check drop chance
        float roll = (float)GetRandomValue(0, 10000) / 10000.0f;
        if (roll > drop.chance) continue;

        // Determine amount
        int amount = GetRandomValue(drop.minAmount, drop.maxAmount);

        // Spawn items (for stackable items like gil, spawn one pile)
        // For non-stackable, spawn multiple items
        if (drop.item == ITEM_GIL) {
            // Gil is stackable - spawn one pile (amount stored elsewhere if needed)
            if (worldItemCount < MAX_WORLD_ITEMS) {
                worldItems[worldItemCount].type = drop.item;
                worldItems[worldItemCount].position = pos;
                worldItems[worldItemCount].position.x += RandomFloat(-0.5f, 0.5f);
                worldItems[worldItemCount].position.z += RandomFloat(-0.5f, 0.5f);
                worldItems[worldItemCount].position.y = 0.0f;
                worldItems[worldItemCount].pickedUp = false;
                worldItemCount++;
            }
        } else {
            // Non-stackable items
            for (int j = 0; j < amount && worldItemCount < MAX_WORLD_ITEMS; j++) {
                worldItems[worldItemCount].type = drop.item;
                worldItems[worldItemCount].position = pos;
                worldItems[worldItemCount].position.x += RandomFloat(-0.5f, 0.5f);
                worldItems[worldItemCount].position.z += RandomFloat(-0.5f, 0.5f);
                worldItems[worldItemCount].position.y = 0.0f;
                worldItems[worldItemCount].pickedUp = false;
                worldItemCount++;
            }
        }
    }
}
