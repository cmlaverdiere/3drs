#include "inventory.h"
#include "combat.h"
#include "sound_system.h"
#include "hud.h"
#include "script_input.h"
#include <cmath>

void GetInventoryPosition(int screenWidth, int* outX, int* outY) {
    // Align right edge with minimap (same margin from screen edge)
    int invWidth = INV_COLS * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING;
    *outX = screenWidth - invWidth - 5;
    *outY = GetMinimapHeight() + 20;  // Position below minimap with gap
}

static bool IsClickInSlot(Vector2 mouse, int slotX, int slotY) {
    return mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
           mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE;
}

// Minimum drag distance before it's considered a drag (not a click)
static const int MIN_DRAG_DISTANCE = 5;

// Get slot index at mouse position (-1 if none)
static int GetSlotAtPosition(Vector2 mouse, int invX, int invY) {
    for (int row = 0; row < INV_ROWS; row++) {
        for (int col = 0; col < INV_COLS; col++) {
            int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
            int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);
            if (IsClickInSlot(mouse, slotX, slotY)) {
                return row * INV_COLS + col;
            }
        }
    }
    return -1;
}

// Swap two inventory slots (items and counts)
static void SwapInventorySlots(PlayerState* state, int slot1, int slot2) {
    ItemType tempItem = state->inventory[slot1];
    int tempCount = state->inventoryCount[slot1];

    state->inventory[slot1] = state->inventory[slot2];
    state->inventoryCount[slot1] = state->inventoryCount[slot2];

    state->inventory[slot2] = tempItem;
    state->inventoryCount[slot2] = tempCount;
}

const char* HandleInventoryInput(PlayerState* state, PlayerRuntime* runtime,
                                 InventoryMenu* menu,
                                 WorldItem* worldItems, int* worldItemCount,
                                 Vector3 playerPos,
                                 XPPopup* xpPopups, LevelUpNotification* levelUpNotif,
                                 int screenWidth, int screenHeight) {
    static char messageBuffer[128];
    const char* message = nullptr;

    Vector2 mouse = Game_GetMousePosition();
    int invX, invY;
    GetInventoryPosition(screenWidth, &invX, &invY);

    // Handle context menu clicks
    if (menu->showContextMenu && Game_IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const int menuWidth = 80;
        const int menuItemHeight = 20;
        ItemType menuItem = state->inventory[menu->contextSlot];
        bool isWeapon = IsWeapon(menuItem);

        int optionCount = isWeapon ? 4 : 3;
        int menuHeight = menuItemHeight * optionCount;

        if (mouse.x >= menu->menuX && mouse.x <= menu->menuX + menuWidth &&
            mouse.y >= menu->menuY && mouse.y <= menu->menuY + menuHeight) {
            int optionIdx = (int)(mouse.y - menu->menuY) / menuItemHeight;

            if (isWeapon) {
                // Weapon menu: Equip, Examine, Drop, Cancel
                if (optionIdx == 0) {
                    // Equip/Unequip
                    state->equippedWeapon = (state->equippedWeapon == menuItem) ? ITEM_NONE : menuItem;
                } else if (optionIdx == 1) {
                    // Examine
                    snprintf(messageBuffer, sizeof(messageBuffer), "It's a %s.", ITEM_NAMES[menuItem]);
                    message = messageBuffer;
                } else if (optionIdx == 2) {
                    // Drop
                    DropFromInventory(state, menu->contextSlot, worldItems, worldItemCount, playerPos);
                }
                // optionIdx == 3 is Cancel
            } else {
                // Non-weapon: Examine, Drop, Cancel
                if (optionIdx == 0) {
                    // Examine
                    if (IsItemStackable(menuItem) && state->inventoryCount[menu->contextSlot] > 1) {
                        snprintf(messageBuffer, sizeof(messageBuffer), "%d x %s.",
                                 state->inventoryCount[menu->contextSlot], ITEM_NAMES[menuItem]);
                    } else {
                        snprintf(messageBuffer, sizeof(messageBuffer), "It's a %s.", ITEM_NAMES[menuItem]);
                    }
                    message = messageBuffer;
                } else if (optionIdx == 1) {
                    // Drop (one at a time for stackable)
                    if (*worldItemCount < MAX_WORLD_ITEMS) {
                        worldItems[*worldItemCount].type = menuItem;
                        worldItems[*worldItemCount].position = playerPos;
                        worldItems[*worldItemCount].position.y = 0.0f;
                        worldItems[*worldItemCount].pickedUp = false;
                        worldItems[*worldItemCount].canRespawn = false;  // Dropped items don't respawn
                        worldItems[*worldItemCount].respawnTimer = 0.0f;
                        worldItems[*worldItemCount].quantity = 1;  // Context menu drops one at a time
                        (*worldItemCount)++;

                        if (IsItemStackable(menuItem) && state->inventoryCount[menu->contextSlot] > 1) {
                            state->inventoryCount[menu->contextSlot]--;
                        } else {
                            state->inventory[menu->contextSlot] = ITEM_NONE;
                            state->inventoryCount[menu->contextSlot] = 0;
                        }
                    }
                }
                // optionIdx == 2 is Cancel
            }
            menu->showContextMenu = false;
        } else {
            // Clicked outside menu
            menu->showContextMenu = false;
        }
        return message;
    }

    // Right-click to open context menu (cancel any drag)
    if (Game_IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        menu->isDragging = false;
        menu->dragSlot = -1;

        int slotIdx = GetSlotAtPosition(mouse, invX, invY);
        if (slotIdx >= 0 && state->inventory[slotIdx] != ITEM_NONE) {
            menu->showContextMenu = true;
            menu->contextSlot = slotIdx;
            menu->menuX = (int)mouse.x;
            menu->menuY = (int)mouse.y;

            // Clamp to screen
            if (menu->menuX + 80 > screenWidth) menu->menuX = screenWidth - 80;
            if (menu->menuY + 80 > screenHeight) menu->menuY = screenHeight - 80;
        } else {
            menu->showContextMenu = false;
        }
        return message;
    }

    // Left-click press: start dragging if slot has an item
    if (Game_IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !menu->showContextMenu && !runtime->isDucking) {
        int slotIdx = GetSlotAtPosition(mouse, invX, invY);
        if (slotIdx >= 0 && state->inventory[slotIdx] != ITEM_NONE) {
            menu->isDragging = true;
            menu->dragSlot = slotIdx;
            menu->dragStartX = (int)mouse.x;
            menu->dragStartY = (int)mouse.y;
        }
    }

    // Left-click release: handle drag end or click action
    if (Game_IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && menu->isDragging) {
        int dx = (int)mouse.x - menu->dragStartX;
        int dy = (int)mouse.y - menu->dragStartY;
        int dragDistance = (int)sqrtf((float)(dx*dx + dy*dy));

        int targetSlot = GetSlotAtPosition(mouse, invX, invY);

        if (dragDistance >= MIN_DRAG_DISTANCE && targetSlot >= 0 && targetSlot != menu->dragSlot) {
            // It's a drag - swap items
            SwapInventorySlots(state, menu->dragSlot, targetSlot);
        } else if (dragDistance < MIN_DRAG_DISTANCE) {
            // It's a click - perform quick action on the original slot
            ItemType clickedItem = state->inventory[menu->dragSlot];

            if (IsWeapon(clickedItem)) {
                // Toggle equip
                state->equippedWeapon = (state->equippedWeapon == clickedItem) ? ITEM_NONE : clickedItem;
            } else if (clickedItem == ITEM_BONES) {
                // Bury bones
                runtime->isDucking = true;
                runtime->duckTimer = DUCK_DURATION;
                PlaySoundEffect(SFX_BURY);

                state->inventory[menu->dragSlot] = ITEM_NONE;
                state->inventoryCount[menu->dragSlot] = 0;

                AwardSkillXP(state, SKILL_PRAYER, BURY_XP, xpPopups, levelUpNotif);
            }
        }

        // Reset drag state
        menu->isDragging = false;
        menu->dragSlot = -1;
    }

    return message;
}

bool HandleItemPickup(PlayerState* state, WorldItem* targetItem) {
    int quantity = targetItem->quantity > 0 ? targetItem->quantity : 1;
    if (AddToInventory(state, targetItem->type, quantity)) {
        targetItem->pickedUp = true;
        if (targetItem->canRespawn) {
            targetItem->respawnTimer = ITEM_RESPAWN_TIME;
        }
        PlaySoundEffect(SFX_PICKUP);
        return true;
    }
    return false;
}

bool AddToInventory(PlayerState* state, ItemType item, int quantity) {
    // For stackable items, try to add to existing stack first
    if (IsItemStackable(item)) {
        for (int i = 0; i < INV_SLOTS; i++) {
            if (state->inventory[i] == item) {
                state->inventoryCount[i] += quantity;
                return true;
            }
        }
    }

    // Find empty slot
    for (int i = 0; i < INV_SLOTS; i++) {
        if (state->inventory[i] == ITEM_NONE) {
            state->inventory[i] = item;
            state->inventoryCount[i] = quantity;
            return true;
        }
    }

    return false;
}

void DropFromInventory(PlayerState* state, int slot,
                       WorldItem* worldItems, int* worldItemCount,
                       Vector3 dropPos) {
    if (*worldItemCount >= MAX_WORLD_ITEMS) return;

    ItemType item = state->inventory[slot];
    if (item == ITEM_NONE) return;

    worldItems[*worldItemCount].type = item;
    worldItems[*worldItemCount].position = dropPos;
    worldItems[*worldItemCount].position.y = 0.0f;
    worldItems[*worldItemCount].pickedUp = false;
    worldItems[*worldItemCount].canRespawn = false;  // Player drops don't respawn
    worldItems[*worldItemCount].respawnTimer = 0.0f;
    worldItems[*worldItemCount].quantity = state->inventoryCount[slot];
    (*worldItemCount)++;

    if (state->equippedWeapon == item) {
        state->equippedWeapon = ITEM_NONE;
    }
    state->inventory[slot] = ITEM_NONE;
    state->inventoryCount[slot] = 0;
}

bool HasItem(const PlayerState* state, ItemType item) {
    for (int i = 0; i < INV_SLOTS; i++) {
        if (state->inventory[i] == item && state->inventoryCount[i] > 0) {
            return true;
        }
    }
    return false;
}

bool RemoveItem(PlayerState* state, ItemType item) {
    for (int i = 0; i < INV_SLOTS; i++) {
        if (state->inventory[i] == item && state->inventoryCount[i] > 0) {
            if (IsItemStackable(item) && state->inventoryCount[i] > 1) {
                state->inventoryCount[i]--;
            } else {
                state->inventory[i] = ITEM_NONE;
                state->inventoryCount[i] = 0;
            }
            return true;
        }
    }
    return false;
}

bool AddGil(PlayerState* state, int amount) {
    // Find existing gil stack
    for (int i = 0; i < INV_SLOTS; i++) {
        if (state->inventory[i] == ITEM_GIL) {
            state->inventoryCount[i] += amount;
            return true;
        }
    }

    // Find empty slot for new gil stack
    for (int i = 0; i < INV_SLOTS; i++) {
        if (state->inventory[i] == ITEM_NONE) {
            state->inventory[i] = ITEM_GIL;
            state->inventoryCount[i] = amount;
            return true;
        }
    }

    return false;  // No room
}

int GetGilCount(const PlayerState* state) {
    for (int i = 0; i < INV_SLOTS; i++) {
        if (state->inventory[i] == ITEM_GIL) {
            return state->inventoryCount[i];
        }
    }
    return 0;
}

bool RemoveGil(PlayerState* state, int amount) {
    for (int i = 0; i < INV_SLOTS; i++) {
        if (state->inventory[i] == ITEM_GIL) {
            if (state->inventoryCount[i] >= amount) {
                state->inventoryCount[i] -= amount;
                if (state->inventoryCount[i] == 0) {
                    state->inventory[i] = ITEM_NONE;
                }
                return true;
            }
            return false;  // Not enough gil
        }
    }
    return false;  // No gil
}
