#ifndef CONSOLE_H
#define CONSOLE_H

#include "raylib.h"
#include "types.h"

struct LightingSystem;

// Developer console (/): runs script-engine commands with tab completion
const int CONSOLE_LOG_LINES = 14;
const int CONSOLE_HISTORY = 32;
const int CONSOLE_INPUT_MAX = 200;

struct Console {
    bool active;
    char input[CONSOLE_INPUT_MAX];
    int length;
    char log[CONSOLE_LOG_LINES][160];
    int logCount;
    char history[CONSOLE_HISTORY][CONSOLE_INPUT_MAX];
    int historyCount;
    int historyPos;        // == historyCount when editing a new line
};

void OpenConsole(Console* console);
void CloseConsole(Console* console);
void ConsolePrint(Console* console, const char* text);

// Fills `out` with completions for the token under the cursor (end of input); returns how many
int ConsoleCompletions(const char* input, const char** out, int max);

// Typing, history, tab completion and Enter (runs the command). ESC is handled by the caller.
void UpdateConsole(Console* console, Camera3D* camera, PlayerState* player, LightingSystem* lighting,
                   int screenWidth, int screenHeight);
void DrawConsole(const Console* console, int screenWidth, int screenHeight);

#endif // CONSOLE_H
