#include "script_input.h"
#include "math_utils.h"
#include "lighting.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sys/stat.h>

// Global script state pointer (nullptr when not in script mode)
ScriptState* g_activeScript = nullptr;

// ============================================================================
// KEY NAME PARSING
// ============================================================================

static int ParseKeyName(const char* name) {
    // Letters
    if (strcmp(name, "W") == 0) return KEY_W;
    if (strcmp(name, "A") == 0) return KEY_A;
    if (strcmp(name, "S") == 0) return KEY_S;
    if (strcmp(name, "D") == 0) return KEY_D;
    if (strcmp(name, "E") == 0) return KEY_E;
    if (strcmp(name, "R") == 0) return KEY_R;
    if (strcmp(name, "P") == 0) return KEY_P;
    if (strcmp(name, "H") == 0) return KEY_H;
    if (strcmp(name, "G") == 0) return KEY_G;
    if (strcmp(name, "T") == 0) return KEY_T;
    if (strcmp(name, "Y") == 0) return KEY_Y;
    if (strcmp(name, "N") == 0) return KEY_N;

    // Special keys
    if (strcmp(name, "SPACE") == 0) return KEY_SPACE;
    if (strcmp(name, "ESCAPE") == 0 || strcmp(name, "ESC") == 0) return KEY_ESCAPE;
    if (strcmp(name, "SHIFT") == 0) return KEY_LEFT_SHIFT;
    if (strcmp(name, "LEFT_SHIFT") == 0) return KEY_LEFT_SHIFT;
    if (strcmp(name, "RIGHT_SHIFT") == 0) return KEY_RIGHT_SHIFT;

    // Numbers
    if (strcmp(name, "ZERO") == 0 || strcmp(name, "0") == 0) return KEY_ZERO;
    if (strcmp(name, "ONE") == 0 || strcmp(name, "1") == 0) return KEY_ONE;
    if (strcmp(name, "TWO") == 0 || strcmp(name, "2") == 0) return KEY_TWO;
    if (strcmp(name, "THREE") == 0 || strcmp(name, "3") == 0) return KEY_THREE;

    // Mouse buttons (stored as negative values to distinguish from keyboard)
    if (strcmp(name, "LMB") == 0) return -MOUSE_BUTTON_LEFT - 1;
    if (strcmp(name, "RMB") == 0) return -MOUSE_BUTTON_RIGHT - 1;

    return -1000; // Invalid
}

// ============================================================================
// SEASON NAME PARSING
// ============================================================================

static int ParseSeasonName(const char* name) {
    if (strcmp(name, "spring") == 0) return SEASON_SPRING;
    if (strcmp(name, "summer") == 0) return SEASON_SUMMER;
    if (strcmp(name, "autumn") == 0) return SEASON_AUTUMN;
    if (strcmp(name, "winter") == 0) return SEASON_WINTER;
    return -1;
}

// ============================================================================
// ITEM TYPE PARSING
// ============================================================================

static int ParseItemType(const char* name) {
    // Common items - support both numeric and name-based
    if (strcmp(name, "BRONZE_SHORTSWORD") == 0) return ITEM_BRONZE_SHORTSWORD;
    if (strcmp(name, "BRONZE_AXE") == 0) return ITEM_BRONZE_AXE;
    if (strcmp(name, "IRON_2H_SWORD") == 0) return ITEM_IRON_2H_SWORD;
    if (strcmp(name, "STEEL_SCIMITAR") == 0) return ITEM_STEEL_SCIMITAR;
    if (strcmp(name, "MITHRIL_SCIMITAR") == 0) return ITEM_MITHRIL_SCIMITAR;
    if (strcmp(name, "ADAMANT_SCIMITAR") == 0) return ITEM_ADAMANT_SCIMITAR;
    if (strcmp(name, "BRONZE_PICKAXE") == 0) return ITEM_BRONZE_PICKAXE;
    if (strcmp(name, "BOW") == 0) return ITEM_BOW;
    if (strcmp(name, "ARROW") == 0) return ITEM_ARROW;
    if (strcmp(name, "GIL") == 0) return ITEM_GIL;
    if (strcmp(name, "LOGS") == 0) return ITEM_LOGS;
    if (strcmp(name, "OAK_LOGS") == 0) return ITEM_OAK_LOGS;
    if (strcmp(name, "BONES") == 0) return ITEM_BONES;
    if (strcmp(name, "COW_HIDE") == 0) return ITEM_COW_HIDE;
    if (strcmp(name, "CHITIN") == 0) return ITEM_CHITIN;

    // Try numeric
    int val = atoi(name);
    if (val > 0 && val < ITEM_COUNT) return val;

    return ITEM_NONE;
}

// ============================================================================
// LOAD SCRIPT FROM FILE
// ============================================================================

bool LoadScript(ScriptState* state, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("ERROR: Cannot open script file: %s\n", filename);
        return false;
    }

    // Reset state
    state->commands.clear();
    state->currentCommand = 0;
    state->frameCounter = 0;
    state->finished = false;
    state->failed = false;
    memset(state->keysPressed, 0, sizeof(state->keysPressed));
    memset(state->keysHeld, 0, sizeof(state->keysHeld));
    state->mousePosition = { 0, 0 };
    state->mouseLeftPressed = false;
    state->mouseRightPressed = false;
    state->mouseDelta = { 0, 0 };
    state->pendingWarp = false;
    state->pendingFace = false;

    char line[512];
    int lineNum = 0;

    while (fgets(line, sizeof(line), f)) {
        lineNum++;

        // Skip leading whitespace
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;

        // Skip empty lines and comments
        if (*p == '#' || *p == '\n' || *p == '\0') continue;

        // Remove trailing newline
        char* nl = strchr(p, '\n');
        if (nl) *nl = '\0';

        ScriptCommand cmd = {};
        char arg1[64] = {}, arg2[64] = {}, arg3[64] = {};

        // Parse commands
        if (sscanf(p, "press %63s", arg1) == 1) {
            cmd.type = ScriptCommandType::PRESS_KEY;
            cmd.keyCode = ParseKeyName(arg1);
            if (cmd.keyCode == -1000) {
                printf("WARNING: Unknown key '%s' at line %d\n", arg1, lineNum);
                continue;
            }
        }
        else if (sscanf(p, "hold %63s %d", arg1, &cmd.frames) == 2) {
            cmd.type = ScriptCommandType::HOLD_KEY;
            cmd.keyCode = ParseKeyName(arg1);
            if (cmd.keyCode == -1000) {
                printf("WARNING: Unknown key '%s' at line %d\n", arg1, lineNum);
                continue;
            }
        }
        else if (sscanf(p, "release %63s", arg1) == 1) {
            cmd.type = ScriptCommandType::RELEASE_KEY;
            cmd.keyCode = ParseKeyName(arg1);
        }
        else if (sscanf(p, "click %d %d %63s", &cmd.x, &cmd.y, arg1) >= 2) {
            cmd.type = ScriptCommandType::CLICK;
            cmd.rightClick = (strcmp(arg1, "right") == 0);
        }
        else if (sscanf(p, "move %d %d", &cmd.x, &cmd.y) == 2) {
            cmd.type = ScriptCommandType::MOVE_MOUSE;
        }
        else if (sscanf(p, "wait %d", &cmd.frames) == 1) {
            cmd.type = ScriptCommandType::WAIT;
        }
        else if (sscanf(p, "wait_seconds %f", &cmd.value) == 1) {
            cmd.type = ScriptCommandType::WAIT;
            cmd.frames = (int)(cmd.value * 60.0f);  // 60 fps
        }
        else if (sscanf(p, "warp %f %f %f", &cmd.fx, &cmd.fy, &cmd.fz) == 3) {
            cmd.type = ScriptCommandType::WARP;
        }
        else if (sscanf(p, "face %f %f", &cmd.fx, &cmd.fy) == 2) {
            cmd.type = ScriptCommandType::FACE;
        }
        else if (sscanf(p, "look_at %f %f %f", &cmd.fx, &cmd.fy, &cmd.fz) == 3) {
            cmd.type = ScriptCommandType::LOOK_AT;
        }
        else if (sscanf(p, "screenshot %63s", cmd.label) == 1) {
            cmd.type = ScriptCommandType::SCREENSHOT;
        }
        else if (strncmp(p, "screenshot", 10) == 0 && (p[10] == '\0' || p[10] == ' ' || p[10] == '\n')) {
            cmd.type = ScriptCommandType::SCREENSHOT;
            snprintf(cmd.label, sizeof(cmd.label), "auto_%d", (int)state->commands.size());
        }
        else if (sscanf(p, "set_time %f", &cmd.value) == 1) {
            cmd.type = ScriptCommandType::SET_TIME;
        }
        else if (sscanf(p, "set_season %63s", arg1) == 1) {
            cmd.type = ScriptCommandType::SET_SEASON;
            cmd.intValue = ParseSeasonName(arg1);
            if (cmd.intValue < 0) {
                printf("WARNING: Unknown season '%s' at line %d\n", arg1, lineNum);
                continue;
            }
        }
        else if (sscanf(p, "give_item %63s %d", arg1, &cmd.intValue2) >= 1) {
            cmd.type = ScriptCommandType::GIVE_ITEM;
            cmd.intValue = ParseItemType(arg1);
            if (cmd.intValue2 == 0) cmd.intValue2 = 1;
        }
        else if (sscanf(p, "equip %63s", arg1) == 1) {
            cmd.type = ScriptCommandType::EQUIP;
            cmd.intValue = ParseItemType(arg1);
        }
        else if (sscanf(p, "set_hp %d %d", &cmd.intValue, &cmd.intValue2) == 2) {
            cmd.type = ScriptCommandType::SET_HP;
        }
        else {
            printf("WARNING: Unknown command at line %d: %s\n", lineNum, p);
            continue;
        }

        if (cmd.type != ScriptCommandType::NONE) {
            state->commands.push_back(cmd);
        }
    }

    fclose(f);

    printf("SCRIPT: Loaded %zu commands from %s\n", state->commands.size(), filename);
    return !state->commands.empty();
}

// ============================================================================
// UPDATE SCRIPT (CALLED EACH FRAME)
// ============================================================================

// External season variable (defined in main.cpp)
extern Season g_currentSeason;

bool UpdateScript(ScriptState* state, Camera3D* camera, PlayerState* player,
                  LightingSystem* lighting, int screenWidth, int screenHeight) {
    if (state->finished || state->failed) return false;

    // Clear per-frame input state
    memset(state->keysPressed, 0, sizeof(state->keysPressed));
    state->mouseLeftPressed = false;
    state->mouseRightPressed = false;
    state->mouseDelta = { 0, 0 };

    // Apply pending warp
    if (state->pendingWarp) {
        float terrainY = GetTerrainHeight(state->warpPosition.x, state->warpPosition.z);
        camera->position.x = state->warpPosition.x;
        camera->position.y = terrainY + PLAYER_EYE_HEIGHT;
        camera->position.z = state->warpPosition.z;

        // Update target to maintain current look direction
        Vector3 lookDir = {
            camera->target.x - camera->position.x,
            camera->target.y - camera->position.y,
            camera->target.z - camera->position.z
        };
        float len = sqrtf(lookDir.x*lookDir.x + lookDir.y*lookDir.y + lookDir.z*lookDir.z);
        if (len > 0.01f) {
            lookDir.x /= len; lookDir.y /= len; lookDir.z /= len;
            camera->target.x = camera->position.x + lookDir.x;
            camera->target.y = camera->position.y + lookDir.y;
            camera->target.z = camera->position.z + lookDir.z;
        }

        printf("SCRIPT: Warped to (%.1f, %.1f, %.1f)\n",
               camera->position.x, camera->position.y, camera->position.z);
        state->pendingWarp = false;
    }

    // Apply pending face direction
    if (state->pendingFace) {
        float yaw = state->faceYaw * DEG2RAD;
        float pitch = state->facePitch * DEG2RAD;

        // Clamp pitch
        if (pitch > 89.0f * DEG2RAD) pitch = 89.0f * DEG2RAD;
        if (pitch < -89.0f * DEG2RAD) pitch = -89.0f * DEG2RAD;

        camera->target.x = camera->position.x + sinf(yaw) * cosf(pitch);
        camera->target.y = camera->position.y + sinf(pitch);
        camera->target.z = camera->position.z + cosf(yaw) * cosf(pitch);

        printf("SCRIPT: Facing yaw=%.1f pitch=%.1f\n", state->faceYaw, state->facePitch);
        state->pendingFace = false;
    }

    // Process wait timer
    if (state->frameCounter > 0) {
        state->frameCounter--;
        return true;
    }

    // Process commands until we hit a blocking command or finish
    while (state->currentCommand < (int)state->commands.size()) {
        ScriptCommand& cmd = state->commands[state->currentCommand];

        switch (cmd.type) {
            case ScriptCommandType::PRESS_KEY: {
                // Handle mouse button "keys"
                if (cmd.keyCode < 0 && cmd.keyCode > -1000) {
                    int mouseBtn = -(cmd.keyCode + 1);
                    if (mouseBtn == MOUSE_BUTTON_LEFT) {
                        state->mouseLeftPressed = true;
                    } else if (mouseBtn == MOUSE_BUTTON_RIGHT) {
                        state->mouseRightPressed = true;
                    }
                } else if (cmd.keyCode >= 0) {
                    state->keysPressed[cmd.keyCode] = true;
                    state->keysHeld[cmd.keyCode] = true;
                }
                printf("SCRIPT: Press %d\n", cmd.keyCode);
                state->currentCommand++;
                // Need to process input this frame, then release next frame
                state->frameCounter = 1;
                return true;
            }

            case ScriptCommandType::HOLD_KEY: {
                if (cmd.keyCode < 0 && cmd.keyCode > -1000) {
                    // Mouse button hold not supported well, just press
                    int mouseBtn = -(cmd.keyCode + 1);
                    if (mouseBtn == MOUSE_BUTTON_LEFT) {
                        state->mouseLeftPressed = true;
                    }
                } else if (cmd.keyCode >= 0) {
                    state->keysHeld[cmd.keyCode] = true;
                    state->keysPressed[cmd.keyCode] = true;
                }
                printf("SCRIPT: Hold key %d for %d frames\n", cmd.keyCode, cmd.frames);
                state->frameCounter = cmd.frames - 1;  // -1 because this frame counts
                state->currentCommand++;
                return true;
            }

            case ScriptCommandType::RELEASE_KEY: {
                if (cmd.keyCode >= 0) {
                    state->keysHeld[cmd.keyCode] = false;
                }
                printf("SCRIPT: Release key %d\n", cmd.keyCode);
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::CLICK: {
                state->mousePosition = { (float)cmd.x, (float)cmd.y };
                if (cmd.rightClick) {
                    state->mouseRightPressed = true;
                } else {
                    state->mouseLeftPressed = true;
                }
                printf("SCRIPT: Click at (%d, %d)%s\n", cmd.x, cmd.y,
                       cmd.rightClick ? " (right)" : "");
                state->currentCommand++;
                return true;
            }

            case ScriptCommandType::MOVE_MOUSE: {
                state->mousePosition = { (float)cmd.x, (float)cmd.y };
                printf("SCRIPT: Move mouse to (%d, %d)\n", cmd.x, cmd.y);
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::WAIT: {
                printf("SCRIPT: Wait %d frames\n", cmd.frames);
                state->frameCounter = cmd.frames;
                state->currentCommand++;
                return true;
            }

            case ScriptCommandType::WARP: {
                state->pendingWarp = true;
                state->warpPosition = { cmd.fx, cmd.fy, cmd.fz };
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::FACE: {
                state->pendingFace = true;
                state->faceYaw = cmd.fx;
                state->facePitch = cmd.fy;
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::LOOK_AT: {
                Vector3 target = { cmd.fx, cmd.fy, cmd.fz };
                Vector3 dir = {
                    target.x - camera->position.x,
                    target.y - camera->position.y,
                    target.z - camera->position.z
                };
                float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
                if (len > 0.01f) {
                    camera->target = target;
                }
                printf("SCRIPT: Look at (%.1f, %.1f, %.1f)\n", cmd.fx, cmd.fy, cmd.fz);
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::SCREENSHOT: {
                time_t now = time(nullptr);
                struct tm* t = localtime(&now);
                char filename[128];
                snprintf(filename, sizeof(filename),
                         "screenshots/%s_%04d%02d%02d_%02d%02d%02d.png",
                         cmd.label,
                         t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                         t->tm_hour, t->tm_min, t->tm_sec);

                Image screenshot = LoadImageFromScreen();
                ExportImage(screenshot, filename);
                UnloadImage(screenshot);
                printf("SCREENSHOT: %s\n", filename);
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::SET_TIME: {
                if (lighting) {
                    lighting->timeOfDay = cmd.value;
                    // Clamp to valid range
                    if (lighting->timeOfDay < 0.0f) lighting->timeOfDay = 0.0f;
                    if (lighting->timeOfDay > 1.0f) lighting->timeOfDay = 1.0f;
                }
                printf("SCRIPT: Set time to %.2f\n", cmd.value);
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::SET_SEASON: {
                g_currentSeason = (Season)cmd.intValue;
                printf("SCRIPT: Set season to %d\n", cmd.intValue);
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::GIVE_ITEM: {
                if (player) {
                    for (int i = 0; i < INV_SLOTS; i++) {
                        if (player->inventory[i] == ITEM_NONE) {
                            player->inventory[i] = (ItemType)cmd.intValue;
                            player->inventoryCount[i] = cmd.intValue2;
                            printf("SCRIPT: Gave item %d x%d\n", cmd.intValue, cmd.intValue2);
                            break;
                        }
                    }
                }
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::EQUIP: {
                if (player) {
                    player->equippedWeapon = (ItemType)cmd.intValue;
                    printf("SCRIPT: Equipped item %d\n", cmd.intValue);
                }
                state->currentCommand++;
                break;
            }

            case ScriptCommandType::SET_HP: {
                if (player) {
                    player->currentHP = cmd.intValue;
                    player->maxHP = cmd.intValue2;
                    printf("SCRIPT: Set HP to %d/%d\n", cmd.intValue, cmd.intValue2);
                }
                state->currentCommand++;
                break;
            }

            default:
                state->currentCommand++;
                break;
        }
    }

    // All commands processed
    state->finished = true;
    printf("SCRIPT: Finished all commands\n");
    return false;
}

// ============================================================================
// INPUT WRAPPER FUNCTIONS
// ============================================================================

bool Script_IsKeyPressed(ScriptState* state, int key) {
    if (key >= 0 && key < 512) {
        return state->keysPressed[key];
    }
    return false;
}

bool Script_IsKeyDown(ScriptState* state, int key) {
    if (key >= 0 && key < 512) {
        return state->keysHeld[key];
    }
    return false;
}

Vector2 Script_GetMousePosition(ScriptState* state) {
    return state->mousePosition;
}

Vector2 Script_GetMouseDelta(ScriptState* state) {
    return state->mouseDelta;
}

bool Script_IsMouseButtonPressed(ScriptState* state, int button) {
    if (button == MOUSE_BUTTON_LEFT) return state->mouseLeftPressed;
    if (button == MOUSE_BUTTON_RIGHT) return state->mouseRightPressed;
    return false;
}

bool Script_IsMouseButtonReleased(ScriptState* state, int button) {
    // In script mode, we don't track button release explicitly
    // Return false since scripts typically just press/click
    return false;
}
