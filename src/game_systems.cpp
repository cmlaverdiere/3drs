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

        // Stackable items drop as a single pile with quantity
        // Non-stackable items drop individually (though typically amount=1)
        if (IsItemStackable(drop.item)) {
            if (worldItemCount < MAX_WORLD_ITEMS) {
                worldItems[worldItemCount].type = drop.item;
                worldItems[worldItemCount].position = pos;
                worldItems[worldItemCount].position.x += RandomFloat(-0.5f, 0.5f);
                worldItems[worldItemCount].position.z += RandomFloat(-0.5f, 0.5f);
                worldItems[worldItemCount].position.y = 0.0f;
                worldItems[worldItemCount].pickedUp = false;
                worldItems[worldItemCount].canRespawn = false;  // Drops don't respawn
                worldItems[worldItemCount].respawnTimer = 0.0f;
                worldItems[worldItemCount].quantity = amount;
                worldItemCount++;
            }
        } else {
            // Non-stackable items spawn individually
            for (int j = 0; j < amount && worldItemCount < MAX_WORLD_ITEMS; j++) {
                worldItems[worldItemCount].type = drop.item;
                worldItems[worldItemCount].position = pos;
                worldItems[worldItemCount].position.x += RandomFloat(-0.5f, 0.5f);
                worldItems[worldItemCount].position.z += RandomFloat(-0.5f, 0.5f);
                worldItems[worldItemCount].position.y = 0.0f;
                worldItems[worldItemCount].pickedUp = false;
                worldItems[worldItemCount].canRespawn = false;  // Drops don't respawn
                worldItems[worldItemCount].respawnTimer = 0.0f;
                worldItems[worldItemCount].quantity = 1;
                worldItemCount++;
            }
        }
    }
}

void UpdateItemRespawns(WorldItem* items, int itemCount, float dt) {
    for (int i = 0; i < itemCount; i++) {
        if (items[i].pickedUp && items[i].canRespawn && items[i].respawnTimer > 0) {
            items[i].respawnTimer -= dt;
            if (items[i].respawnTimer <= 0) {
                items[i].pickedUp = false;
                items[i].position = items[i].spawnPosition;
            }
        }
    }
}
