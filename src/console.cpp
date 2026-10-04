#include "console.h"
#include "script_input.h"
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <cctype>

// Console-only commands, completed alongside the script commands
static const char* CONSOLE_COMMANDS[] = {"help", "clear"};

void OpenConsole(Console* console) {
    console->active = true;
    console->length = 0;
    console->input[0] = '\0';
    console->historyPos = console->historyCount;
    while (GetCharPressed() != 0) {}   // drop the '/' that opened it
    if (console->logCount == 0) ConsolePrint(console, "Script commands run here. Tab completes, 'help' lists them.");
    EnableCursor();
}

void CloseConsole(Console* console) {
    console->active = false;
}

void ConsolePrint(Console* console, const char* text) {
    if (console->logCount == CONSOLE_LOG_LINES) {
        memmove(console->log[0], console->log[1], sizeof(console->log[0]) * (CONSOLE_LOG_LINES - 1));
        console->logCount--;
    }
    snprintf(console->log[console->logCount++], sizeof(console->log[0]), "%s", text);
}

static bool StartsWith(const char* s, const char* prefix, size_t n) {
    return strncasecmp(s, prefix, n) == 0;
}

int ConsoleCompletions(const char* input, const char** out, int max) {
    int count;
    const ScriptCommandInfo* cmds = GetScriptCommands(&count);
    const char* space = strchr(input, ' ');
    int n = 0;
    if (!space) {
        size_t len = strlen(input);
        for (int i = 0; i < count && n < max; i++) {
            if (StartsWith(cmds[i].name, input, len)) out[n++] = cmds[i].name;
        }
        for (const char* c : CONSOLE_COMMANDS) {
            if (n < max && StartsWith(c, input, len)) out[n++] = c;
        }
        return n;
    }
    // First argument of a command with a known argument kind
    const char* arg = space + 1;
    if (strchr(arg, ' ')) return 0;
    size_t nameLen = (size_t)(space - input);
    for (int i = 0; i < count; i++) {
        if (strlen(cmds[i].name) == nameLen && strncmp(cmds[i].name, input, nameLen) == 0) {
            const char* all[64];
            int m = GetScriptArgCompletions(cmds[i].argKind, all, 64);
            size_t len = strlen(arg);
            for (int k = 0; k < m && n < max; k++) {
                if (StartsWith(all[k], arg, len)) out[n++] = all[k];
            }
            break;
        }
    }
    return n;
}

static void SetInput(Console* console, const char* text) {
    snprintf(console->input, sizeof(console->input), "%s", text);
    console->length = (int)strlen(console->input);
}

static void Complete(Console* console) {
    const char* cands[64];
    int n = ConsoleCompletions(console->input, cands, 64);
    if (n == 0) return;
    char* token = strrchr(console->input, ' ');
    token = token ? token + 1 : console->input;
    size_t start = (size_t)(token - console->input);
    // Longest common prefix of the candidates
    size_t common = strlen(cands[0]);
    for (int i = 1; i < n; i++) {
        size_t k = 0;
        while (k < common && cands[i][k] && tolower(cands[i][k]) == tolower(cands[0][k])) k++;
        common = k;
    }
    char buf[CONSOLE_INPUT_MAX];
    snprintf(buf, sizeof(buf), "%.*s%.*s%s", (int)start, console->input, (int)common, cands[0], n == 1 ? " " : "");
    SetInput(console, buf);
    if (n > 1) {
        char line[160] = "";
        for (int i = 0; i < n; i++) {
            size_t used = strlen(line);
            if (used + strlen(cands[i]) + 2 >= sizeof(line)) break;
            snprintf(line + used, sizeof(line) - used, "%s%s", i ? "  " : "", cands[i]);
        }
        ConsolePrint(console, line);
    }
}

static void Run(Console* console, Camera3D* camera, PlayerState* player, LightingSystem* lighting,
                int screenWidth, int screenHeight) {
    char line[CONSOLE_INPUT_MAX];
    snprintf(line, sizeof(line), "%s", console->input);
    char* p = line;
    while (*p == ' ') p++;
    for (char* e = p + strlen(p); e > p && e[-1] == ' ';) *--e = '\0';
    SetInput(console, "");
    if (!*p) return;

    if (console->historyCount == CONSOLE_HISTORY) {
        memmove(console->history[0], console->history[1], sizeof(console->history[0]) * (CONSOLE_HISTORY - 1));
        console->historyCount--;
    }
    snprintf(console->history[console->historyCount++], CONSOLE_INPUT_MAX, "%s", p);
    console->historyPos = console->historyCount;

    char echo[160];
    snprintf(echo, sizeof(echo), "> %s", p);
    ConsolePrint(console, echo);
    if (strcmp(p, "help") == 0) {
        int count;
        const ScriptCommandInfo* cmds = GetScriptCommands(&count);
        for (int i = 0; i < count; i++) {
            if (!cmds[i].scriptOnly) ConsolePrint(console, cmds[i].usage);
        }
        return;
    }
    if (strcmp(p, "clear") == 0) {
        console->logCount = 0;
        return;
    }
    ScriptCommand cmd;
    char err[160];
    if (!ParseScriptCommand(p, &cmd, err, sizeof(err))) {
        ConsolePrint(console, err);
        return;
    }
    if (!ExecuteScriptCommandNow(cmd, camera, player, lighting, screenWidth, screenHeight)) {
        ConsolePrint(console, "Input and timing commands only run in scripts");
    }
}

void UpdateConsole(Console* console, Camera3D* camera, PlayerState* player, LightingSystem* lighting,
                   int screenWidth, int screenHeight) {
    if (!console->active) return;
    for (int c = GetCharPressed(); c != 0; c = GetCharPressed()) {
        if (c >= 32 && c < 127 && console->length < CONSOLE_INPUT_MAX - 1) {
            console->input[console->length++] = (char)c;
            console->input[console->length] = '\0';
        }
    }
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && console->length > 0) {
        console->input[--console->length] = '\0';
    }
    if (IsKeyPressed(KEY_TAB)) Complete(console);
    if (IsKeyPressed(KEY_UP) && console->historyPos > 0) {
        SetInput(console, console->history[--console->historyPos]);
    }
    if (IsKeyPressed(KEY_DOWN) && console->historyPos < console->historyCount) {
        console->historyPos++;
        SetInput(console, console->historyPos < console->historyCount ? console->history[console->historyPos] : "");
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        Run(console, camera, player, lighting, screenWidth, screenHeight);
    }
}

void DrawConsole(const Console* console, int screenWidth, int screenHeight) {
    (void)screenHeight;
    if (!console->active) return;
    const int FONT = 16, LINE = 20, PAD = 10;
    int height = PAD * 2 + (CONSOLE_LOG_LINES + 2) * LINE;
    DrawRectangle(0, 0, screenWidth, height, (Color){12, 10, 8, 215});
    DrawRectangle(0, height, screenWidth, 2, (Color){139, 90, 43, 255});
    for (int i = 0; i < console->logCount; i++) {
        bool echo = console->log[i][0] == '>';
        DrawText(console->log[i], PAD, PAD + i * LINE, FONT, echo ? (Color){200, 190, 160, 255} : (Color){150, 200, 140, 255});
    }
    int inputY = PAD + CONSOLE_LOG_LINES * LINE;
    char shown[CONSOLE_INPUT_MAX + 4];
    bool caret = ((int)(GetTime() * 2.0)) % 2 == 0;
    snprintf(shown, sizeof(shown), "/ %s%s", console->input, caret ? "_" : "");
    DrawText(shown, PAD, inputY, FONT, (Color){255, 230, 150, 255});

    // Live hint: matching completions, or the usage of the command being typed
    const char* cands[64];
    int n = ConsoleCompletions(console->input, cands, 64);
    char hint[200] = "";
    const char* space = strchr(console->input, ' ');
    int count;
    const ScriptCommandInfo* cmds = GetScriptCommands(&count);
    if (space) {
        for (int i = 0; i < count; i++) {
            size_t len = (size_t)(space - console->input);
            if (strlen(cmds[i].name) == len && strncmp(cmds[i].name, console->input, len) == 0) {
                snprintf(hint, sizeof(hint), "%s%s", cmds[i].usage, cmds[i].scriptOnly ? "   (scripts only)" : "");
            }
        }
    }
    if (n > 0 && console->length > 0) {
        size_t used = strlen(hint);
        if (used) used += (size_t)snprintf(hint + used, sizeof(hint) - used, "   ");
        for (int i = 0; i < n && used < sizeof(hint) - 24; i++) {
            used += (size_t)snprintf(hint + used, sizeof(hint) - used, "%s%s", i ? "  " : "", cands[i]);
        }
    }
    DrawText(hint, PAD + 16, inputY + LINE, 14, (Color){150, 140, 120, 255});
}
