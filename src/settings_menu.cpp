#include "settings_menu.h"
#include "script_input.h"

// Same palette as the time menu (hud.cpp)
static const Color FRAME = {60, 50, 40, 255};
static const Color PANEL = {40, 35, 30, 240};
static const Color TITLE = {220, 200, 160, 255};
static const Color HINT = {160, 150, 130, 255};

bool DrawSettingsMenu(SettingsMenu* menu, bool* classic, int screenWidth, int screenHeight) {
    if (!menu->active) return false;
    const int W = 320, H = 150, PAD = 14;
    const int x = (screenWidth - W) / 2, y = (screenHeight - H) / 2;
    DrawRectangle(x - 3, y - 3, W + 6, H + 6, FRAME);
    DrawRectangle(x, y, W, H, PANEL);
    const char* title = "Settings";
    DrawText(title, x + (W - MeasureText(title, 18)) / 2, y + PAD, 18, TITLE);

    // Classic graphics toggle
    Rectangle row = {(float)(x + PAD), (float)(y + 50), (float)(W - 2 * PAD), 34};
    Vector2 mouse = Game_GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, row);
    DrawRectangleRec(row, hover ? (Color){80, 68, 52, 255} : (Color){60, 52, 42, 255});
    DrawRectangleLinesEx(row, 1, FRAME);
    DrawText("Classic graphics", (int)row.x + 10, (int)row.y + 9, 16, TITLE);
    Rectangle box = {row.x + row.width - 64, row.y + 6, 54, 22};
    DrawRectangleRec(box, *classic ? (Color){90, 150, 70, 255} : (Color){90, 80, 70, 255});
    const char* state = *classic ? "ON" : "OFF";
    DrawText(state, (int)(box.x + (box.width - MeasureText(state, 14)) / 2), (int)box.y + 4, 14, RAYWHITE);
    DrawText("Flat colours and shading like the original OSRS", x + PAD, y + 92, 12, HINT);
    DrawText("M or ESC to close", x + PAD, y + H - 22, 12, HINT);

    if (hover && Game_IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        *classic = !*classic;
        return true;
    }
    return false;
}
