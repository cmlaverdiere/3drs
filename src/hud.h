#ifndef HUD_H
#define HUD_H

#include "raylib.h"
#include "types.h"
#include "player.h"
#include "inventory.h"

// Forward declaration for BowState
struct BowState;

// Draw all HUD elements
void DrawHUD(const Camera3D* camera, const PlayerState* state, const PlayerRuntime* runtime,
             const Enemy* enemies, int enemyCount,
             const DamageIndicator* damageIndicators,
             const XPPopup* xpPopups,
             const LevelUpNotification* levelUpNotif,
             const InventoryMenu* invMenu,
             const WorldItem* targetItem, bool showActionMenu,
             float attackCooldown, float swingTimer,
             const BowState* bowState,
             bool mouseMode, const char* statusMessage,
             int screenWidth, int screenHeight);

// Draw FPS weapon view
void DrawWeaponView(ItemType weapon, float swingTimer, const BowState* bowState,
                    int screenWidth, int screenHeight);

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

// Draw NPC dialogue box (parchment style)
// questDialogue: if not null, use these lines instead of NPC's default dialogue
// showAcceptPrompt: if true, show Accept/Decline buttons on last line
void DrawDialogueBox(const DialogueState* dialogue, const NPC* npcs,
                     const char** questDialogue, int questDialogueCount,
                     bool showAcceptPrompt,
                     int screenWidth, int screenHeight);

// Draw NPC interaction prompt (when near an NPC)
void DrawNPCPrompt(const char* npcName, int screenWidth, int screenHeight);

// Draw shop UI
void DrawShopUI(const ShopState* shop, const PlayerState* state,
                int screenWidth, int screenHeight);

// Draw minimap (top right corner)
void DrawMinimap(Vector3 playerPos, float playerYaw,
                 const Enemy* enemies, int enemyCount,
                 const NPC* npcs, int npcCount,
                 const Tree* trees, int treeCount,
                 const Wall* walls, int wallCount,
                 int screenWidth, int screenHeight);

// Get minimap dimensions (for inventory positioning)
int GetMinimapHeight();

// Draw item icon at center position (used by inventory and bank)
void DrawItemIcon(ItemType item, int cx, int cy);

// Draw time and season selection menu and handle clicks
// timeResult: 0 = no change, 1-4 = preset selected (dawn/noon/dusk/midnight)
// seasonResult: -1 = no change, 0-3 = season selected (spring/summer/autumn/winter)
void DrawTimeSelectMenu(TimeSelectMenu* menu, float currentTime, Season currentSeason,
                        int screenWidth, int screenHeight, int* timeResult, int* seasonResult);

#endif
