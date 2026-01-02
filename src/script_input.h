#ifndef SCRIPT_INPUT_H
#define SCRIPT_INPUT_H

#include "raylib.h"
#include "types.h"
#include <vector>

// Forward declarations
struct LightingSystem;
struct ScriptState;

// ============================================================================
// SCRIPT COMMAND TYPES
// ============================================================================

enum class ScriptCommandType {
    NONE = 0,
    // Input commands
    PRESS_KEY,       // Press and release key (1 frame)
    HOLD_KEY,        // Hold key for N frames
    RELEASE_KEY,     // Release a held key
    CLICK,           // Mouse click at position
    MOVE_MOUSE,      // Move mouse cursor
    // Timing
    WAIT,            // Wait N frames
    // Player manipulation
    WARP,            // Teleport player to position
    FACE,            // Set camera yaw/pitch
    LOOK_AT,         // Point camera at world position
    // Screenshots
    SCREENSHOT,      // Capture screenshot with label
    // State manipulation
    SET_TIME,        // Set time of day
    SET_SEASON,      // Set current season
    GIVE_ITEM,       // Add item to inventory
    EQUIP,           // Equip weapon
    SET_HP,          // Set player health
};

// ============================================================================
// SCRIPT COMMAND STRUCTURE
// ============================================================================

struct ScriptCommand {
    ScriptCommandType type = ScriptCommandType::NONE;
    int keyCode = 0;              // For key commands
    int x = 0, y = 0;             // For mouse/position commands
    int frames = 0;               // For hold/wait
    float fx = 0, fy = 0, fz = 0; // For warp/look_at
    float value = 0;              // For set_time
    int intValue = 0;             // For season, item type, etc.
    int intValue2 = 0;            // For set_hp (max), give_item (count)
    char label[64] = {};          // For screenshot label
    bool rightClick = false;      // For click commands
};

// ============================================================================
// SCRIPT STATE
// ============================================================================

struct ScriptState {
    std::vector<ScriptCommand> commands;
    int currentCommand = 0;
    int frameCounter = 0;         // Frames remaining for current timed command
    bool finished = false;
    bool failed = false;
    char failReason[256] = {};

    // Input override state
    bool keysPressed[512] = {};   // Keys pressed this frame
    bool keysHeld[512] = {};      // Keys currently held
    Vector2 mousePosition = {};
    bool mouseLeftPressed = false;
    bool mouseRightPressed = false;
    Vector2 mouseDelta = {};      // For camera control

    // Queued state changes (applied next frame)
    bool pendingWarp = false;
    Vector3 warpPosition = {};
    bool pendingFace = false;
    float faceYaw = 0, facePitch = 0;
};

// ============================================================================
// SCRIPT FUNCTIONS
// ============================================================================

// Load script from file
// Returns true on success, false on failure
bool LoadScript(ScriptState* state, const char* filename);

// Process one frame of script execution
// Updates camera/player/lighting based on commands
// Returns true if script is still running, false when finished
bool UpdateScript(ScriptState* state, Camera3D* camera, PlayerState* player,
                  LightingSystem* lighting, int screenWidth, int screenHeight);

// ============================================================================
// INPUT WRAPPER FUNCTIONS
// Call these instead of Raylib functions when in script mode
// ============================================================================

bool Script_IsKeyPressed(ScriptState* state, int key);
bool Script_IsKeyDown(ScriptState* state, int key);
Vector2 Script_GetMousePosition(ScriptState* state);
Vector2 Script_GetMouseDelta(ScriptState* state);
bool Script_IsMouseButtonPressed(ScriptState* state, int button);
bool Script_IsMouseButtonReleased(ScriptState* state, int button);

// ============================================================================
// GLOBAL SCRIPT STATE AND INLINE WRAPPERS
// Use these throughout the codebase instead of direct Raylib calls
// ============================================================================

// Global script state pointer (nullptr when not in script mode)
extern ScriptState* g_activeScript;

// Inline wrappers that check for script mode
inline bool Game_IsKeyPressed(int key) {
    if (g_activeScript) return Script_IsKeyPressed(g_activeScript, key);
    return IsKeyPressed(key);
}

inline bool Game_IsKeyDown(int key) {
    if (g_activeScript) return Script_IsKeyDown(g_activeScript, key);
    return IsKeyDown(key);
}

inline Vector2 Game_GetMousePosition() {
    if (g_activeScript) return Script_GetMousePosition(g_activeScript);
    return GetMousePosition();
}

inline Vector2 Game_GetMouseDelta() {
    if (g_activeScript) return Script_GetMouseDelta(g_activeScript);
    return GetMouseDelta();
}

inline bool Game_IsMouseButtonPressed(int button) {
    if (g_activeScript) return Script_IsMouseButtonPressed(g_activeScript, button);
    return IsMouseButtonPressed(button);
}

inline bool Game_IsMouseButtonReleased(int button) {
    if (g_activeScript) return Script_IsMouseButtonReleased(g_activeScript, button);
    return IsMouseButtonReleased(button);
}

#endif // SCRIPT_INPUT_H
