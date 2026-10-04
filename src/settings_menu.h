#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

// Settings menu (M): graphics options
struct SettingsMenu {
    bool active;
};

// Draws the menu and handles clicks. Returns true when an option changed.
// classic: classic (flat-shaded) graphics toggle, flipped on click.
bool DrawSettingsMenu(SettingsMenu* menu, bool* classic, int screenWidth, int screenHeight);

#endif // SETTINGS_MENU_H
