#include "hud.h"
#include "math_utils.h"
#include "xp_system.h"
#include "inventory.h"
#include <cstdio>

// Colors
static const Color PARCHMENT_BG = { 222, 198, 158, 240 };
static const Color PARCHMENT_BORDER = { 139, 90, 43, 255 };
static const Color PARCHMENT_DARK = { 180, 150, 100, 255 };
static const Color PARCHMENT_TEXT = { 60, 40, 20, 255 };
static const Color INV_BG = { 62, 53, 41, 220 };
static const Color INV_BORDER = { 86, 74, 57, 255 };
static const Color INV_SLOT = { 40, 35, 28, 255 };
static const Color GOLD_TEXT = { 255, 204, 0, 255 };
static const Color BRONZE = { 205, 127, 50, 255 };
static const Color BRONZE_HANDLE = { 139, 90, 43, 255 };
static const Color WOOD_HANDLE = { 101, 67, 33, 255 };

static void DrawOutlinedText(const char* text, int x, int y, int fontSize, Color color, float alpha) {
    Color outlineColor = BLACK;
    outlineColor.a = (unsigned char)(255 * alpha);
    for (int ox = -2; ox <= 2; ox++) {
        for (int oy = -2; oy <= 2; oy++) {
            if (ox != 0 || oy != 0) {
                DrawText(text, x + ox, y + oy, fontSize, outlineColor);
            }
        }
    }
    color.a = (unsigned char)(255 * alpha);
    DrawText(text, x, y, fontSize, color);
}

void DrawHUD(const Camera3D* camera, const PlayerState* state, const PlayerRuntime* runtime,
             const Enemy* enemies, int enemyCount,
             const DamageIndicator* damageIndicators,
             const XPPopup* xpPopups,
             const LevelUpNotification* levelUpNotif,
             const InventoryMenu* invMenu,
             const WorldItem* targetItem, bool showActionMenu,
             float attackCooldown, float swingTimer,
             bool mouseMode, const char* statusMessage,
             int screenWidth, int screenHeight) {

    // Controls help
    DrawText("WASD move, Mouse look, R toggle run, SHIFT inventory, LMB attack", 10, 10, 20, WHITE);
    if (mouseMode) {
        DrawText("[INVENTORY MODE - Release SHIFT to resume]", 10, 35, 16, YELLOW);
    }
    DrawFPS(screenWidth - 100, 10);

    // HP bar
    int hpBarX = 10, hpBarY = screenHeight - 50, hpBarW = 150, hpBarH = 20;
    float hpRatio = (state->maxHP > 0) ? (float)state->currentHP / state->maxHP : 0.0f;
    DrawRectangle(hpBarX, hpBarY, hpBarW, hpBarH, DARKGRAY);
    DrawRectangle(hpBarX, hpBarY, (int)(hpBarW * hpRatio), hpBarH, RED);
    DrawRectangleLines(hpBarX, hpBarY, hpBarW, hpBarH, BLACK);
    char hpText[32];
    snprintf(hpText, sizeof(hpText), "HP: %d/%d", state->currentHP, state->maxHP);
    DrawText(hpText, hpBarX + 5, hpBarY + 3, 14, WHITE);

    // Run energy circle
    int energyCircleX = hpBarX + 20;
    int energyCircleY = hpBarY - 35;
    int energyRadius = 22;
    float energyRatio = runtime->runEnergy / 100.0f;

    const char* modeText = runtime->isRunning ? "RUN" : "Walk";
    int modeW = MeasureText(modeText, 12);
    DrawText(modeText, energyCircleX - modeW/2, energyCircleY - energyRadius - 16, 12,
             runtime->isRunning ? ORANGE : GREEN);

    DrawCircle(energyCircleX, energyCircleY, energyRadius, DARKGRAY);
    Color energyColor = runtime->isRunning ? ORANGE : (Color){80, 180, 80, 255};
    if (energyRatio > 0.01f) {
        float fillAngle = energyRatio * 360.0f;
        DrawCircleSector((Vector2){(float)energyCircleX, (float)energyCircleY}, energyRadius - 2,
                         270.0f - fillAngle, 270.0f, 32, energyColor);
    }
    DrawCircleLines(energyCircleX, energyCircleY, energyRadius, BLACK);
    char energyText[8];
    snprintf(energyText, sizeof(energyText), "%d", (int)runtime->runEnergy);
    int textW = MeasureText(energyText, 14);
    DrawText(energyText, energyCircleX - textW/2, energyCircleY - 7, 14, WHITE);

    // Attack cooldown
    if (attackCooldown > 0) {
        float maxCooldown = GetWeaponCooldown(state->equippedWeapon);
        int cdWidth = (int)(100 * (attackCooldown / maxCooldown));
        DrawRectangle(screenWidth/2 - 50, screenHeight - 40, 100, 10, DARKGRAY);
        DrawRectangle(screenWidth/2 - 50, screenHeight - 40, cdWidth, 10, RED);
    }

    // Damage indicators
    DrawDamageIndicators(camera, damageIndicators, screenWidth, screenHeight);

    // Enemy health bars
    int playerCombatLevel = GetLevelFromXP(state->skillXP[SKILL_COMBAT]);
    DrawEnemyHealthBars(camera, enemies, enemyCount, playerCombatLevel, screenWidth, screenHeight);

    // Crosshair
    if (!mouseMode) {
        int cx = screenWidth / 2;
        int cy = screenHeight / 2;
        DrawLine(cx - 10, cy, cx + 10, cy, WHITE);
        DrawLine(cx, cy - 10, cx, cy + 10, WHITE);
    }

    // Weapon view
    DrawWeaponView(state->equippedWeapon, swingTimer, screenWidth, screenHeight);

    // XP popups
    int xpPopupY = screenHeight / 3;
    for (int i = 0; i < MAX_XP_POPUPS; i++) {
        if (xpPopups[i].active) {
            float timeRatio = xpPopups[i].timer / XP_POPUP_DURATION;
            float alpha = (timeRatio > 0.2f) ? 1.0f : (timeRatio / 0.2f);
            int skillIdx = xpPopups[i].skillIndex;
            int currentXP = state->skillXP[skillIdx];
            int currentLevel = GetLevelFromXP(currentXP);
            int xpForCurrent = XP_TABLE[currentLevel - 1];
            int xpForNext = (currentLevel < 99) ? XP_TABLE[currentLevel] : XP_TABLE[98];
            int xpIntoLevel = currentXP - xpForCurrent;
            int xpNeeded = xpForNext - xpForCurrent;
            float progress = (xpNeeded > 0) ? (float)xpIntoLevel / xpNeeded : 1.0f;

            char xpText[64];
            snprintf(xpText, sizeof(xpText), "+%d %s", xpPopups[i].xpAmount, SKILL_NAMES[skillIdx]);
            int xpTextWidth = MeasureText(xpText, 28);
            int popupX = (screenWidth - xpTextWidth) / 2;

            DrawOutlinedText(xpText, popupX, xpPopupY, 28, (Color){255, 215, 0, 255}, alpha);

            int barWidth = 200;
            int barX = (screenWidth - barWidth) / 2;
            int barY = xpPopupY + 32;
            int barHeight = 8;
            Color barBg = { 40, 40, 40, (unsigned char)(180 * alpha) };
            Color barFg = { 50, 205, 50, (unsigned char)(255 * alpha) };
            Color barBorder = { 100, 100, 100, (unsigned char)(200 * alpha) };

            DrawRectangle(barX, barY, barWidth, barHeight, barBg);
            DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, barFg);
            DrawRectangleLines(barX, barY, barWidth, barHeight, barBorder);

            char levelText[32];
            snprintf(levelText, sizeof(levelText), "Lv %d", currentLevel);
            Color levelColor = { 255, 255, 255, (unsigned char)(255 * alpha) };
            DrawText(levelText, barX + barWidth + 10, barY - 2, 14, levelColor);

            xpPopupY += 55;
        }
    }

    // Skills display
    int skillY = 60;
    DrawText("Skills:", 10, skillY, 18, GOLD);
    skillY += 22;
    for (int i = 0; i < SKILL_COUNT; i++) {
        // Skip Hitpoints skill - HP is now derived from Combat level
        if (i == SKILL_HITPOINTS) continue;
        int level = GetLevelFromXP(state->skillXP[i]);
        int xpForNext = (level < 99) ? XP_TABLE[level] : XP_TABLE[98];
        char skillText[64];
        snprintf(skillText, sizeof(skillText), "%s: %d (%d/%d)",
            SKILL_NAMES[i], level, state->skillXP[i], xpForNext);
        DrawText(skillText, 10, skillY, 14, WHITE);
        skillY += 18;
    }

    // Quest points display
    char qpText[32];
    snprintf(qpText, sizeof(qpText), "Quest Points: %d", state->questPoints);
    DrawText(qpText, 10, skillY + 5, 14, (Color){100, 200, 255, 255});

    // Action menu overlay
    if (showActionMenu && targetItem != nullptr) {
        int menuX = screenWidth / 2 - 100;
        int menuY = screenHeight / 2 - 60;
        int menuW = 200, menuH = 120;

        DrawRectangle(menuX, menuY, menuW, menuH, (Color){0, 0, 0, 180});
        DrawRectangleLines(menuX, menuY, menuW, menuH, GOLD);

        const char* itemName = ITEM_NAMES[targetItem->type];
        DrawText(itemName, menuX + 10, menuY + 10, 18, GOLD);
        DrawText("1. Pickup", menuX + 10, menuY + 40, 16, WHITE);
        DrawText("2. Examine", menuX + 10, menuY + 60, 16, WHITE);
        DrawText("3. Cancel", menuX + 10, menuY + 80, 16, GRAY);
    }

    // Inventory UI
    DrawInventoryUI(state, invMenu, screenWidth, screenHeight);

    // Status message
    if (statusMessage != nullptr) {
        DrawText(statusMessage, 10, screenHeight - 80, 20, YELLOW);
    }

    // Death overlay
    if (runtime->isDead) {
        float fadeProgress = 1.0f - (runtime->deathFadeTimer / DEATH_FADE_DURATION);
        unsigned char alpha = (unsigned char)(255 * fadeProgress);
        if (fadeProgress > 0.5f) alpha = 255;
        DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, alpha });

        if (fadeProgress > 0.3f) {
            const char* deathText = "You died!";
            int deathWidth = MeasureText(deathText, 48);
            unsigned char textAlpha = (unsigned char)(255 * ((fadeProgress - 0.3f) / 0.7f));
            DrawText(deathText, (screenWidth - deathWidth) / 2, screenHeight / 2 - 24, 48,
                     (Color){ 200, 0, 0, textAlpha });
        }
    }

    // Level up banner
    if (levelUpNotif->active) {
        Color bg = PARCHMENT_BG;
        Color border = PARCHMENT_BORDER;
        Color dark = PARCHMENT_DARK;
        Color text = PARCHMENT_TEXT;

        int bannerW = 400, bannerH = 100;
        int bannerX = (screenWidth - bannerW) / 2;
        int bannerY = screenHeight - bannerH - 20;

        float alpha = 1.0f;
        if (levelUpNotif->timer > LEVEL_UP_DURATION - 0.3f) {
            alpha = (LEVEL_UP_DURATION - levelUpNotif->timer) / 0.3f;
        } else if (levelUpNotif->timer < 0.5f) {
            alpha = levelUpNotif->timer / 0.5f;
        }

        bg.a = (unsigned char)(240 * alpha);
        border.a = (unsigned char)(255 * alpha);
        dark.a = (unsigned char)(255 * alpha);
        text.a = (unsigned char)(255 * alpha);

        DrawRectangle(bannerX, bannerY, bannerW, bannerH, bg);
        for (int i = 0; i < 5; i++) {
            int lineY = bannerY + 15 + i * 18;
            DrawLine(bannerX + 10, lineY, bannerX + bannerW - 10, lineY, dark);
        }
        DrawRectangleLinesEx((Rectangle){(float)bannerX, (float)bannerY, (float)bannerW, (float)bannerH}, 3, border);
        DrawRectangleLinesEx((Rectangle){(float)bannerX + 5, (float)bannerY + 5, (float)bannerW - 10, (float)bannerH - 10}, 1, border);

        int cornerSize = 12;
        DrawRectangle(bannerX, bannerY, cornerSize, cornerSize, border);
        DrawRectangle(bannerX + bannerW - cornerSize, bannerY, cornerSize, cornerSize, border);
        DrawRectangle(bannerX, bannerY + bannerH - cornerSize, cornerSize, cornerSize, border);
        DrawRectangle(bannerX + bannerW - cornerSize, bannerY + bannerH - cornerSize, cornerSize, cornerSize, border);

        const char* skillName = SKILL_NAMES[levelUpNotif->skillIndex];
        char titleText[64], levelTextStr[64], newLevelText[64];
        snprintf(titleText, sizeof(titleText), "Congratulations!");
        snprintf(levelTextStr, sizeof(levelTextStr), "You've advanced a %s level!", skillName);
        snprintf(newLevelText, sizeof(newLevelText), "You are now level %d.", levelUpNotif->newLevel);

        int titleWidth = MeasureText(titleText, 24);
        int levelWidth = MeasureText(levelTextStr, 20);
        int newLevelWidth = MeasureText(newLevelText, 18);

        DrawText(titleText, bannerX + (bannerW - titleWidth) / 2, bannerY + 15, 24, text);
        DrawText(levelTextStr, bannerX + (bannerW - levelWidth) / 2, bannerY + 45, 20, text);
        DrawText(newLevelText, bannerX + (bannerW - newLevelWidth) / 2, bannerY + 70, 18, text);
    }
}

void DrawWeaponView(ItemType weapon, float swingTimer, int screenWidth, int screenHeight) {
    if (weapon == ITEM_NONE) return;

    float weaponBaseX = screenWidth - 150.0f;
    float weaponBaseY = screenHeight - 100.0f;

    float swingAngle = 0.0f;
    float swingOffsetX = 0.0f;
    float swingOffsetY = 0.0f;
    if (swingTimer > 0) {
        float swingProgress = swingTimer / SWING_DURATION;
        swingAngle = sinf(swingProgress * PI) * 60.0f;
        swingOffsetX = -sinf(swingProgress * PI) * 80.0f;
        swingOffsetY = -sinf(swingProgress * PI) * 40.0f;
    }

    float wpnX = weaponBaseX + swingOffsetX;
    float wpnY = weaponBaseY + swingOffsetY;
    float radAngle = swingAngle * DEG2RAD;
    float cosA = cosf(radAngle);
    float sinA = sinf(radAngle);

    if (weapon == ITEM_BRONZE_SHORTSWORD) {
        float bladeLen = 120.0f, bladeWidth = 12.0f;
        Vector2 bladeTip = { wpnX + (-bladeLen * sinA), wpnY + (-bladeLen * cosA) };
        Vector2 bladeBase = { wpnX, wpnY };

        DrawLineEx(bladeBase, bladeTip, bladeWidth + 2, DARKGRAY);
        DrawLineEx(bladeBase, bladeTip, bladeWidth, BRONZE);

        Vector2 handleEnd = { wpnX + (30.0f * sinA), wpnY + (30.0f * cosA) };
        DrawLineEx(bladeBase, handleEnd, 10.0f, BRONZE_HANDLE);

        Vector2 guardLeft = { wpnX + (-15.0f * cosA), wpnY + (15.0f * sinA) };
        Vector2 guardRight = { wpnX + (15.0f * cosA), wpnY + (-15.0f * sinA) };
        DrawLineEx(guardLeft, guardRight, 6.0f, BRONZE_HANDLE);
    } else if (weapon == ITEM_BRONZE_AXE) {
        float handleLen = 100.0f, handleWidth = 8.0f;

        Vector2 handleEnd = { wpnX + (-handleLen * sinA), wpnY + (-handleLen * cosA) };
        Vector2 handleBase = { wpnX + (30.0f * sinA), wpnY + (30.0f * cosA) };
        DrawLineEx(handleBase, handleEnd, handleWidth, WOOD_HANDLE);

        Vector2 headCenter = { wpnX + (-handleLen * 0.85f * sinA), wpnY + (-handleLen * 0.85f * cosA) };
        Vector2 headLeft = { headCenter.x + (-30.0f * cosA), headCenter.y + (30.0f * sinA) };
        Vector2 headRight = { headCenter.x + (10.0f * cosA), headCenter.y + (-10.0f * sinA) };
        DrawLineEx(headLeft, headRight, 20.0f, BRONZE);
    } else if (weapon == ITEM_IRON_2H_SWORD) {
        Color ironBlade = { 180, 180, 190, 255 };
        Color ironDark = { 120, 120, 130, 255 };
        Color leatherGrip = { 80, 50, 30, 255 };

        // Longer blade for 2H sword
        float bladeLen = 180.0f, bladeWidth = 16.0f;
        Vector2 bladeTip = { wpnX + (-bladeLen * sinA), wpnY + (-bladeLen * cosA) };
        Vector2 bladeBase = { wpnX, wpnY };

        DrawLineEx(bladeBase, bladeTip, bladeWidth + 2, DARKGRAY);
        DrawLineEx(bladeBase, bladeTip, bladeWidth, ironBlade);
        // Fuller (groove)
        DrawLineEx(bladeBase, bladeTip, 4.0f, ironDark);

        // Longer handle for two hands
        Vector2 handleEnd = { wpnX + (50.0f * sinA), wpnY + (50.0f * cosA) };
        DrawLineEx(bladeBase, handleEnd, 12.0f, leatherGrip);

        // Larger crossguard
        Vector2 guardLeft = { wpnX + (-25.0f * cosA), wpnY + (25.0f * sinA) };
        Vector2 guardRight = { wpnX + (25.0f * cosA), wpnY + (-25.0f * sinA) };
        DrawLineEx(guardLeft, guardRight, 8.0f, ironDark);

        // Pommel
        Vector2 pommelPos = { wpnX + (55.0f * sinA), wpnY + (55.0f * cosA) };
        DrawCircleV(pommelPos, 8.0f, ironDark);
    } else if (weapon == ITEM_STEEL_SCIMITAR || weapon == ITEM_MITHRIL_SCIMITAR || weapon == ITEM_ADAMANT_SCIMITAR) {
        // Scimitar colors based on tier
        Color bladeColor, edgeColor, darkColor;
        if (weapon == ITEM_STEEL_SCIMITAR) {
            bladeColor = (Color){ 180, 180, 190, 255 };
            edgeColor = (Color){ 220, 220, 230, 255 };
            darkColor = (Color){ 100, 100, 110, 255 };
        } else if (weapon == ITEM_MITHRIL_SCIMITAR) {
            bladeColor = (Color){ 80, 130, 170, 255 };
            edgeColor = (Color){ 140, 180, 220, 255 };
            darkColor = (Color){ 40, 80, 120, 255 };
        } else {  // Adamant
            bladeColor = (Color){ 60, 140, 60, 255 };
            edgeColor = (Color){ 100, 200, 100, 255 };
            darkColor = (Color){ 30, 90, 30, 255 };
        }
        Color leatherGrip = { 70, 45, 25, 255 };
        Color goldAccent = { 200, 160, 60, 255 };

        // Scimitar: wide curved blade sweeping to the right
        // Draw as a series of connected quads to form the curve
        int segments = 8;
        float bladeLength = 140.0f;

        // Store points for the curved blade shape
        Vector2 outerEdge[9], innerEdge[9];
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / segments;
            // Parametric curve - gentle curve outward
            float baseX = -bladeLength * t;
            float curveAmount = sinf(t * PI) * 18.0f;  // Gentler curve
            float bladeW = 14.0f + sinf(t * PI) * 12.0f;  // Wider blade overall

            // Transform by swing angle
            float px = baseX * sinA - curveAmount * cosA;
            float py = baseX * cosA + curveAmount * sinA;

            // Outer edge (the sharp curved side)
            outerEdge[i] = (Vector2){ wpnX + px - bladeW * cosA * 0.5f,
                                       wpnY + py + bladeW * sinA * 0.5f };
            // Inner edge (spine of blade)
            innerEdge[i] = (Vector2){ wpnX + px + bladeW * cosA * 0.5f,
                                       wpnY + py - bladeW * sinA * 0.5f };
        }

        // Draw blade as triangles
        for (int i = 0; i < segments; i++) {
            // Blade body
            DrawTriangle(outerEdge[i], innerEdge[i], outerEdge[i+1], bladeColor);
            DrawTriangle(innerEdge[i], innerEdge[i+1], outerEdge[i+1], bladeColor);
        }

        // Shiny edge highlight on outer curve
        for (int i = 0; i < segments; i++) {
            DrawLineEx(outerEdge[i], outerEdge[i+1], 2.0f, edgeColor);
        }

        // Dark spine on inner edge
        for (int i = 0; i < segments; i++) {
            DrawLineEx(innerEdge[i], innerEdge[i+1], 2.0f, darkColor);
        }

        // Handle
        Vector2 handleEnd = { wpnX + (40.0f * sinA), wpnY + (40.0f * cosA) };
        DrawLineEx((Vector2){wpnX, wpnY}, handleEnd, 12.0f, leatherGrip);
        // Wrap lines on handle
        for (int i = 1; i <= 3; i++) {
            float ht = i * 0.25f;
            Vector2 wrapPos = { wpnX + (40.0f * ht * sinA), wpnY + (40.0f * ht * cosA) };
            float wrapSize = 8.0f;
            Vector2 wrapL = { wrapPos.x - wrapSize * cosA, wrapPos.y + wrapSize * sinA };
            Vector2 wrapR = { wrapPos.x + wrapSize * cosA, wrapPos.y - wrapSize * sinA };
            DrawLineEx(wrapL, wrapR, 2.0f, darkColor);
        }

        // Curved guard with gold accent
        Vector2 guardL = { wpnX + (-18.0f * cosA), wpnY + (18.0f * sinA) };
        Vector2 guardR = { wpnX + (6.0f * cosA), wpnY + (-6.0f * sinA) };
        DrawLineEx(guardL, guardR, 6.0f, darkColor);
        DrawLineEx(guardL, guardR, 3.0f, goldAccent);

        // Pommel
        Vector2 pommelPos = { wpnX + (45.0f * sinA), wpnY + (45.0f * cosA) };
        DrawCircleV(pommelPos, 6.0f, darkColor);
        DrawCircleV(pommelPos, 3.0f, goldAccent);
    }
}

// Helper to draw an item icon at a given center position
static void DrawItemIcon(ItemType item, int cx, int cy) {
    if (item == ITEM_BRONZE_SHORTSWORD) {
        DrawRectangle(cx - 2, cy - 14, 4, 24, BRONZE);
        DrawRectangle(cx - 2, cy + 10, 4, 8, BROWN);
        DrawRectangle(cx - 8, cy + 8, 16, 3, BROWN);
    } else if (item == ITEM_BRONZE_AXE) {
        DrawRectangle(cx - 2, cy - 10, 4, 20, WOOD_HANDLE);
        DrawRectangle(cx - 10, cy - 10, 12, 8, BRONZE);
    } else if (item == ITEM_COW_HIDE) {
        Color hideColor = { 139, 90, 43, 255 };
        DrawRectangle(cx - 12, cy - 8, 24, 16, hideColor);
        DrawRectangle(cx - 4, cy - 4, 6, 6, DARKBROWN);
    } else if (item == ITEM_BONES) {
        Color boneColor = { 230, 220, 200, 255 };
        DrawRectangle(cx - 2, cy - 10, 4, 20, boneColor);
        DrawCircle(cx, cy - 10, 4, boneColor);
        DrawCircle(cx, cy + 10, 4, boneColor);
    } else if (item == ITEM_GIL) {
        Color goldColor = { 255, 215, 0, 255 };
        DrawCircle(cx, cy, 10, goldColor);
        DrawCircle(cx, cy, 6, GOLD);
    } else if (item == ITEM_LOGS) {
        Color barkColor = { 101, 67, 33, 255 };
        Color woodColor = { 210, 180, 140, 255 };
        DrawRectangle(cx - 12, cy - 4, 24, 8, barkColor);
        DrawCircle(cx - 12, cy, 4, woodColor);
        DrawCircle(cx + 12, cy, 4, woodColor);
    } else if (item == ITEM_OAK_LOGS) {
        // Oak logs - darker, larger icon
        Color oakBark = { 80, 50, 25, 255 };
        Color oakRings = { 150, 110, 60, 255 };
        DrawRectangle(cx - 14, cy - 5, 28, 10, oakBark);
        DrawCircle(cx - 14, cy, 5, oakRings);
        DrawCircle(cx + 14, cy, 5, oakRings);
        // Ring detail
        DrawCircle(cx - 14, cy, 3, oakBark);
        DrawCircle(cx + 14, cy, 3, oakBark);
    } else if (item == ITEM_CHITIN) {
        Color chitinColor = { 101, 67, 33, 255 };
        DrawRectangle(cx - 10, cy - 6, 20, 12, chitinColor);
        DrawRectangle(cx - 8, cy - 8, 4, 4, chitinColor);
        DrawRectangle(cx + 4, cy - 8, 4, 4, chitinColor);
    } else if (item == ITEM_IRON_2H_SWORD) {
        Color ironBlade = { 180, 180, 190, 255 };
        Color ironDark = { 120, 120, 130, 255 };
        Color leatherGrip = { 80, 50, 30, 255 };
        DrawRectangle(cx - 3, cy - 16, 6, 28, ironBlade);
        DrawRectangle(cx - 1, cy - 14, 2, 20, ironDark);
        DrawRectangle(cx - 2, cy + 12, 4, 10, leatherGrip);
        DrawRectangle(cx - 10, cy + 10, 20, 4, ironDark);
        DrawCircle(cx, cy + 24, 3, ironDark);
    } else if (item == ITEM_BANDIT_ORDERS) {
        Color parchment = { 240, 230, 200, 255 };
        Color waxSeal = { 150, 40, 40, 255 };
        DrawRectangle(cx - 8, cy - 10, 16, 20, parchment);
        DrawCircle(cx, cy + 6, 4, waxSeal);
    } else if (item == ITEM_DESERT_ARTIFACT) {
        Color gold = { 255, 200, 50, 255 };
        Color glow = { 255, 230, 150, 255 };
        DrawTriangle((Vector2){(float)cx, (float)(cy - 12)},
                     (Vector2){(float)(cx - 10), (float)(cy + 8)},
                     (Vector2){(float)(cx + 10), (float)(cy + 8)}, gold);
        DrawCircle(cx, cy - 4, 4, glow);
    } else if (item == ITEM_SILK) {
        Color silkColor = { 200, 50, 80, 255 };
        Color silkHighlight = { 230, 100, 120, 255 };
        DrawRectangle(cx - 10, cy - 6, 20, 12, silkColor);
        DrawRectangle(cx - 8, cy - 2, 16, 4, silkHighlight);
    } else if (item == ITEM_SPICE) {
        Color bagColor = { 160, 120, 80, 255 };
        Color spiceColor = { 200, 100, 30, 255 };
        DrawCircle(cx, cy + 2, 10, bagColor);
        DrawRectangle(cx - 2, cy - 10, 4, 8, bagColor);
        DrawCircle(cx, cy - 6, 3, spiceColor);
    } else if (item == ITEM_STEEL_SCIMITAR) {
        Color steelBlade = { 180, 180, 190, 255 };
        Color steelHandle = { 100, 80, 60, 255 };
        // Curved blade
        DrawRectangle(cx - 2, cy - 14, 4, 18, steelBlade);
        DrawRectangle(cx - 5, cy - 14, 4, 10, steelBlade);
        // Handle
        DrawRectangle(cx - 2, cy + 4, 4, 10, steelHandle);
        DrawRectangle(cx - 6, cy + 2, 12, 3, steelHandle);
    } else if (item == ITEM_MITHRIL_SCIMITAR) {
        Color mithrilBlade = { 100, 140, 180, 255 };
        Color mithrilHandle = { 80, 100, 120, 255 };
        // Curved blade
        DrawRectangle(cx - 2, cy - 14, 4, 18, mithrilBlade);
        DrawRectangle(cx - 5, cy - 14, 4, 10, mithrilBlade);
        // Handle
        DrawRectangle(cx - 2, cy + 4, 4, 10, mithrilHandle);
        DrawRectangle(cx - 6, cy + 2, 12, 3, mithrilHandle);
    } else if (item == ITEM_ADAMANT_SCIMITAR) {
        Color adamantBlade = { 80, 160, 80, 255 };
        Color adamantHandle = { 60, 100, 60, 255 };
        // Curved blade
        DrawRectangle(cx - 2, cy - 14, 4, 18, adamantBlade);
        DrawRectangle(cx - 5, cy - 14, 4, 10, adamantBlade);
        // Handle
        DrawRectangle(cx - 2, cy + 4, 4, 10, adamantHandle);
        DrawRectangle(cx - 6, cy + 2, 12, 3, adamantHandle);
    }
}

void DrawInventoryUI(const PlayerState* state, const InventoryMenu* menu,
                     int screenWidth, int screenHeight) {
    int invX, invY;
    GetInventoryPosition(screenWidth, &invX, &invY);

    int invW = INV_COLS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING;
    int invH = INV_ROWS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING + 25;
    DrawRectangle(invX - SLOT_PADDING, invY - 25, invW, invH, INV_BG);
    DrawRectangleLines(invX - SLOT_PADDING, invY - 25, invW, invH, INV_BORDER);
    DrawText("Inventory", invX, invY - 22, 16, GOLD_TEXT);

    Vector2 mouse = GetMousePosition();

    // Determine which slot the mouse is over (for drag highlighting)
    int hoverSlot = -1;
    if (menu->isDragging) {
        for (int row = 0; row < INV_ROWS; row++) {
            for (int col = 0; col < INV_COLS; col++) {
                int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
                int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);
                if (mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
                    mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE) {
                    hoverSlot = row * INV_COLS + col;
                    break;
                }
            }
            if (hoverSlot >= 0) break;
        }
    }

    for (int row = 0; row < INV_ROWS; row++) {
        for (int col = 0; col < INV_COLS; col++) {
            int slotIdx = row * INV_COLS + col;
            int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
            int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);

            // Slot background (dimmed if being dragged from)
            if (menu->isDragging && slotIdx == menu->dragSlot) {
                DrawRectangle(slotX, slotY, SLOT_SIZE, SLOT_SIZE, (Color){30, 25, 20, 255});
            } else {
                DrawRectangle(slotX, slotY, SLOT_SIZE, SLOT_SIZE, INV_SLOT);
            }

            // Slot border
            bool isEquipped = (state->inventory[slotIdx] != ITEM_NONE &&
                               state->inventory[slotIdx] == state->equippedWeapon);
            bool isDragTarget = (menu->isDragging && slotIdx == hoverSlot && slotIdx != menu->dragSlot);

            if (isDragTarget) {
                // Highlight drop target with cyan
                DrawRectangleLines(slotX, slotY, SLOT_SIZE, SLOT_SIZE, (Color){0, 200, 255, 255});
                DrawRectangleLines(slotX+1, slotY+1, SLOT_SIZE-2, SLOT_SIZE-2, (Color){0, 200, 255, 255});
            } else if (isEquipped) {
                DrawRectangleLines(slotX, slotY, SLOT_SIZE, SLOT_SIZE, (Color){255, 215, 0, 255});
                DrawRectangleLines(slotX+1, slotY+1, SLOT_SIZE-2, SLOT_SIZE-2, (Color){255, 215, 0, 255});
            } else {
                DrawRectangleLines(slotX, slotY, SLOT_SIZE, SLOT_SIZE, INV_BORDER);
            }

            // Don't draw item if being dragged (it will be drawn at cursor)
            if (menu->isDragging && slotIdx == menu->dragSlot) continue;

            // Draw item icon
            ItemType item = state->inventory[slotIdx];
            int cx = slotX + SLOT_SIZE / 2;
            int cy = slotY + SLOT_SIZE / 2;
            DrawItemIcon(item, cx, cy);

            // Stack count
            if (item != ITEM_NONE && IsItemStackable(item) && state->inventoryCount[slotIdx] > 1) {
                char countText[16];
                snprintf(countText, sizeof(countText), "%d", state->inventoryCount[slotIdx]);
                DrawText(countText, slotX + 2, slotY + 2, 10, YELLOW);
            }
        }
    }

    // Draw dragged item at cursor
    if (menu->isDragging && menu->dragSlot >= 0) {
        ItemType draggedItem = state->inventory[menu->dragSlot];
        if (draggedItem != ITEM_NONE) {
            int cx = (int)mouse.x;
            int cy = (int)mouse.y;
            DrawItemIcon(draggedItem, cx, cy);

            // Draw stack count for dragged stackable items
            if (IsItemStackable(draggedItem) && state->inventoryCount[menu->dragSlot] > 1) {
                char countText[16];
                snprintf(countText, sizeof(countText), "%d", state->inventoryCount[menu->dragSlot]);
                DrawText(countText, cx - SLOT_SIZE/2 + 2, cy - SLOT_SIZE/2 + 2, 10, YELLOW);
            }
        }
    }

    // Context menu
    if (menu->showContextMenu && menu->contextSlot >= 0 && state->inventory[menu->contextSlot] != ITEM_NONE) {
        const int menuWidth = 80;
        const int menuItemHeight = 20;
        const int menuPadding = 4;
        ItemType menuItem = state->inventory[menu->contextSlot];
        bool isWeapon = IsWeapon(menuItem);

        const char* options[4];
        int optionCount;
        if (isWeapon) {
            bool equipped = (state->equippedWeapon == menuItem);
            options[0] = equipped ? "Unequip" : "Equip";
            options[1] = "Examine";
            options[2] = "Drop";
            options[3] = "Cancel";
            optionCount = 4;
        } else {
            options[0] = "Examine";
            options[1] = "Drop";
            options[2] = "Cancel";
            optionCount = 3;
        }

        int menuHeight = menuItemHeight * optionCount;

        DrawRectangle(menu->menuX, menu->menuY, menuWidth, menuHeight, INV_SLOT);
        DrawRectangleLines(menu->menuX, menu->menuY, menuWidth, menuHeight, INV_BORDER);

        Vector2 mouse = GetMousePosition();
        for (int i = 0; i < optionCount; i++) {
            int optY = menu->menuY + i * menuItemHeight;

            if (mouse.x >= menu->menuX && mouse.x <= menu->menuX + menuWidth &&
                mouse.y >= optY && mouse.y <= optY + menuItemHeight) {
                DrawRectangle(menu->menuX + 1, optY + 1, menuWidth - 2, menuItemHeight - 2,
                              (Color){60, 55, 45, 255});
            }

            DrawText(options[i], menu->menuX + menuPadding, optY + 4, 12, GOLD_TEXT);
        }
    }
}

void DrawDamageIndicators(const Camera3D* camera, const DamageIndicator* indicators,
                          int screenWidth, int screenHeight) {
    for (int i = 0; i < MAX_DAMAGE_INDICATORS; i++) {
        if (!indicators[i].active) continue;

        Vector3 toIndicator = {
            indicators[i].position.x - camera->position.x,
            0,
            indicators[i].position.z - camera->position.z
        };
        Vector3 camForward = {
            camera->target.x - camera->position.x,
            0,
            camera->target.z - camera->position.z
        };
        if (Dot3D(toIndicator, camForward) <= 0) continue;

        Vector2 screenPos = GetWorldToScreen(indicators[i].position, *camera);
        if (screenPos.x < 0 || screenPos.x > screenWidth ||
            screenPos.y < 0 || screenPos.y > screenHeight) continue;

        char dmgText[16];
        snprintf(dmgText, sizeof(dmgText), "%d", indicators[i].damage);
        float timeRatio = indicators[i].timer / DAMAGE_INDICATOR_DURATION;
        float alpha = (timeRatio > 0.2f) ? 1.0f : (timeRatio / 0.2f);
        int fontSize = 48;
        int textWidth = MeasureText(dmgText, fontSize);
        int tx = (int)screenPos.x - textWidth/2;
        int ty = (int)screenPos.y - fontSize/2;

        Color dmgColor = (indicators[i].damage == 0) ? BLUE : RED;
        DrawOutlinedText(dmgText, tx, ty, fontSize, dmgColor, alpha);
    }
}

void DrawEnemyHealthBars(const Camera3D* camera, const Enemy* enemies, int enemyCount,
                         int playerCombatLevel, int screenWidth, int screenHeight) {
    for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].alive) continue;

        const EnemyConfig& config = ENEMY_CONFIGS[enemies[i].type];
        float enemyTerrainY = GetTerrainHeight(enemies[i].position.x, enemies[i].position.z);
        Vector3 enemyPos = { enemies[i].position.x, enemyTerrainY, enemies[i].position.z };
        float dist = Distance3D(camera->position, enemyPos);
        if (dist > PLAYER_ATTACK_RANGE) continue;

        Vector3 toEnemy = {
            enemies[i].position.x - camera->position.x,
            0,
            enemies[i].position.z - camera->position.z
        };
        Vector3 camForward = {
            camera->target.x - camera->position.x,
            0,
            camera->target.z - camera->position.z
        };
        if (Dot3D(toEnemy, camForward) <= 0) continue;

        Vector3 healthBarPos = { enemies[i].position.x, enemyTerrainY + 2.0f, enemies[i].position.z };
        Vector2 screenPos = GetWorldToScreen(healthBarPos, *camera);
        if (screenPos.x < 0 || screenPos.x > screenWidth ||
            screenPos.y < 0 || screenPos.y > screenHeight) continue;

        int barWidth = 40, barHeight = 6;
        int healthWidth = (int)(barWidth * enemies[i].health / (float)config.maxHealth);
        DrawRectangle((int)screenPos.x - barWidth/2, (int)screenPos.y, barWidth, barHeight, DARKGRAY);
        DrawRectangle((int)screenPos.x - barWidth/2, (int)screenPos.y, healthWidth, barHeight, GREEN);
        DrawRectangleLines((int)screenPos.x - barWidth/2, (int)screenPos.y, barWidth, barHeight, BLACK);

        // OSRS-style level color coding
        int levelDiff = config.combatLevel - playerCombatLevel;
        Color levelColor;
        if (levelDiff < -5) {
            levelColor = (Color){ 0, 255, 0, 255 };       // Green - much lower level
        } else if (levelDiff <= 0) {
            levelColor = (Color){ 255, 255, 0, 255 };     // Yellow - slightly lower or equal
        } else if (levelDiff <= 5) {
            levelColor = (Color){ 255, 128, 0, 255 };     // Orange - slightly higher
        } else {
            levelColor = (Color){ 255, 0, 0, 255 };       // Red - much higher level
        }

        char nameText[64];
        snprintf(nameText, sizeof(nameText), "Level %d %s", config.combatLevel, config.name);
        int fontSize = 18;
        int nameWidth = MeasureText(nameText, fontSize);
        DrawText(nameText, (int)screenPos.x - nameWidth/2, (int)screenPos.y - 20, fontSize, levelColor);
    }
}

void UpdateHUDTimers(DamageIndicator* damageIndicators,
                     XPPopup* xpPopups,
                     LevelUpNotification* levelUpNotif,
                     float dt) {
    for (int i = 0; i < MAX_DAMAGE_INDICATORS; i++) {
        if (damageIndicators[i].active) {
            damageIndicators[i].timer -= dt;
            if (damageIndicators[i].timer <= 0) {
                damageIndicators[i].active = false;
            }
        }
    }

    for (int i = 0; i < MAX_XP_POPUPS; i++) {
        if (xpPopups[i].active) {
            xpPopups[i].timer -= dt;
            if (xpPopups[i].timer <= 0) {
                xpPopups[i].active = false;
            }
        }
    }

    if (levelUpNotif->active) {
        levelUpNotif->timer -= dt;
        if (levelUpNotif->timer <= 0) {
            levelUpNotif->active = false;
        }
    }
}

void DrawDialogueBox(const DialogueState* dialogue, const NPC* npcs,
                     const char** questDialogue, int questDialogueCount,
                     bool showAcceptPrompt,
                     int screenWidth, int screenHeight) {
    if (!dialogue->active || dialogue->npcIndex < 0) return;

    const NPC& npc = npcs[dialogue->npcIndex];
    const NPCConfig& config = NPC_CONFIGS[npc.type];

    // Use quest dialogue if provided, otherwise NPC's default
    bool useQuestDialogue = (questDialogue != nullptr && questDialogueCount > 0);
    int totalLines = useQuestDialogue ? questDialogueCount : config.dialogueCount;

    // Dialogue box dimensions - large parchment at bottom of screen
    const int BOX_MARGIN = 40;
    const int BOX_HEIGHT = 180;
    const int BOX_WIDTH = screenWidth - (BOX_MARGIN * 2);
    const int BOX_X = BOX_MARGIN;
    const int BOX_Y = screenHeight - BOX_HEIGHT - BOX_MARGIN;

    // Inner padding
    const int PADDING = 20;
    const int BORDER_WIDTH = 4;

    // Draw outer border (darker)
    DrawRectangle(BOX_X - BORDER_WIDTH, BOX_Y - BORDER_WIDTH,
                  BOX_WIDTH + BORDER_WIDTH * 2, BOX_HEIGHT + BORDER_WIDTH * 2,
                  PARCHMENT_BORDER);

    // Draw main parchment background
    DrawRectangle(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, PARCHMENT_BG);

    // Draw inner border accent
    DrawRectangleLines(BOX_X + 6, BOX_Y + 6, BOX_WIDTH - 12, BOX_HEIGHT - 12, PARCHMENT_DARK);

    // Decorative corner triangles
    const int CORNER_SIZE = 12;
    // Top-left
    DrawTriangle(
        (Vector2){(float)BOX_X, (float)BOX_Y},
        (Vector2){(float)(BOX_X + CORNER_SIZE), (float)BOX_Y},
        (Vector2){(float)BOX_X, (float)(BOX_Y + CORNER_SIZE)},
        PARCHMENT_BORDER
    );
    // Top-right
    DrawTriangle(
        (Vector2){(float)(BOX_X + BOX_WIDTH), (float)BOX_Y},
        (Vector2){(float)(BOX_X + BOX_WIDTH - CORNER_SIZE), (float)BOX_Y},
        (Vector2){(float)(BOX_X + BOX_WIDTH), (float)(BOX_Y + CORNER_SIZE)},
        PARCHMENT_BORDER
    );
    // Bottom-left
    DrawTriangle(
        (Vector2){(float)BOX_X, (float)(BOX_Y + BOX_HEIGHT)},
        (Vector2){(float)(BOX_X + CORNER_SIZE), (float)(BOX_Y + BOX_HEIGHT)},
        (Vector2){(float)BOX_X, (float)(BOX_Y + BOX_HEIGHT - CORNER_SIZE)},
        PARCHMENT_BORDER
    );
    // Bottom-right
    DrawTriangle(
        (Vector2){(float)(BOX_X + BOX_WIDTH), (float)(BOX_Y + BOX_HEIGHT)},
        (Vector2){(float)(BOX_X + BOX_WIDTH - CORNER_SIZE), (float)(BOX_Y + BOX_HEIGHT)},
        (Vector2){(float)(BOX_X + BOX_WIDTH), (float)(BOX_Y + BOX_HEIGHT - CORNER_SIZE)},
        PARCHMENT_BORDER
    );

    // NPC name header
    int nameX = BOX_X + PADDING;
    int nameY = BOX_Y + PADDING;
    DrawText(config.name, nameX, nameY, 28, PARCHMENT_BORDER);

    // Separator line under name
    int separatorY = nameY + 32;
    DrawRectangle(BOX_X + PADDING, separatorY, BOX_WIDTH - PADDING * 2, 2, PARCHMENT_BORDER);

    // Dialogue text
    if (dialogue->currentLine < totalLines) {
        const char* dialogueText = useQuestDialogue ?
            questDialogue[dialogue->currentLine] :
            config.dialogueLines[dialogue->currentLine];
        int textX = BOX_X + PADDING;
        int textY = separatorY + 15;

        // Draw dialogue text (simple, no word wrap for now)
        DrawText(dialogueText, textX, textY, 22, PARCHMENT_TEXT);
    }

    // Check if on last line
    bool onLastLine = (dialogue->currentLine >= totalLines - 1);

    // Show accept/decline buttons if this is a quest intro on the last line
    if (showAcceptPrompt && onLastLine) {
        // Button dimensions
        const int BUTTON_W = 100;
        const int BUTTON_H = 30;
        const int BUTTON_Y = BOX_Y + BOX_HEIGHT - PADDING - BUTTON_H - 5;
        const int ACCEPT_X = screenWidth / 2 - 120;
        const int DECLINE_X = screenWidth / 2 + 20;

        Vector2 mouse = GetMousePosition();

        // Accept button
        bool acceptHover = (mouse.x >= ACCEPT_X && mouse.x <= ACCEPT_X + BUTTON_W &&
                           mouse.y >= BUTTON_Y && mouse.y <= BUTTON_Y + BUTTON_H);
        Color acceptBg = acceptHover ? (Color){100, 160, 100, 255} : (Color){80, 130, 80, 255};
        DrawRectangle(ACCEPT_X, BUTTON_Y, BUTTON_W, BUTTON_H, acceptBg);
        DrawRectangleLines(ACCEPT_X, BUTTON_Y, BUTTON_W, BUTTON_H, PARCHMENT_BORDER);
        const char* acceptText = "Accept";
        int acceptTextW = MeasureText(acceptText, 18);
        DrawText(acceptText, ACCEPT_X + (BUTTON_W - acceptTextW) / 2, BUTTON_Y + 6, 18, WHITE);

        // Decline button
        bool declineHover = (mouse.x >= DECLINE_X && mouse.x <= DECLINE_X + BUTTON_W &&
                            mouse.y >= BUTTON_Y && mouse.y <= BUTTON_Y + BUTTON_H);
        Color declineBg = declineHover ? (Color){160, 100, 100, 255} : (Color){130, 80, 80, 255};
        DrawRectangle(DECLINE_X, BUTTON_Y, BUTTON_W, BUTTON_H, declineBg);
        DrawRectangleLines(DECLINE_X, BUTTON_Y, BUTTON_W, BUTTON_H, PARCHMENT_BORDER);
        const char* declineText = "Decline";
        int declineTextW = MeasureText(declineText, 18);
        DrawText(declineText, DECLINE_X + (BUTTON_W - declineTextW) / 2, BUTTON_Y + 6, 18, WHITE);
    } else {
        // "Click to continue" / "Click to close" prompt (blinking)
        const char* prompt;
        if (!onLastLine) {
            prompt = "Click to continue...";
        } else {
            prompt = "Click to close";
        }

        int promptFontSize = 18;
        int promptWidth = MeasureText(prompt, promptFontSize);
        int promptX = BOX_X + BOX_WIDTH - PADDING - promptWidth;
        int promptY = BOX_Y + BOX_HEIGHT - PADDING - promptFontSize;

        // Blinking effect (visible 70% of the time)
        float time = (float)GetTime();
        if (fmodf(time, 1.0f) < 0.7f) {
            Color promptColor = PARCHMENT_TEXT;
            promptColor.a = 180;
            DrawText(prompt, promptX, promptY, promptFontSize, promptColor);
        }
    }
}

void DrawNPCPrompt(const char* npcName, int screenWidth, int screenHeight) {
    char prompt[64];
    snprintf(prompt, sizeof(prompt), "Press E to talk to %s", npcName);
    int promptFontSize = 20;
    int promptWidth = MeasureText(prompt, promptFontSize);
    int promptX = screenWidth / 2 - promptWidth / 2;
    int promptY = screenHeight - 100;

    // Draw with outline for visibility
    DrawText(prompt, promptX - 1, promptY - 1, promptFontSize, BLACK);
    DrawText(prompt, promptX + 1, promptY - 1, promptFontSize, BLACK);
    DrawText(prompt, promptX - 1, promptY + 1, promptFontSize, BLACK);
    DrawText(prompt, promptX + 1, promptY + 1, promptFontSize, BLACK);
    DrawText(prompt, promptX, promptY, promptFontSize, WHITE);
}

void DrawShopUI(const ShopState* shop, const PlayerState* state,
                int screenWidth, int screenHeight) {
    if (!shop->active) return;

    const int BOX_WIDTH = 400;
    const int BOX_HEIGHT = 350;
    const int BOX_X = (screenWidth - BOX_WIDTH) / 2;
    const int BOX_Y = (screenHeight - BOX_HEIGHT) / 2;
    const int PADDING = 20;

    // Draw parchment background
    DrawRectangle(BOX_X - 4, BOX_Y - 4, BOX_WIDTH + 8, BOX_HEIGHT + 8, PARCHMENT_BORDER);
    DrawRectangle(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, PARCHMENT_BG);
    DrawRectangleLines(BOX_X + 6, BOX_Y + 6, BOX_WIDTH - 12, BOX_HEIGHT - 12, PARCHMENT_DARK);

    // Title
    const char* title = "Zeke's Superior Scimitars";
    int titleW = MeasureText(title, 24);
    DrawText(title, BOX_X + (BOX_WIDTH - titleW) / 2, BOX_Y + PADDING, 24, PARCHMENT_BORDER);

    // Separator
    DrawRectangle(BOX_X + PADDING, BOX_Y + 55, BOX_WIDTH - PADDING * 2, 2, PARCHMENT_BORDER);

    // Player's gold display
    int gilCount = GetGilCount(state);
    char gilText[32];
    snprintf(gilText, sizeof(gilText), "Your gold: %d", gilCount);
    DrawText(gilText, BOX_X + PADDING, BOX_Y + 65, 16, GOLD_TEXT);

    // Item grid (3 items horizontally)
    const int ITEM_SIZE = 80;
    const int ITEM_SPACING = 30;
    int startX = BOX_X + (BOX_WIDTH - (3 * ITEM_SIZE + 2 * ITEM_SPACING)) / 2;
    int itemY = BOX_Y + 100;

    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < shop->itemCount; i++) {
        int itemX = startX + i * (ITEM_SIZE + ITEM_SPACING);

        // Item slot background
        bool isHovered = (mouse.x >= itemX && mouse.x <= itemX + ITEM_SIZE &&
                          mouse.y >= itemY && mouse.y <= itemY + ITEM_SIZE);
        bool isSelected = (shop->selectedIndex == i);

        Color slotBg = isSelected ? (Color){180, 160, 120, 255} :
                       isHovered ? (Color){200, 180, 140, 255} : PARCHMENT_DARK;
        DrawRectangle(itemX, itemY, ITEM_SIZE, ITEM_SIZE, slotBg);
        DrawRectangleLines(itemX, itemY, ITEM_SIZE, ITEM_SIZE, PARCHMENT_BORDER);

        // Draw item icon (centered)
        DrawItemIcon(shop->items[i].item, itemX + ITEM_SIZE / 2, itemY + ITEM_SIZE / 2);

        // Item name below
        const char* itemName = ITEM_NAMES[shop->items[i].item];
        int nameW = MeasureText(itemName, 12);
        DrawText(itemName, itemX + (ITEM_SIZE - nameW) / 2, itemY + ITEM_SIZE + 5, 12, PARCHMENT_TEXT);

        // Price below name
        char priceText[32];
        snprintf(priceText, sizeof(priceText), "%d gp", shop->items[i].price);
        int priceW = MeasureText(priceText, 14);
        DrawText(priceText, itemX + (ITEM_SIZE - priceW) / 2, itemY + ITEM_SIZE + 20, 14, GOLD_TEXT);
    }

    // Selected item details and buy button
    if (shop->selectedIndex >= 0) {
        int detailY = itemY + ITEM_SIZE + 50;
        const ShopItem& selected = shop->items[shop->selectedIndex];

        // Show weapon stats
        float cooldown = GetWeaponCooldown(selected.item);
        float dmgMult = GetWeaponDamageMultiplier(selected.item);
        char statsText[128];
        snprintf(statsText, sizeof(statsText), "Speed: %.2fs  |  Damage: x%.1f", cooldown, dmgMult);
        int statsW = MeasureText(statsText, 16);
        DrawText(statsText, BOX_X + (BOX_WIDTH - statsW) / 2, detailY, 16, PARCHMENT_TEXT);

        // Buy button
        bool canAfford = (gilCount >= selected.price);
        int btnW = 120, btnH = 35;
        int btnX = BOX_X + (BOX_WIDTH - btnW) / 2;
        int btnY = detailY + 35;

        bool btnHover = (mouse.x >= btnX && mouse.x <= btnX + btnW &&
                         mouse.y >= btnY && mouse.y <= btnY + btnH);
        Color btnColor = !canAfford ? (Color){100, 100, 100, 255} :
                         btnHover ? (Color){100, 160, 100, 255} : (Color){80, 130, 80, 255};
        DrawRectangle(btnX, btnY, btnW, btnH, btnColor);
        DrawRectangleLines(btnX, btnY, btnW, btnH, PARCHMENT_BORDER);

        const char* buyText = canAfford ? "Buy" : "Need more gold";
        int buyTextSize = canAfford ? 18 : 14;
        int buyW = MeasureText(buyText, buyTextSize);
        DrawText(buyText, btnX + (btnW - buyW) / 2, btnY + (btnH - buyTextSize) / 2, buyTextSize, WHITE);
    }

    // Close hint
    const char* closeHint = "Press ESC to close";
    int closeW = MeasureText(closeHint, 14);
    DrawText(closeHint, BOX_X + (BOX_WIDTH - closeW) / 2, BOX_Y + BOX_HEIGHT - 30, 14, PARCHMENT_TEXT);
}

// Minimap constants
static const int MINIMAP_SIZE = 160;
static const int MINIMAP_MARGIN = 10;
static const int MINIMAP_TOP_OFFSET = 35;  // Below FPS counter
static const float MINIMAP_RADIUS = 75.0f;  // World units visible from center

int GetMinimapHeight() {
    // Returns Y position where inventory should start (below minimap)
    // Minimap starts at: MINIMAP_MARGIN + MINIMAP_TOP_OFFSET = 45
    // Minimap ends at: 45 + MINIMAP_SIZE = 205
    // Add margin below for inventory
    return MINIMAP_MARGIN + MINIMAP_TOP_OFFSET + MINIMAP_SIZE + 15;
}

void DrawMinimap(Vector3 playerPos, float playerYaw,
                 const Enemy* enemies, int enemyCount,
                 const NPC* npcs, int npcCount,
                 const Tree* trees, int treeCount,
                 const Wall* walls, int wallCount,
                 int screenWidth, int screenHeight) {
    (void)screenHeight;  // Unused

    // Minimap position (top right)
    int mapX = screenWidth - MINIMAP_SIZE - MINIMAP_MARGIN;
    int mapY = MINIMAP_MARGIN + MINIMAP_TOP_OFFSET;  // Below FPS counter
    int centerX = mapX + MINIMAP_SIZE / 2;
    int centerY = mapY + MINIMAP_SIZE / 2;

    // Scale factor: world units to pixels
    float scale = (MINIMAP_SIZE / 2.0f) / MINIMAP_RADIUS;

    // Draw background (dark with border)
    DrawRectangle(mapX - 2, mapY - 2, MINIMAP_SIZE + 4, MINIMAP_SIZE + 4, (Color){60, 50, 40, 255});
    DrawRectangle(mapX, mapY, MINIMAP_SIZE, MINIMAP_SIZE, (Color){30, 35, 25, 220});

    // Clip to minimap bounds
    BeginScissorMode(mapX, mapY, MINIMAP_SIZE, MINIMAP_SIZE);

    // Draw walls (gray rectangles)
    for (int i = 0; i < wallCount; i++) {
        float dx = walls[i].position.x - playerPos.x;
        float dz = walls[i].position.z - playerPos.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > MINIMAP_RADIUS + 20.0f) continue;  // Skip far walls

        int wx = centerX + (int)(dx * scale);
        int wy = centerY + (int)(dz * scale);  // Z is forward in world
        int ww = (int)(walls[i].width * scale);
        int wd = (int)(walls[i].depth * scale);
        if (ww < 2) ww = 2;
        if (wd < 2) wd = 2;
        DrawRectangle(wx - ww/2, wy - wd/2, ww, wd, (Color){100, 100, 100, 180});
    }

    // Draw trees (small green circles for normal, darker for oak)
    for (int i = 0; i < treeCount; i++) {
        if (!trees[i].alive) continue;
        float dx = trees[i].position.x - playerPos.x;
        float dz = trees[i].position.z - playerPos.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > MINIMAP_RADIUS) continue;

        int tx = centerX + (int)(dx * scale);
        int ty = centerY + (int)(dz * scale);
        Color treeColor = (trees[i].type == TREE_OAK) ?
            (Color){30, 80, 30, 200} : (Color){50, 120, 50, 200};
        int radius = (trees[i].type == TREE_OAK) ? 4 : 3;
        DrawCircle(tx, ty, radius, treeColor);
    }

    // Draw NPCs (green dots)
    for (int i = 0; i < npcCount; i++) {
        if (!npcs[i].active) continue;
        float dx = npcs[i].position.x - playerPos.x;
        float dz = npcs[i].position.z - playerPos.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > MINIMAP_RADIUS) continue;

        int nx = centerX + (int)(dx * scale);
        int ny = centerY + (int)(dz * scale);
        DrawCircle(nx, ny, 4, (Color){100, 200, 100, 255});
    }

    // Draw enemies (red dots)
    for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].alive) continue;
        float dx = enemies[i].position.x - playerPos.x;
        float dz = enemies[i].position.z - playerPos.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > MINIMAP_RADIUS) continue;

        int ex = centerX + (int)(dx * scale);
        int ey = centerY + (int)(dz * scale);
        DrawCircle(ex, ey, 4, (Color){200, 60, 60, 255});
    }

    // Draw player (yellow triangle pointing in facing direction)
    // Player yaw: 0 = facing +Z (south), PI/2 = facing +X (east)
    float arrowLen = 8.0f;
    float arrowAngle = playerYaw;  // Yaw already in radians
    // Triangle points in direction player is facing
    float tipX = centerX + sinf(arrowAngle) * arrowLen;
    float tipY = centerY + cosf(arrowAngle) * arrowLen;
    float backAngle = arrowAngle + PI;
    float sideOffset = 5.0f;
    float leftX = centerX + sinf(backAngle - 0.5f) * sideOffset;
    float leftY = centerY + cosf(backAngle - 0.5f) * sideOffset;
    float rightX = centerX + sinf(backAngle + 0.5f) * sideOffset;
    float rightY = centerY + cosf(backAngle + 0.5f) * sideOffset;
    DrawTriangle(
        (Vector2){tipX, tipY},
        (Vector2){leftX, leftY},
        (Vector2){rightX, rightY},
        (Color){255, 220, 50, 255}
    );

    EndScissorMode();

    // Draw border
    DrawRectangleLinesEx((Rectangle){(float)(mapX - 2), (float)(mapY - 2),
                                      (float)(MINIMAP_SIZE + 4), (float)(MINIMAP_SIZE + 4)},
                          2, (Color){120, 100, 80, 255});

    // Label
    DrawText("Map", mapX, mapY - 18, 16, (Color){200, 180, 140, 255});

    // Compass directions
    DrawText("N", centerX - 4, mapY + 4, 12, (Color){180, 160, 140, 180});
    DrawText("S", centerX - 4, mapY + MINIMAP_SIZE - 14, 12, (Color){180, 160, 140, 180});
}
