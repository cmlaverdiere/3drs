#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include "raylib.h"
#include "types.h"
#include "help_system.h"

// ============================================================================
// MENU SYSTEM
// ============================================================================
// Unified menu management for consistent input handling across all game menus.
// Use query functions instead of checking individual menu states.

// Menu types supported by the unified system
enum class MenuType {
    NONE = 0,
    INVENTORY,      // Shift-held inventory
    DIALOGUE,       // NPC dialogue
    SHOP,           // Shop interface
    TIME_SELECT,    // Time-of-day picker
    HELP,           // Help UI
    BANKING         // Bank storage
};

// Bank constants are defined in types.h (BANK_SLOTS, BANK_COLS, BANK_ROWS)

// Bank state
struct BankState {
    bool active;
    int selectedBankSlot;   // -1 if none
    int selectedInvSlot;    // -1 if none
};

// Unified menu system - holds pointers to all menu states
struct MenuSystem {
    // Pointers to existing menu states (for compatibility)
    bool* inventoryActive;          // Points to mouseMode in main
    DialogueState* dialogue;
    ShopState* shop;
    TimeSelectMenu* timeSelect;
    HelpSystem* help;

    // Banking state (owned by MenuSystem)
    BankState bank;
};

// ============================================================================
// INITIALIZATION
// ============================================================================

// Initialize the menu system with pointers to existing menu states
void InitMenuSystem(MenuSystem* menu,
                    bool* inventoryActive,
                    DialogueState* dialogue,
                    ShopState* shop,
                    TimeSelectMenu* timeSelect,
                    HelpSystem* help);

// ============================================================================
// QUERY FUNCTIONS
// ============================================================================
// Use these instead of checking individual menu states!

// Returns true if ANY menu is open (including inventory)
bool IsAnyMenuOpen(const MenuSystem* menu);

// Returns true if game input should be processed (movement, camera, attacks)
// This is false when any blocking menu is open
bool CanProcessGameInput(const MenuSystem* menu);

// Returns true if world interaction input should be processed (E key, pickups)
bool CanProcessWorldInteraction(const MenuSystem* menu);

// Returns true if the screenshot key (P) should work
bool CanProcessScreenshotKey(const MenuSystem* menu);

// Returns true if hotkeys should work (0 for reload, T for time, etc.)
bool CanProcessHotkeys(const MenuSystem* menu);

// ============================================================================
// MENU CONTROL
// ============================================================================

// Open a specific menu type
void OpenMenu(MenuSystem* menu, MenuType type);

// Close a specific menu type
void CloseMenu(MenuSystem* menu, MenuType type);

// Handle ESC key - closes the topmost menu
// Returns true if ESC was consumed (a menu was closed)
bool HandleMenuEscape(MenuSystem* menu);

// ============================================================================
// BANKING
// ============================================================================

// Open the bank UI
void OpenBank(MenuSystem* menu);

// Close the bank UI
void CloseBank(MenuSystem* menu);

// Handle bank input (mouse clicks, button interactions)
// Returns a status message or nullptr
const char* UpdateBankInput(MenuSystem* menu, PlayerState* player, int screenWidth, int screenHeight);

// Draw the bank UI
void DrawBankUI(const MenuSystem* menu, const PlayerState* player, int screenWidth, int screenHeight);

#endif // MENU_SYSTEM_H
