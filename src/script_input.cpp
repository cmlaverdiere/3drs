#include "script_input.h"
#include "math_utils.h"
#include "lighting.h"
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sys/stat.h>

// Global script state pointer (nullptr when not in script mode)
ScriptState* g_activeScript = nullptr;

// ============================================================================
// KEY NAME PARSING
// ============================================================================

struct KeyName { const char* name; int code; };
static const KeyName KEY_NAMES[] = {
    {"W", KEY_W}, {"A", KEY_A}, {"S", KEY_S}, {"D", KEY_D}, {"E", KEY_E}, {"R", KEY_R}, {"P", KEY_P},
    {"H", KEY_H}, {"G", KEY_G}, {"T", KEY_T}, {"Y", KEY_Y}, {"N", KEY_N}, {"M", KEY_M},
    {"SPACE", KEY_SPACE}, {"ESCAPE", KEY_ESCAPE}, {"ESC", KEY_ESCAPE}, {"SHIFT", KEY_LEFT_SHIFT},
    {"LEFT_SHIFT", KEY_LEFT_SHIFT}, {"RIGHT_SHIFT", KEY_RIGHT_SHIFT}, {"SLASH", KEY_SLASH},
    {"ZERO", KEY_ZERO}, {"0", KEY_ZERO}, {"ONE", KEY_ONE}, {"1", KEY_ONE}, {"TWO", KEY_TWO}, {"2", KEY_TWO},
    {"THREE", KEY_THREE}, {"3", KEY_THREE},
    // Mouse buttons (stored as negative values to distinguish from keyboard)
    {"LMB", -MOUSE_BUTTON_LEFT - 1}, {"RMB", -MOUSE_BUTTON_RIGHT - 1},
};

static int ParseKeyName(const char* name) {
    for (const KeyName& k : KEY_NAMES) {
        if (strcmp(name, k.name) == 0) return k.code;
    }
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

struct ItemName { const char* name; ItemType type; };
static const ItemName ITEM_NAMES_SCRIPT[] = {
    {"BRONZE_SHORTSWORD", ITEM_BRONZE_SHORTSWORD}, {"BRONZE_AXE", ITEM_BRONZE_AXE},
    {"IRON_2H_SWORD", ITEM_IRON_2H_SWORD}, {"STEEL_SCIMITAR", ITEM_STEEL_SCIMITAR},
    {"MITHRIL_SCIMITAR", ITEM_MITHRIL_SCIMITAR}, {"ADAMANT_SCIMITAR", ITEM_ADAMANT_SCIMITAR},
    {"BRONZE_PICKAXE", ITEM_BRONZE_PICKAXE}, {"BOW", ITEM_BOW}, {"ARROW", ITEM_ARROW}, {"GIL", ITEM_GIL},
    {"LOGS", ITEM_LOGS}, {"OAK_LOGS", ITEM_OAK_LOGS}, {"BONES", ITEM_BONES}, {"COW_HIDE", ITEM_COW_HIDE},
    {"CHITIN", ITEM_CHITIN},
};

static int ParseItemType(const char* name) {
    for (const ItemName& it : ITEM_NAMES_SCRIPT) {
        if (strcasecmp(name, it.name) == 0) return it.type;
    }
    // Try numeric
    int val = atoi(name);
    if (val > 0 && val < ITEM_COUNT) return val;

    return ITEM_NONE;
}

// ============================================================================
// COMMAND TABLE (scripts and the console)
// ============================================================================

static const ScriptCommandInfo SCRIPT_COMMANDS[] = {
    {"press", "press <key>", "key", true},
    {"hold", "hold <key> <frames>", "key", true},
    {"release", "release <key>", "key", true},
    {"click", "click <x> <y> [right]", nullptr, true},
    {"move", "move <x> <y>", nullptr, true},
    {"wait", "wait <frames>", nullptr, true},
    {"wait_seconds", "wait_seconds <s>", nullptr, true},
    {"warp", "warp <x> <y> <z>", nullptr, false},
    {"face", "face <yaw> <pitch>", nullptr, false},
    {"look_at", "look_at <x> <y> <z>", nullptr, false},
    {"screenshot", "screenshot [label]", nullptr, false},
    {"set_time", "set_time <0-1>", nullptr, false},
    {"set_season", "set_season <season>", "season", false},
    {"give_item", "give_item <item> [count]", "item", false},
    {"equip", "equip <item>", "item", false},
    {"set_hp", "set_hp <hp> <max>", nullptr, false},
    {"classic", "classic <on|off|toggle>", "toggle", false},
};

const ScriptCommandInfo* GetScriptCommands(int* count) {
    *count = (int)(sizeof(SCRIPT_COMMANDS) / sizeof(SCRIPT_COMMANDS[0]));
    return SCRIPT_COMMANDS;
}

int GetScriptArgCompletions(const char* kind, const char** out, int max) {
    static const char* SEASONS[] = {"spring", "summer", "autumn", "winter"};
    static const char* TOGGLES[] = {"on", "off", "toggle"};
    int n = 0;
    if (!kind) return 0;
    if (strcmp(kind, "key") == 0) {
        for (const KeyName& k : KEY_NAMES) if (n < max) out[n++] = k.name;
    } else if (strcmp(kind, "item") == 0) {
        for (const ItemName& it : ITEM_NAMES_SCRIPT) if (n < max) out[n++] = it.name;
    } else if (strcmp(kind, "season") == 0) {
        for (const char* v : SEASONS) if (n < max) out[n++] = v;
    } else if (strcmp(kind, "toggle") == 0) {
        for (const char* v : TOGGLES) if (n < max) out[n++] = v;
    }
    return n;
}

// ============================================================================
// PARSE ONE COMMAND LINE (scripts and the console)
// ============================================================================

bool ParseScriptCommand(const char* p, ScriptCommand* cmd, char* err, int errLen) {
    *cmd = {};
    err[0] = '\0';
    char arg1[64] = {};

    // Parse commands
    if (sscanf(p, "press %63s", arg1) == 1) {
        cmd->type = ScriptCommandType::PRESS_KEY;
        cmd->keyCode = ParseKeyName(arg1);
        if (cmd->keyCode == -1000) {
            snprintf(err, errLen, "Unknown key '%s'", arg1);
            return false;
        }
    }
    else if (sscanf(p, "hold %63s %d", arg1, &cmd->frames) == 2) {
        cmd->type = ScriptCommandType::HOLD_KEY;
        cmd->keyCode = ParseKeyName(arg1);
        if (cmd->keyCode == -1000) {
            snprintf(err, errLen, "Unknown key '%s'", arg1);
            return false;
        }
    }
    else if (sscanf(p, "release %63s", arg1) == 1) {
        cmd->type = ScriptCommandType::RELEASE_KEY;
        cmd->keyCode = ParseKeyName(arg1);
    }
    else if (sscanf(p, "click %d %d %63s", &cmd->x, &cmd->y, arg1) >= 2) {
        cmd->type = ScriptCommandType::CLICK;
        cmd->rightClick = (strcmp(arg1, "right") == 0);
    }
    else if (sscanf(p, "move %d %d", &cmd->x, &cmd->y) == 2) {
        cmd->type = ScriptCommandType::MOVE_MOUSE;
    }
    else if (sscanf(p, "wait %d", &cmd->frames) == 1) {
        cmd->type = ScriptCommandType::WAIT;
    }
    else if (sscanf(p, "wait_seconds %f", &cmd->value) == 1) {
        cmd->type = ScriptCommandType::WAIT;
        cmd->frames = (int)(cmd->value * 60.0f);  // 60 fps
    }
    else if (sscanf(p, "warp %f %f %f", &cmd->fx, &cmd->fy, &cmd->fz) == 3) {
        cmd->type = ScriptCommandType::WARP;
    }
    else if (sscanf(p, "face %f %f", &cmd->fx, &cmd->fy) == 2) {
        cmd->type = ScriptCommandType::FACE;
    }
    else if (sscanf(p, "look_at %f %f %f", &cmd->fx, &cmd->fy, &cmd->fz) == 3) {
        cmd->type = ScriptCommandType::LOOK_AT;
    }
    else if (sscanf(p, "screenshot %63s", cmd->label) == 1) {
        cmd->type = ScriptCommandType::SCREENSHOT;
    }
    else if (strncmp(p, "screenshot", 10) == 0 && (p[10] == '\0' || p[10] == ' ' || p[10] == '\n')) {
        cmd->type = ScriptCommandType::SCREENSHOT;
        cmd->label[0] = '\0';
    }
    else if (sscanf(p, "set_time %f", &cmd->value) == 1) {
        cmd->type = ScriptCommandType::SET_TIME;
    }
    else if (sscanf(p, "set_season %63s", arg1) == 1) {
        cmd->type = ScriptCommandType::SET_SEASON;
        cmd->intValue = ParseSeasonName(arg1);
        if (cmd->intValue < 0) {
            snprintf(err, errLen, "Unknown season '%s'", arg1);
            return false;
        }
    }
    else if (sscanf(p, "give_item %63s %d", arg1, &cmd->intValue2) >= 1) {
        cmd->type = ScriptCommandType::GIVE_ITEM;
        cmd->intValue = ParseItemType(arg1);
        if (cmd->intValue2 == 0) cmd->intValue2 = 1;
    }
    else if (sscanf(p, "equip %63s", arg1) == 1) {
        cmd->type = ScriptCommandType::EQUIP;
        cmd->intValue = ParseItemType(arg1);
    }
    else if (sscanf(p, "set_hp %d %d", &cmd->intValue, &cmd->intValue2) == 2) {
        cmd->type = ScriptCommandType::SET_HP;
    }
    else if (sscanf(p, "classic %63s", arg1) == 1) {
        cmd->type = ScriptCommandType::SET_CLASSIC;
        if (strcmp(arg1, "on") == 0) cmd->intValue = 1;
        else if (strcmp(arg1, "off") == 0) cmd->intValue = 0;
        else if (strcmp(arg1, "toggle") == 0) cmd->intValue = -1;
        else {
            snprintf(err, errLen, "classic expects on, off or toggle");
            return false;
        }
    }
    else {
        snprintf(err, errLen, "Unknown command: %s", p);
        return false;
    }
    return true;
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
        char err[160];
        if (!ParseScriptCommand(p, &cmd, err, sizeof(err))) {
            printf("WARNING: %s at line %d\n", err, lineNum);
            continue;
        }
        if (cmd.type == ScriptCommandType::SCREENSHOT && cmd.label[0] == '\0') {
            snprintf(cmd.label, sizeof(cmd.label), "auto_%d", (int)state->commands.size());
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
                extern float g_frameTimeMsAvg;
                printf("PERF: %s frame_ms=%.2f fps=%d\n", cmd.label, g_frameTimeMsAvg, GetFPS());
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

            case ScriptCommandType::SET_CLASSIC: {
                if (lighting) {
                    lighting->classicMode = cmd.intValue < 0 ? !lighting->classicMode : cmd.intValue != 0;
                    if (player) player->classicGraphics = lighting->classicMode;
                    printf("SCRIPT: Classic graphics %s\n", lighting->classicMode ? "on" : "off");
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
// RUN ONE COMMAND IMMEDIATELY (console)
// ============================================================================

bool ExecuteScriptCommandNow(const ScriptCommand& cmd, Camera3D* camera, PlayerState* player,
                             LightingSystem* lighting, int screenWidth, int screenHeight) {
    switch (cmd.type) {
        case ScriptCommandType::PRESS_KEY: case ScriptCommandType::HOLD_KEY: case ScriptCommandType::RELEASE_KEY:
        case ScriptCommandType::CLICK: case ScriptCommandType::MOVE_MOUSE: case ScriptCommandType::WAIT:
            return false;   // input and timing commands only make sense in scripts
        default:
            break;
    }
    // Run it through the script interpreter; a second step applies a queued warp/face
    ScriptState state;
    state.commands.push_back(cmd);
    if (cmd.type == ScriptCommandType::SCREENSHOT && cmd.label[0] == '\0') {
        snprintf(state.commands[0].label, sizeof(state.commands[0].label), "console");
    }
    UpdateScript(&state, camera, player, lighting, screenWidth, screenHeight);
    state.finished = false;
    UpdateScript(&state, camera, player, lighting, screenWidth, screenHeight);
    return true;
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
