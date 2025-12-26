#include "menu_system.h"
#include "hud.h"
#include "inventory.h"
#include <cstring>

// ============================================================================
// PARCHMENT UI COLORS (matching hud.cpp style)
// ============================================================================
static const Color PARCHMENT_BG = { 222, 198, 158, 240 };
static const Color PARCHMENT_BORDER = { 139, 90, 43, 255 };
static const Color PARCHMENT_DARK = { 180, 150, 100, 255 };
static const Color PARCHMENT_TEXT = { 60, 40, 20, 255 };
static const Color GOLD_TEXT = { 255, 204, 0, 255 };

// ============================================================================
// INITIALIZATION
// ============================================================================

void InitMenuSystem(MenuSystem* menu,
                    bool* inventoryActive,
                    DialogueState* dialogue,
                    ShopState* shop,
                    TimeSelectMenu* timeSelect,
                    HelpSystem* help,
                    MonsterGenerator* generator) {
    menu->inventoryActive = inventoryActive;
    menu->dialogue = dialogue;
    menu->shop = shop;
    menu->timeSelect = timeSelect;
    menu->help = help;
    menu->generator = generator;

    // Initialize bank state
    menu->bank.active = false;
    menu->bank.selectedBankSlot = -1;
    menu->bank.selectedInvSlot = -1;
}

// ============================================================================
// QUERY FUNCTIONS
// ============================================================================

bool IsAnyMenuOpen(const MenuSystem* menu) {
    if (menu->inventoryActive && *menu->inventoryActive) return true;
    if (menu->dialogue && menu->dialogue->active) return true;
    if (menu->shop && menu->shop->active) return true;
    if (menu->timeSelect && menu->timeSelect->active) return true;
    if (menu->help && menu->help->state != HelpState::CLOSED) return true;
    if (menu->bank.active) return true;
    if (menu->generator && IsGeneratorOpen(menu->generator)) return true;
    return false;
}

bool CanProcessGameInput(const MenuSystem* menu) {
    // Game input (movement, camera, attacks) is blocked by any menu
    if (menu->inventoryActive && *menu->inventoryActive) return false;
    if (menu->dialogue && menu->dialogue->active) return false;
    if (menu->shop && menu->shop->active) return false;
    if (menu->timeSelect && menu->timeSelect->active) return false;
    if (menu->help && menu->help->state != HelpState::CLOSED) return false;
    if (menu->bank.active) return false;
    if (menu->generator && IsGeneratorOpen(menu->generator)) return false;
    return true;
}

bool CanProcessWorldInteraction(const MenuSystem* menu) {
    // World interaction (E key, pickups) is blocked by dialogue, shop, bank, help, generator
    if (menu->dialogue && menu->dialogue->active) return false;
    if (menu->shop && menu->shop->active) return false;
    if (menu->help && menu->help->state != HelpState::CLOSED) return false;
    if (menu->bank.active) return false;
    if (menu->generator && IsGeneratorOpen(menu->generator)) return false;
    return true;
}

bool CanProcessScreenshotKey(const MenuSystem* menu) {
    // Screenshot key (P) works except during help UI or generator (might want to type P)
    if (menu->help && menu->help->state == HelpState::TYPING) return false;
    if (menu->generator && menu->generator->state == GeneratorState::TYPING) return false;
    return true;
}

bool CanProcessHotkeys(const MenuSystem* menu) {
    // Hotkeys (0 reload, T time, H help, G generator) blocked during certain menus
    if (menu->dialogue && menu->dialogue->active) return false;
    if (menu->shop && menu->shop->active) return false;
    if (menu->help && menu->help->state != HelpState::CLOSED) return false;
    if (menu->bank.active) return false;
    if (menu->generator && IsGeneratorOpen(menu->generator)) return false;
    return true;
}

// ============================================================================
// MENU CONTROL
// ============================================================================

void OpenMenu(MenuSystem* menu, MenuType type) {
    switch (type) {
        case MenuType::INVENTORY:
            // Inventory is controlled externally via SHIFT key
            break;
        case MenuType::DIALOGUE:
            if (menu->dialogue) menu->dialogue->active = true;
            break;
        case MenuType::SHOP:
            if (menu->shop) menu->shop->active = true;
            break;
        case MenuType::TIME_SELECT:
            if (menu->timeSelect) menu->timeSelect->active = true;
            break;
        case MenuType::HELP:
            // Help is controlled via UpdateHelpSystem
            break;
        case MenuType::BANKING:
            OpenBank(menu);
            break;
        case MenuType::MONSTER_GENERATOR:
            if (menu->generator) OpenMonsterGenerator(menu->generator);
            break;
        default:
            break;
    }
    EnableCursor();
}

void CloseMenu(MenuSystem* menu, MenuType type) {
    switch (type) {
        case MenuType::INVENTORY:
            // Inventory is controlled externally via SHIFT key
            break;
        case MenuType::DIALOGUE:
            if (menu->dialogue) {
                menu->dialogue->active = false;
                menu->dialogue->npcIndex = -1;
                menu->dialogue->currentLine = 0;
            }
            break;
        case MenuType::SHOP:
            if (menu->shop) {
                menu->shop->active = false;
                menu->shop->selectedIndex = -1;
            }
            break;
        case MenuType::TIME_SELECT:
            if (menu->timeSelect) menu->timeSelect->active = false;
            break;
        case MenuType::HELP:
            if (menu->help) menu->help->state = HelpState::CLOSED;
            break;
        case MenuType::BANKING:
            CloseBank(menu);
            break;
        case MenuType::MONSTER_GENERATOR:
            if (menu->generator) CloseMonsterGenerator(menu->generator);
            break;
        default:
            break;
    }

    // Disable cursor if no menus are open
    if (!IsAnyMenuOpen(menu)) {
        DisableCursor();
    }
}

bool HandleMenuEscape(MenuSystem* menu) {
    // Close menus in priority order (topmost first)
    // Help UI has highest priority
    if (menu->help && menu->help->state != HelpState::CLOSED) {
        menu->help->state = HelpState::CLOSED;
        if (!IsAnyMenuOpen(menu)) DisableCursor();
        return true;
    }

    // Monster generator
    if (menu->generator && IsGeneratorOpen(menu->generator)) {
        CloseMonsterGenerator(menu->generator);
        if (!IsAnyMenuOpen(menu)) DisableCursor();
        return true;
    }

    // Bank
    if (menu->bank.active) {
        CloseBank(menu);
        if (!IsAnyMenuOpen(menu)) DisableCursor();
        return true;
    }

    // Shop
    if (menu->shop && menu->shop->active) {
        CloseMenu(menu, MenuType::SHOP);
        return true;
    }

    // Time select
    if (menu->timeSelect && menu->timeSelect->active) {
        CloseMenu(menu, MenuType::TIME_SELECT);
        return true;
    }

    // Dialogue
    if (menu->dialogue && menu->dialogue->active) {
        CloseMenu(menu, MenuType::DIALOGUE);
        return true;
    }

    // Inventory is controlled by SHIFT, not ESC
    return false;
}

// ============================================================================
// BANKING
// ============================================================================

void OpenBank(MenuSystem* menu) {
    menu->bank.active = true;
    menu->bank.selectedBankSlot = -1;
    menu->bank.selectedInvSlot = -1;
    EnableCursor();
}

void CloseBank(MenuSystem* menu) {
    menu->bank.active = false;
    menu->bank.selectedBankSlot = -1;
    menu->bank.selectedInvSlot = -1;
}

// Helper to find first empty bank slot
static int FindEmptyBankSlot(const PlayerState* player) {
    for (int i = 0; i < BANK_SLOTS; i++) {
        if (player->bank[i] == ITEM_NONE) return i;
    }
    return -1;
}

// Helper to find bank slot with specific item (for stacking)
static int FindBankSlotWithItem(const PlayerState* player, ItemType item) {
    for (int i = 0; i < BANK_SLOTS; i++) {
        if (player->bank[i] == item) return i;
    }
    return -1;
}

// Helper to find first empty inventory slot
static int FindEmptyInvSlot(const PlayerState* player) {
    for (int i = 0; i < INV_SLOTS; i++) {
        if (player->inventory[i] == ITEM_NONE) return i;
    }
    return -1;
}

// Helper to find inventory slot with specific item (for stacking)
static int FindInvSlotWithItem(const PlayerState* player, ItemType item) {
    for (int i = 0; i < INV_SLOTS; i++) {
        if (player->inventory[i] == item) return i;
    }
    return -1;
}

const char* UpdateBankInput(MenuSystem* menu, PlayerState* player, int screenWidth, int screenHeight) {
    if (!menu->bank.active) return nullptr;

    const int BOX_WIDTH = 500;
    const int BOX_HEIGHT = 410;  // Must match DrawBankUI
    const int BOX_X = (screenWidth - BOX_WIDTH) / 2;
    const int BOX_Y = (screenHeight - BOX_HEIGHT) / 2;
    const int PADDING = 20;
    const int SLOT_SIZE = 40;
    const int SLOT_PADDING = 4;

    Vector2 mouse = GetMousePosition();

    // Bank slots area
    int bankGridX = BOX_X + (BOX_WIDTH - BANK_COLS * (SLOT_SIZE + SLOT_PADDING)) / 2;
    int bankGridY = BOX_Y + 60;

    // Handle bank slot clicks
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // Check bank slots
        for (int row = 0; row < BANK_ROWS; row++) {
            for (int col = 0; col < BANK_COLS; col++) {
                int slot = row * BANK_COLS + col;
                int slotX = bankGridX + col * (SLOT_SIZE + SLOT_PADDING);
                int slotY = bankGridY + row * (SLOT_SIZE + SLOT_PADDING);

                if (mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
                    mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE) {
                    if (player->bank[slot] != ITEM_NONE) {
                        menu->bank.selectedBankSlot = slot;
                        menu->bank.selectedInvSlot = -1;
                    }
                }
            }
        }

        // Check inventory slots (right side panel - use real inventory position)
        int invX, invY;
        GetInventoryPosition(screenWidth, &invX, &invY);

        for (int row = 0; row < INV_ROWS; row++) {
            for (int col = 0; col < INV_COLS; col++) {
                int slot = row * INV_COLS + col;
                int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
                int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);

                if (mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
                    mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE) {
                    if (player->inventory[slot] != ITEM_NONE) {
                        menu->bank.selectedInvSlot = slot;
                        menu->bank.selectedBankSlot = -1;
                    }
                }
            }
        }

        // Buttons
        int buttonY = bankGridY + BANK_ROWS * (SLOT_SIZE + SLOT_PADDING) + 15;
        int btnH = 30;
        int btnW = 100;
        int depositBtnX = BOX_X + PADDING;
        int withdrawBtnX = BOX_X + PADDING + btnW + 10;
        int depositAllBtnX = BOX_X + BOX_WIDTH - PADDING - btnW;

        // Deposit button
        if (menu->bank.selectedInvSlot >= 0) {
            if (mouse.x >= depositBtnX && mouse.x <= depositBtnX + btnW &&
                mouse.y >= buttonY && mouse.y <= buttonY + btnH) {
                int invSlot = menu->bank.selectedInvSlot;
                ItemType item = player->inventory[invSlot];
                int count = player->inventoryCount[invSlot];

                // Find or create bank slot
                int bankSlot = -1;
                if (IsItemStackable(item)) {
                    bankSlot = FindBankSlotWithItem(player, item);
                }
                if (bankSlot < 0) {
                    bankSlot = FindEmptyBankSlot(player);
                }

                if (bankSlot >= 0) {
                    if (player->bank[bankSlot] == item) {
                        // Stack with existing
                        player->bankCount[bankSlot] += count;
                    } else {
                        // New slot
                        player->bank[bankSlot] = item;
                        player->bankCount[bankSlot] = count;
                    }
                    // Clear inventory slot
                    player->inventory[invSlot] = ITEM_NONE;
                    player->inventoryCount[invSlot] = 0;
                    menu->bank.selectedInvSlot = -1;
                    return "Item deposited";
                } else {
                    return "Bank is full!";
                }
            }
        }

        // Withdraw button
        if (menu->bank.selectedBankSlot >= 0) {
            if (mouse.x >= withdrawBtnX && mouse.x <= withdrawBtnX + btnW &&
                mouse.y >= buttonY && mouse.y <= buttonY + btnH) {
                int bankSlot = menu->bank.selectedBankSlot;
                ItemType item = player->bank[bankSlot];
                int count = player->bankCount[bankSlot];

                // Find or create inv slot
                int invSlot = -1;
                if (IsItemStackable(item)) {
                    invSlot = FindInvSlotWithItem(player, item);
                }
                if (invSlot < 0) {
                    invSlot = FindEmptyInvSlot(player);
                }

                if (invSlot >= 0) {
                    if (player->inventory[invSlot] == item) {
                        // Stack with existing
                        player->inventoryCount[invSlot] += count;
                    } else {
                        // New slot
                        player->inventory[invSlot] = item;
                        player->inventoryCount[invSlot] = count;
                    }
                    // Clear bank slot
                    player->bank[bankSlot] = ITEM_NONE;
                    player->bankCount[bankSlot] = 0;
                    menu->bank.selectedBankSlot = -1;
                    return "Item withdrawn";
                } else {
                    return "Inventory is full!";
                }
            }
        }

        // Deposit All button
        if (mouse.x >= depositAllBtnX && mouse.x <= depositAllBtnX + btnW &&
            mouse.y >= buttonY && mouse.y <= buttonY + btnH) {
            int deposited = 0;
            for (int i = 0; i < INV_SLOTS; i++) {
                if (player->inventory[i] == ITEM_NONE) continue;

                ItemType item = player->inventory[i];
                int count = player->inventoryCount[i];

                int bankSlot = -1;
                if (IsItemStackable(item)) {
                    bankSlot = FindBankSlotWithItem(player, item);
                }
                if (bankSlot < 0) {
                    bankSlot = FindEmptyBankSlot(player);
                }

                if (bankSlot >= 0) {
                    if (player->bank[bankSlot] == item) {
                        player->bankCount[bankSlot] += count;
                    } else {
                        player->bank[bankSlot] = item;
                        player->bankCount[bankSlot] = count;
                    }
                    player->inventory[i] = ITEM_NONE;
                    player->inventoryCount[i] = 0;
                    deposited++;
                }
            }
            menu->bank.selectedInvSlot = -1;
            if (deposited > 0) {
                return "All items deposited";
            }
        }
    }

    return nullptr;
}

void DrawBankUI(const MenuSystem* menu, const PlayerState* player, int screenWidth, int screenHeight) {
    if (!menu->bank.active) return;

    const int BOX_WIDTH = 500;
    const int BOX_HEIGHT = 410;  // Just bank + buttons + close hint
    const int BOX_X = (screenWidth - BOX_WIDTH) / 2;
    const int BOX_Y = (screenHeight - BOX_HEIGHT) / 2;
    const int PADDING = 20;
    const int SLOT_SIZE = 40;
    const int SLOT_PADDING = 4;

    // Draw parchment background
    DrawRectangle(BOX_X - 4, BOX_Y - 4, BOX_WIDTH + 8, BOX_HEIGHT + 8, PARCHMENT_BORDER);
    DrawRectangle(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, PARCHMENT_BG);
    DrawRectangleLines(BOX_X + 6, BOX_Y + 6, BOX_WIDTH - 12, BOX_HEIGHT - 12, PARCHMENT_DARK);

    // Title
    const char* title = "Bank";
    int titleW = MeasureText(title, 24);
    DrawText(title, BOX_X + (BOX_WIDTH - titleW) / 2, BOX_Y + PADDING, 24, PARCHMENT_BORDER);

    // Separator
    DrawRectangle(BOX_X + PADDING, BOX_Y + 50, BOX_WIDTH - PADDING * 2, 2, PARCHMENT_BORDER);

    // Bank grid
    int bankGridX = BOX_X + (BOX_WIDTH - BANK_COLS * (SLOT_SIZE + SLOT_PADDING)) / 2;
    int bankGridY = BOX_Y + 60;

    Vector2 mouse = GetMousePosition();

    // Track hovered item for tooltip
    const char* hoveredItemName = nullptr;

    for (int row = 0; row < BANK_ROWS; row++) {
        for (int col = 0; col < BANK_COLS; col++) {
            int slot = row * BANK_COLS + col;
            int slotX = bankGridX + col * (SLOT_SIZE + SLOT_PADDING);
            int slotY = bankGridY + row * (SLOT_SIZE + SLOT_PADDING);

            bool isHovered = (mouse.x >= slotX && mouse.x <= slotX + SLOT_SIZE &&
                              mouse.y >= slotY && mouse.y <= slotY + SLOT_SIZE);
            bool isSelected = (menu->bank.selectedBankSlot == slot);

            Color slotBg = isSelected ? (Color){180, 160, 120, 255} :
                           isHovered ? (Color){200, 180, 140, 255} : PARCHMENT_DARK;
            DrawRectangle(slotX, slotY, SLOT_SIZE, SLOT_SIZE, slotBg);
            DrawRectangleLines(slotX, slotY, SLOT_SIZE, SLOT_SIZE, PARCHMENT_BORDER);

            if (player->bank[slot] != ITEM_NONE) {
                DrawItemIcon(player->bank[slot], slotX + SLOT_SIZE / 2, slotY + SLOT_SIZE / 2);

                // Track hover for tooltip
                if (isHovered) {
                    hoveredItemName = ITEM_NAMES[player->bank[slot]];
                }

                // Stack count
                if (player->bankCount[slot] > 1) {
                    char countText[16];
                    snprintf(countText, sizeof(countText), "%d", player->bankCount[slot]);
                    DrawText(countText, slotX + 2, slotY + 2, 10, GOLD_TEXT);
                }
            }
        }
    }

    // Buttons row
    int buttonY = bankGridY + BANK_ROWS * (SLOT_SIZE + SLOT_PADDING) + 15;
    int btnW = 100, btnH = 30;
    int depositBtnX = BOX_X + PADDING;
    int withdrawBtnX = BOX_X + PADDING + btnW + 10;
    int depositAllBtnX = BOX_X + BOX_WIDTH - PADDING - btnW;

    // Deposit button
    bool depositHover = (mouse.x >= depositBtnX && mouse.x <= depositBtnX + btnW &&
                         mouse.y >= buttonY && mouse.y <= buttonY + btnH);
    bool depositEnabled = (menu->bank.selectedInvSlot >= 0);
    Color depositColor = !depositEnabled ? (Color){100, 100, 100, 255} :
                         depositHover ? (Color){100, 160, 100, 255} : (Color){80, 130, 80, 255};
    DrawRectangle(depositBtnX, buttonY, btnW, btnH, depositColor);
    DrawRectangleLines(depositBtnX, buttonY, btnW, btnH, PARCHMENT_BORDER);
    const char* depositText = "Deposit";
    int depositTextW = MeasureText(depositText, 14);
    DrawText(depositText, depositBtnX + (btnW - depositTextW) / 2, buttonY + 8, 14, WHITE);

    // Withdraw button
    bool withdrawHover = (mouse.x >= withdrawBtnX && mouse.x <= withdrawBtnX + btnW &&
                          mouse.y >= buttonY && mouse.y <= buttonY + btnH);
    bool withdrawEnabled = (menu->bank.selectedBankSlot >= 0);
    Color withdrawColor = !withdrawEnabled ? (Color){100, 100, 100, 255} :
                          withdrawHover ? (Color){100, 100, 160, 255} : (Color){80, 80, 130, 255};
    DrawRectangle(withdrawBtnX, buttonY, btnW, btnH, withdrawColor);
    DrawRectangleLines(withdrawBtnX, buttonY, btnW, btnH, PARCHMENT_BORDER);
    const char* withdrawText = "Withdraw";
    int withdrawTextW = MeasureText(withdrawText, 14);
    DrawText(withdrawText, withdrawBtnX + (btnW - withdrawTextW) / 2, buttonY + 8, 14, WHITE);

    // Deposit All button
    bool depositAllHover = (mouse.x >= depositAllBtnX && mouse.x <= depositAllBtnX + btnW &&
                            mouse.y >= buttonY && mouse.y <= buttonY + btnH);
    Color depositAllColor = depositAllHover ? (Color){160, 140, 100, 255} : (Color){130, 110, 80, 255};
    DrawRectangle(depositAllBtnX, buttonY, btnW, btnH, depositAllColor);
    DrawRectangleLines(depositAllBtnX, buttonY, btnW, btnH, PARCHMENT_BORDER);
    const char* depositAllText = "Deposit All";
    int depositAllTextW = MeasureText(depositAllText, 12);
    DrawText(depositAllText, depositAllBtnX + (btnW - depositAllTextW) / 2, buttonY + 9, 12, WHITE);

    // Hovered item tooltip (show bank item name when hovering) - above buttons
    if (hoveredItemName) {
        int tooltipW = MeasureText(hoveredItemName, 16);
        int tooltipY = buttonY - 20;
        DrawText(hoveredItemName, BOX_X + (BOX_WIDTH - tooltipW) / 2, tooltipY, 16, GOLD_TEXT);
    }

    // Draw selection highlight on inventory panel (right side)
    if (menu->bank.selectedInvSlot >= 0) {
        int invX, invY;
        GetInventoryPosition(screenWidth, &invX, &invY);
        int row = menu->bank.selectedInvSlot / INV_COLS;
        int col = menu->bank.selectedInvSlot % INV_COLS;
        int slotX = invX + col * (SLOT_SIZE + SLOT_PADDING);
        int slotY = invY + row * (SLOT_SIZE + SLOT_PADDING);
        // Draw green highlight border
        DrawRectangleLines(slotX - 2, slotY - 2, SLOT_SIZE + 4, SLOT_SIZE + 4, GREEN);
        DrawRectangleLines(slotX - 1, slotY - 1, SLOT_SIZE + 2, SLOT_SIZE + 2, GREEN);
    }

    // Close hint (below buttons)
    const char* closeHint = "Press ESC to close";
    int closeW = MeasureText(closeHint, 14);
    DrawText(closeHint, BOX_X + (BOX_WIDTH - closeW) / 2, buttonY + btnH + 10, 14, PARCHMENT_TEXT);
}
