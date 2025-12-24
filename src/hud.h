#ifndef HUD_H
#define HUD_H

#include "raylib.h"
#include "types.h"
#include "player.h"
#include "inventory.h"

// Draw all HUD elements
void DrawHUD(const Camera3D* camera, const PlayerState* state, const PlayerRuntime* runtime,
             const Enemy* enemies, int enemyCount,
             const DamageIndicator* damageIndicators,
             const XPPopup* xpPopups,
             const LevelUpNotification* levelUpNotif,
             const InventoryMenu* invMenu,
             const WorldItem* targetItem, bool showActionMenu,
             float attackCooldown, float swingTimer,
             bool mouseMode, const char* statusMessage,
             int screenWidth, int screenHeight);

// Draw FPS weapon view
void DrawWeaponView(ItemType weapon, float swingTimer, int screenWidth, int screenHeight);

// Draw inventory UI
void DrawInventoryUI(const PlayerState* state, const InventoryMenu* menu,
                     int screenWidth, int screenHeight);

// Draw floating damage indicators
void DrawDamageIndicators(const Camera3D* camera, const DamageIndicator* indicators,
                          int screenWidth, int screenHeight);

// Draw enemy health bars (when in range)
void DrawEnemyHealthBars(const Camera3D* camera, const Enemy* enemies, int enemyCount,
                         int playerCombatLevel, int screenWidth, int screenHeight);

// Update timers for damage indicators, XP popups, level up notifications
void UpdateHUDTimers(DamageIndicator* damageIndicators,
                     XPPopup* xpPopups,
                     LevelUpNotification* levelUpNotif,
                     float dt);

#endif
