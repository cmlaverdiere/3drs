#include "game_systems.h"

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
