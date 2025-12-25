#include "help_system.h"
#include "raylib.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <curl/curl.h>

// Parchment colors (matching hud.cpp)
static const Color PARCHMENT_BG = { 222, 198, 158, 240 };
static const Color PARCHMENT_BORDER = { 139, 90, 43, 255 };
static const Color PARCHMENT_DARK = { 180, 150, 100, 255 };
static const Color PARCHMENT_TEXT = { 60, 40, 20, 255 };

// Response buffer for curl
struct ResponseBuffer {
    char* data;
    size_t size;
    size_t capacity;
};

static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t totalSize = size * nmemb;
    ResponseBuffer* buf = (ResponseBuffer*)userdata;

    if (buf->size + totalSize >= buf->capacity) {
        return 0; // Buffer full
    }

    memcpy(buf->data + buf->size, ptr, totalSize);
    buf->size += totalSize;
    buf->data[buf->size] = '\0';
    return totalSize;
}

// JSON escaping for the prompt
static void EscapeJsonString(const char* input, char* output, int maxLen) {
    int j = 0;
    for (int i = 0; input[i] && j < maxLen - 2; i++) {
        char c = input[i];
        if (c == '"' || c == '\\') {
            output[j++] = '\\';
            output[j++] = c;
        } else if (c == '\n') {
            output[j++] = '\\';
            output[j++] = 'n';
        } else if (c == '\r') {
            // Skip carriage returns
        } else if (c == '\t') {
            output[j++] = '\\';
            output[j++] = 't';
        } else {
            output[j++] = c;
        }
    }
    output[j] = '\0';
}

// Extract text content from Claude API JSON response
static bool ExtractResponseText(const char* jsonResponse, char* textOut, int maxLen) {
    // Find "text": " or "text":" in the response
    const char* textKey = "\"text\":";
    const char* start = strstr(jsonResponse, textKey);
    if (!start) return false;

    start += strlen(textKey);
    // Skip whitespace
    while (*start == ' ' || *start == '\t') start++;
    if (*start != '"') return false;
    start++; // Skip opening quote

    // Find closing quote (handling escaped quotes)
    int j = 0;
    while (*start && j < maxLen - 1) {
        if (*start == '"' && *(start - 1) != '\\') {
            break;
        }
        // Handle escape sequences
        if (*start == '\\' && *(start + 1)) {
            start++;
            switch (*start) {
                case 'n': textOut[j++] = '\n'; break;
                case 't': textOut[j++] = '\t'; break;
                case 'r': break; // Skip carriage return
                case '"': textOut[j++] = '"'; break;
                case '\\': textOut[j++] = '\\'; break;
                default: textOut[j++] = *start; break;
            }
        } else {
            textOut[j++] = *start;
        }
        start++;
    }
    textOut[j] = '\0';
    return j > 0;
}

// Build the objective description
static void DescribeObjective(const QuestObjective* obj, char* desc, int maxLen) {
    if (obj->type == OBJ_ITEM) {
        snprintf(desc, maxLen, "Bring %s to %s",
                 ITEM_NAMES[obj->item], NPC_CONFIGS[obj->targetNPC].name);
    } else if (obj->type == OBJ_TALK_TO) {
        snprintf(desc, maxLen, "Talk to %s",
                 NPC_CONFIGS[obj->targetNPC].name);
    } else {
        snprintf(desc, maxLen, "Unknown objective");
    }
}

// Build the prompt with quest context (prevents spoilers)
static void BuildHelpPrompt(char* prompt, int maxLen,
                           const Quest* quest, const QuestProgress* progress,
                           const char* playerQuestion) {
    char questContext[2048];
    int pos = 0;

    // Add quest name
    pos += snprintf(questContext + pos, sizeof(questContext) - pos,
        "Quest: %s\\n\\n", quest->name);

    // Add quest intro as description (first 2-3 lines)
    if (quest->dialogueStart && quest->dialogueStartCount > 0) {
        pos += snprintf(questContext + pos, sizeof(questContext) - pos,
            "Quest Description:\\n");
        for (int i = 0; i < quest->dialogueStartCount && i < 3; i++) {
            // Escape dialogue lines for JSON
            char escapedLine[512];
            EscapeJsonString(quest->dialogueStart[i], escapedLine, sizeof(escapedLine));
            pos += snprintf(questContext + pos, sizeof(questContext) - pos,
                "%s\\n", escapedLine);
        }
        pos += snprintf(questContext + pos, sizeof(questContext) - pos, "\\n");
    }

    // Add ONLY completed objectives
    if (progress->state != QUEST_NOT_STARTED && progress->currentObjective > 0) {
        pos += snprintf(questContext + pos, sizeof(questContext) - pos,
            "Completed objectives:\\n");
        for (int i = 0; i < progress->currentObjective; i++) {
            char desc[128];
            DescribeObjective(&quest->objectives[i], desc, sizeof(desc));
            pos += snprintf(questContext + pos, sizeof(questContext) - pos,
                "- %s\\n", desc);
        }
        pos += snprintf(questContext + pos, sizeof(questContext) - pos, "\\n");
    }

    // Add CURRENT objective only (never future ones)
    if (progress->state == QUEST_IN_PROGRESS &&
        progress->currentObjective < quest->objectiveCount) {
        char desc[128];
        DescribeObjective(&quest->objectives[progress->currentObjective], desc, sizeof(desc));
        pos += snprintf(questContext + pos, sizeof(questContext) - pos,
            "Current objective:\\n- %s\\n", desc);
    }

    // Escape the player question
    char escapedQuestion[512];
    EscapeJsonString(playerQuestion, escapedQuestion, sizeof(escapedQuestion));

    // Build final prompt
    snprintf(prompt, maxLen,
        "You are a helpful quest guide for an OSRS-style fantasy game. "
        "A player is asking for help with their current quest.\\n\\n"
        "IMPORTANT RULES:\\n"
        "1. ONLY help with the current objective or previously completed objectives\\n"
        "2. DO NOT reveal or hint at any future objectives\\n"
        "3. DO NOT spoil quest rewards\\n"
        "4. Keep responses concise (2-3 sentences)\\n"
        "5. Stay in-character as a friendly game guide\\n\\n"
        "QUEST CONTEXT:\\n%s\\n"
        "PLAYER QUESTION: %s\\n\\n"
        "Provide a helpful hint without spoilers:",
        questContext, escapedQuestion);
}

// Worker thread function for API call
static void HelpWorkerThread(HelpSystem* help, const Quest* quest,
                            const QuestProgress* progress, const char* question) {
    // Get API key
    const char* apiKey = getenv("MY_ANTHROPIC_API_KEY");
    if (!apiKey || strlen(apiKey) == 0) {
        std::lock_guard<std::mutex> lock(*help->responseMutex);
        strcpy(help->errorMessage, "Set MY_ANTHROPIC_API_KEY environment variable");
        help->hasError = true;
        help->responseReady = true;
        *help->requestInFlight = false;
        return;
    }

    // Build the prompt
    char prompt[4096];
    BuildHelpPrompt(prompt, sizeof(prompt), quest, progress, question);

    // Build JSON body
    char jsonBody[8192];
    snprintf(jsonBody, sizeof(jsonBody),
        "{"
        "\"model\":\"claude-sonnet-4-20250514\","
        "\"max_tokens\":500,"
        "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}]"
        "}",
        prompt);

    // Initialize curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::lock_guard<std::mutex> lock(*help->responseMutex);
        strcpy(help->errorMessage, "Failed to initialize HTTP client");
        help->hasError = true;
        help->responseReady = true;
        *help->requestInFlight = false;
        return;
    }

    // Response buffer
    ResponseBuffer responseBuffer;
    responseBuffer.capacity = 16384;
    responseBuffer.data = (char*)malloc(responseBuffer.capacity);
    responseBuffer.size = 0;
    responseBuffer.data[0] = '\0';

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");

    // Set headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    char authHeader[256];
    snprintf(authHeader, sizeof(authHeader), "x-api-key: %s", apiKey);
    headers = curl_slist_append(headers, authHeader);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set POST data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody);

    // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    // Set timeout (30 seconds)
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    // Perform request
    CURLcode res = curl_easy_perform(curl);

    // Process result
    {
        std::lock_guard<std::mutex> lock(*help->responseMutex);

        if (res == CURLE_OK) {
            // Check HTTP status
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

            if (httpCode == 200) {
                // Parse response
                if (ExtractResponseText(responseBuffer.data, help->response, HELP_RESPONSE_MAX)) {
                    help->responseLength = strlen(help->response);
                    help->hasError = false;
                } else {
                    strcpy(help->errorMessage, "Failed to parse API response");
                    help->hasError = true;
                }
            } else {
                snprintf(help->errorMessage, sizeof(help->errorMessage),
                         "API error (HTTP %ld)", httpCode);
                help->hasError = true;
            }
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
            strcpy(help->errorMessage, "Request timed out. Try again.");
            help->hasError = true;
        } else {
            snprintf(help->errorMessage, sizeof(help->errorMessage),
                     "Network error: %s", curl_easy_strerror(res));
            help->hasError = true;
        }

        help->responseReady = true;
    }

    // Cleanup
    free(responseBuffer.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    *help->requestInFlight = false;
}

// Find first in-progress quest
static int FindActiveQuest(const Quest* quests, int questCount,
                          const PlayerState* playerState) {
    for (int i = 0; i < questCount; i++) {
        if (!quests[i].loaded) continue;
        if (playerState->questProgress[i].state == QUEST_IN_PROGRESS) {
            return i;
        }
    }
    return -1;
}

void InitHelpSystem(HelpSystem* help) {
    memset(help, 0, sizeof(HelpSystem));
    help->state = HelpState::CLOSED;
    help->activeQuestIndex = -1;
    help->workerThread = nullptr;

    // Allocate threading primitives (can't be zero-initialized)
    help->requestInFlight = new std::atomic<bool>(false);
    help->responseMutex = new std::mutex();

    // Initialize curl globally (once per app)
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void ShutdownHelpSystem(HelpSystem* help) {
    // Wait for any pending request
    if (help->workerThread && help->workerThread->joinable()) {
        help->workerThread->join();
        delete help->workerThread;
        help->workerThread = nullptr;
    }

    // Free threading primitives
    delete help->requestInFlight;
    delete help->responseMutex;
    help->requestInFlight = nullptr;
    help->responseMutex = nullptr;

    curl_global_cleanup();
}

void OpenHelpUI(HelpSystem* help, const Quest* quests, int questCount,
               const PlayerState* playerState) {
    // Find active quest
    int questIndex = FindActiveQuest(quests, questCount, playerState);

    if (questIndex < 0) {
        // No active quest - show error immediately
        help->state = HelpState::DISPLAYING;
        strcpy(help->errorMessage, "You don't have an active quest to ask about.");
        help->hasError = true;
        help->activeQuestIndex = -1;
        return;
    }

    help->state = HelpState::TYPING;
    help->activeQuestIndex = questIndex;
    help->inputBuffer[0] = '\0';
    help->inputLength = 0;
    help->cursorPos = 0;
    help->response[0] = '\0';
    help->responseLength = 0;
    help->responseReady = false;
    help->hasError = false;
    help->scrollOffset = 0;
    help->maxScrollOffset = 0;
}

void CloseHelpUI(HelpSystem* help) {
    help->state = HelpState::CLOSED;
    // Note: Don't clean up thread here - let it finish naturally
}

static void SubmitHelpQuestion(HelpSystem* help, const Quest* quests,
                               const PlayerState* playerState) {
    if (help->inputLength == 0) return;
    if (help->activeQuestIndex < 0) return;
    if (*help->requestInFlight) return;

    help->state = HelpState::WAITING;
    help->responseReady = false;
    *help->requestInFlight = true;

    // Clean up previous thread if any
    if (help->workerThread && help->workerThread->joinable()) {
        help->workerThread->join();
        delete help->workerThread;
    }

    // Copy question for thread
    char question[HELP_INPUT_MAX];
    strncpy(question, help->inputBuffer, HELP_INPUT_MAX);
    question[HELP_INPUT_MAX - 1] = '\0';

    const Quest* quest = &quests[help->activeQuestIndex];
    const QuestProgress* progress = &playerState->questProgress[help->activeQuestIndex];

    // Spawn worker thread
    help->workerThread = new std::thread(HelpWorkerThread, help, quest, progress, question);
}

void UpdateHelpSystem(HelpSystem* help, const Quest* quests, int questCount,
                      const PlayerState* playerState) {
    // Check for worker thread completion
    if (help->state == HelpState::WAITING && !*help->requestInFlight) {
        std::lock_guard<std::mutex> lock(*help->responseMutex);
        if (help->responseReady) {
            help->state = HelpState::DISPLAYING;
            // Thread cleanup will happen on next submit or shutdown
        }
    }

    // Handle input based on state
    if (help->state == HelpState::TYPING) {
        // Text input
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126 && help->inputLength < HELP_INPUT_MAX - 1) {
                // Insert character at cursor
                for (int i = help->inputLength; i > help->cursorPos; i--) {
                    help->inputBuffer[i] = help->inputBuffer[i - 1];
                }
                help->inputBuffer[help->cursorPos] = (char)key;
                help->cursorPos++;
                help->inputLength++;
                help->inputBuffer[help->inputLength] = '\0';
            }
            key = GetCharPressed();
        }

        // Backspace
        if (IsKeyPressed(KEY_BACKSPACE) && help->cursorPos > 0) {
            for (int i = help->cursorPos - 1; i < help->inputLength - 1; i++) {
                help->inputBuffer[i] = help->inputBuffer[i + 1];
            }
            help->cursorPos--;
            help->inputLength--;
            help->inputBuffer[help->inputLength] = '\0';
        }

        // Delete
        if (IsKeyPressed(KEY_DELETE) && help->cursorPos < help->inputLength) {
            for (int i = help->cursorPos; i < help->inputLength - 1; i++) {
                help->inputBuffer[i] = help->inputBuffer[i + 1];
            }
            help->inputLength--;
            help->inputBuffer[help->inputLength] = '\0';
        }

        // Cursor movement
        if (IsKeyPressed(KEY_LEFT) && help->cursorPos > 0) {
            help->cursorPos--;
        }
        if (IsKeyPressed(KEY_RIGHT) && help->cursorPos < help->inputLength) {
            help->cursorPos++;
        }

        // Submit
        if (IsKeyPressed(KEY_ENTER) && help->inputLength > 0) {
            SubmitHelpQuestion(help, quests, playerState);
        }

        // Cancel
        if (IsKeyPressed(KEY_ESCAPE)) {
            CloseHelpUI(help);
        }
    }
    else if (help->state == HelpState::WAITING) {
        // Allow cancel during waiting
        if (IsKeyPressed(KEY_ESCAPE)) {
            // Note: Request will still complete, but we close the UI
            CloseHelpUI(help);
        }
    }
    else if (help->state == HelpState::DISPLAYING) {
        // Scroll
        if (IsKeyDown(KEY_UP) && help->scrollOffset > 0) {
            help->scrollOffset -= 2;
            if (help->scrollOffset < 0) help->scrollOffset = 0;
        }
        if (IsKeyDown(KEY_DOWN) && help->scrollOffset < help->maxScrollOffset) {
            help->scrollOffset += 2;
        }

        // Close
        if (IsKeyPressed(KEY_ESCAPE)) {
            CloseHelpUI(help);
        }
    }
}

// Word wrap helper
static int DrawWordWrappedText(const char* text, int x, int y, int maxWidth,
                               int maxHeight, int fontSize, Color color,
                               int scrollOffset, int* outTotalHeight) {
    if (!text || !text[0]) return 0;

    int lineHeight = fontSize + 4;
    int currentY = y - scrollOffset;
    int startY = y;
    int endY = y + maxHeight;

    char lineBuffer[256];
    int lineLen = 0;
    int wordStart = 0;

    for (int i = 0; ; i++) {
        char c = text[i];
        bool isEnd = (c == '\0');
        bool isSpace = (c == ' ' || c == '\n');
        bool isNewline = (c == '\n');

        if (isSpace || isEnd) {
            // Check if adding this word would overflow
            char testLine[256];
            int testLen = lineLen;
            if (lineLen > 0) {
                memcpy(testLine, lineBuffer, lineLen);
                testLine[testLen++] = ' ';
            }
            for (int j = wordStart; j < i; j++) {
                testLine[testLen++] = text[j];
            }
            testLine[testLen] = '\0';

            int testWidth = MeasureText(testLine, fontSize);

            if (testWidth > maxWidth && lineLen > 0) {
                // Flush current line
                lineBuffer[lineLen] = '\0';
                if (currentY >= startY && currentY < endY) {
                    DrawText(lineBuffer, x, currentY, fontSize, color);
                }
                currentY += lineHeight;
                lineLen = 0;

                // Start new line with current word
                for (int j = wordStart; j < i; j++) {
                    lineBuffer[lineLen++] = text[j];
                }
            } else {
                // Add word to current line
                if (lineLen > 0) lineBuffer[lineLen++] = ' ';
                for (int j = wordStart; j < i; j++) {
                    lineBuffer[lineLen++] = text[j];
                }
            }

            wordStart = i + 1;

            // Handle explicit newlines
            if (isNewline) {
                lineBuffer[lineLen] = '\0';
                if (currentY >= startY && currentY < endY) {
                    DrawText(lineBuffer, x, currentY, fontSize, color);
                }
                currentY += lineHeight;
                lineLen = 0;
            }
        }

        if (isEnd) break;
    }

    // Flush remaining line
    if (lineLen > 0) {
        lineBuffer[lineLen] = '\0';
        if (currentY >= startY && currentY < endY) {
            DrawText(lineBuffer, x, currentY, fontSize, color);
        }
        currentY += lineHeight;
    }

    if (outTotalHeight) {
        *outTotalHeight = currentY - (y - scrollOffset);
    }

    return currentY - y + scrollOffset;
}

void DrawHelpUI(const HelpSystem* help, int screenWidth, int screenHeight) {
    if (help->state == HelpState::CLOSED) return;

    // Box dimensions - centered, larger than dialogue box
    const int BOX_WIDTH = 600;
    const int BOX_HEIGHT = 400;
    const int BOX_X = (screenWidth - BOX_WIDTH) / 2;
    const int BOX_Y = (screenHeight - BOX_HEIGHT) / 2;
    const int PADDING = 20;
    const int BORDER_WIDTH = 4;

    // Draw outer border
    DrawRectangle(BOX_X - BORDER_WIDTH, BOX_Y - BORDER_WIDTH,
                  BOX_WIDTH + BORDER_WIDTH * 2, BOX_HEIGHT + BORDER_WIDTH * 2,
                  PARCHMENT_BORDER);

    // Draw main background
    DrawRectangle(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, PARCHMENT_BG);

    // Draw inner border accent
    DrawRectangleLines(BOX_X + 6, BOX_Y + 6, BOX_WIDTH - 12, BOX_HEIGHT - 12, PARCHMENT_DARK);

    // Decorative corner triangles
    const int CORNER_SIZE = 12;
    DrawTriangle(
        (Vector2){(float)BOX_X, (float)BOX_Y},
        (Vector2){(float)(BOX_X + CORNER_SIZE), (float)BOX_Y},
        (Vector2){(float)BOX_X, (float)(BOX_Y + CORNER_SIZE)},
        PARCHMENT_BORDER);
    DrawTriangle(
        (Vector2){(float)(BOX_X + BOX_WIDTH), (float)BOX_Y},
        (Vector2){(float)(BOX_X + BOX_WIDTH - CORNER_SIZE), (float)BOX_Y},
        (Vector2){(float)(BOX_X + BOX_WIDTH), (float)(BOX_Y + CORNER_SIZE)},
        PARCHMENT_BORDER);
    DrawTriangle(
        (Vector2){(float)BOX_X, (float)(BOX_Y + BOX_HEIGHT)},
        (Vector2){(float)(BOX_X + CORNER_SIZE), (float)(BOX_Y + BOX_HEIGHT)},
        (Vector2){(float)BOX_X, (float)(BOX_Y + BOX_HEIGHT - CORNER_SIZE)},
        PARCHMENT_BORDER);
    DrawTriangle(
        (Vector2){(float)(BOX_X + BOX_WIDTH), (float)(BOX_Y + BOX_HEIGHT)},
        (Vector2){(float)(BOX_X + BOX_WIDTH - CORNER_SIZE), (float)(BOX_Y + BOX_HEIGHT)},
        (Vector2){(float)(BOX_X + BOX_WIDTH), (float)(BOX_Y + BOX_HEIGHT - CORNER_SIZE)},
        PARCHMENT_BORDER);

    // Title
    const char* title = "Quest Help";
    DrawText(title, BOX_X + PADDING, BOX_Y + PADDING, 28, PARCHMENT_BORDER);

    // Separator line
    int separatorY = BOX_Y + PADDING + 32;
    DrawRectangle(BOX_X + PADDING, separatorY, BOX_WIDTH - PADDING * 2, 2, PARCHMENT_BORDER);

    int contentY = separatorY + 15;

    if (help->state == HelpState::TYPING) {
        // Prompt
        DrawText("Ask about your current quest:", BOX_X + PADDING, contentY, 18, PARCHMENT_TEXT);
        contentY += 30;

        // Input box
        int inputBoxY = contentY;
        int inputBoxH = 30;
        int inputBoxW = BOX_WIDTH - PADDING * 2;
        DrawRectangle(BOX_X + PADDING, inputBoxY, inputBoxW, inputBoxH, WHITE);
        DrawRectangleLines(BOX_X + PADDING, inputBoxY, inputBoxW, inputBoxH, PARCHMENT_BORDER);

        // Input text
        DrawText(help->inputBuffer, BOX_X + PADDING + 5, inputBoxY + 7, 16, BLACK);

        // Cursor (blinking)
        if (((int)(GetTime() * 2) % 2) == 0) {
            char temp[HELP_INPUT_MAX];
            strncpy(temp, help->inputBuffer, help->cursorPos);
            temp[help->cursorPos] = '\0';
            int cursorX = BOX_X + PADDING + 5 + MeasureText(temp, 16);
            DrawRectangle(cursorX, inputBoxY + 5, 2, 20, BLACK);
        }

        // Instructions
        DrawText("ENTER to submit, ESC to cancel",
                 BOX_X + PADDING, BOX_Y + BOX_HEIGHT - 30, 14, PARCHMENT_TEXT);
    }
    else if (help->state == HelpState::WAITING) {
        // Animated waiting message
        const char* waitText = "Waiting for response";
        int dots = ((int)(GetTime() * 3)) % 4;
        char waitDisplay[64];
        snprintf(waitDisplay, sizeof(waitDisplay), "%s%.*s", waitText, dots, "...");

        int textWidth = MeasureText(waitDisplay, 24);
        DrawText(waitDisplay, BOX_X + (BOX_WIDTH - textWidth) / 2,
                 BOX_Y + BOX_HEIGHT / 2 - 12, 24, PARCHMENT_TEXT);

        DrawText("ESC to cancel", BOX_X + PADDING, BOX_Y + BOX_HEIGHT - 30, 14, PARCHMENT_TEXT);
    }
    else if (help->state == HelpState::DISPLAYING) {
        if (help->hasError) {
            DrawText(help->errorMessage, BOX_X + PADDING, contentY, 18, RED);
        } else {
            // Draw word-wrapped response
            int totalHeight = 0;
            int maxContentHeight = BOX_HEIGHT - 100;
            DrawWordWrappedText(help->response, BOX_X + PADDING, contentY,
                               BOX_WIDTH - PADDING * 2, maxContentHeight,
                               16, PARCHMENT_TEXT, help->scrollOffset, &totalHeight);

            // Update max scroll (cast away const for this update)
            HelpSystem* mutableHelp = const_cast<HelpSystem*>(help);
            mutableHelp->maxScrollOffset = totalHeight > maxContentHeight ?
                                           totalHeight - maxContentHeight : 0;
        }

        // Instructions
        if (help->maxScrollOffset > 0) {
            DrawText("UP/DOWN to scroll, ESC to close",
                     BOX_X + PADDING, BOX_Y + BOX_HEIGHT - 30, 14, PARCHMENT_TEXT);
        } else {
            DrawText("ESC to close",
                     BOX_X + PADDING, BOX_Y + BOX_HEIGHT - 30, 14, PARCHMENT_TEXT);
        }
    }
}
