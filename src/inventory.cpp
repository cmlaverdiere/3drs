#include "inventory.h"
#include "combat.h"
#include "sound_system.h"

void GetInventoryPosition(int screenWidth, int* outX, int* outY) {
    *outX = screenWidth - (INV_COLS * (SLOT_SIZE + SLOT_PADDING)) - 20;
    *outY = 60;
}

static bool IsClickInSlot(Vector2 mouse, int slotX, int slotY) {
    return mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
           mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE;
}

const char* HandleInventoryInput(PlayerState* state, PlayerRuntime* runtime,
                                 InventoryMenu* menu,
                                 WorldItem* worldItems, int* worldItemCount,
                                 Vector3 playerPos,
                                 XPPopup* xpPopups, LevelUpNotification* levelUpNotif,
                                 int screenWidth, int screenHeight) {
    static char messageBuffer[128];
    const char* message = nullptr;

    Vector2 mouse = GetMousePosition();
    int invX, invY;
    GetInventoryPosition(screenWidth, &invX, &invY);

    // Handle context menu clicks
    if (menu->showContextMenu && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

    // Right-click to open context menu
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        for (int row = 0; row < INV_ROWS; row++) {
            for (int col = 0; col < INV_COLS; col++) {
                int slotIdx = row * INV_COLS + col;
                int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
                int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);

                if (IsClickInSlot(mouse, slotX, slotY)) {
                    if (state->inventory[slotIdx] != ITEM_NONE) {
                        menu->showContextMenu = true;
                        menu->contextSlot = slotIdx;
                        menu->menuX = (int)mouse.x;
                        menu->menuY = (int)mouse.y;

                        // Clamp to screen
                        if (menu->menuX + 80 > screenWidth) menu->menuX = screenWidth - 80;
                        if (menu->menuY + 80 > screenHeight) menu->menuY = screenHeight - 80;
                    }
                    return message;
                }
            }
        }
        menu->showContextMenu = false;
        return message;
    }

    // Left-click on inventory (quick actions)
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !menu->showContextMenu && !runtime->isDucking) {
        for (int row = 0; row < INV_ROWS; row++) {
            for (int col = 0; col < INV_COLS; col++) {
                int slotIdx = row * INV_COLS + col;
                int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
                int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);

                if (IsClickInSlot(mouse, slotX, slotY)) {
                    ItemType clickedItem = state->inventory[slotIdx];

                    if (IsWeapon(clickedItem)) {
                        // Toggle equip
                        state->equippedWeapon = (state->equippedWeapon == clickedItem) ? ITEM_NONE : clickedItem;
                    } else if (clickedItem == ITEM_BONES) {
                        // Bury bones
                        runtime->isDucking = true;
                        runtime->duckTimer = DUCK_DURATION;
                        PlaySoundEffect(SFX_BURY);

                        state->inventory[slotIdx] = ITEM_NONE;
                        state->inventoryCount[slotIdx] = 0;

                        AwardSkillXP(state, SKILL_PRAYER, BURY_XP, xpPopups, levelUpNotif);
                    }
                    return message;
                }
            }
        }
    }

    return message;
}

bool HandleItemPickup(PlayerState* state, WorldItem* targetItem) {
    if (AddToInventory(state, targetItem->type)) {
        targetItem->pickedUp = true;
        PlaySoundEffect(SFX_PICKUP);
        return true;
    }
    return false;
}

bool AddToInventory(PlayerState* state, ItemType item) {
    // For stackable items, try to add to existing stack first
    if (IsItemStackable(item)) {
        for (int i = 0; i < INV_SLOTS; i++) {
            if (state->inventory[i] == item) {
                state->inventoryCount[i]++;
                return true;
            }
        }
    }

    // Find empty slot
    for (int i = 0; i < INV_SLOTS; i++) {
        if (state->inventory[i] == ITEM_NONE) {
            state->inventory[i] = item;
            state->inventoryCount[i] = 1;
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
    (*worldItemCount)++;

    if (state->equippedWeapon == item) {
        state->equippedWeapon = ITEM_NONE;
    }
    state->inventory[slot] = ITEM_NONE;
    state->inventoryCount[slot] = 0;
}
