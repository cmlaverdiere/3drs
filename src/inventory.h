#ifndef INVENTORY_H
#define INVENTORY_H

#include "raylib.h"
#include "types.h"
#include "player.h"

// Inventory menu state
struct InventoryMenu {
    bool showContextMenu = false;
    int contextSlot = -1;
    int menuX = 0;
    int menuY = 0;

    // Drag-and-swap state
    bool isDragging = false;
    int dragSlot = -1;        // Slot being dragged from
    int dragStartX = 0;       // Mouse position when drag started
    int dragStartY = 0;
};

// Handle inventory input (mouse mode clicks)
// Returns message to display (or nullptr)
const char* HandleInventoryInput(PlayerState* state, PlayerRuntime* runtime,
                                 InventoryMenu* menu,
                                 WorldItem* worldItems, int* worldItemCount,
                                 Vector3 playerPos,
                                 XPPopup* xpPopups, LevelUpNotification* levelUpNotif,
                                 int screenWidth, int screenHeight);

// Handle action menu input for world items
// Returns true if item was picked up
bool HandleItemPickup(PlayerState* state, WorldItem* targetItem);

// Try to add item to inventory (handles stacking)
// Returns true if added successfully
bool AddToInventory(PlayerState* state, ItemType item);

// Drop item from inventory slot
void DropFromInventory(PlayerState* state, int slot,
                       WorldItem* worldItems, int* worldItemCount,
                       Vector3 dropPos);

// Get inventory UI position
void GetInventoryPosition(int screenWidth, int* outX, int* outY);

// Check if player has at least 1 of item
bool HasItem(const PlayerState* state, ItemType item);

// Remove one item from inventory (returns false if not found)
bool RemoveItem(PlayerState* state, ItemType item);

// Add gil to inventory (handles stacking)
bool AddGil(PlayerState* state, int amount);

#endif
