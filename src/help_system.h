#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include "types.h"

// Maximum sizes
constexpr int HELP_INPUT_MAX = 256;
constexpr int HELP_RESPONSE_MAX = 4096;

// Help UI state machine
enum class HelpState {
    CLOSED,           // Help UI not visible
    TYPING,           // Player is typing question
    WAITING,          // API request in flight
    DISPLAYING        // Showing Claude's response
};

// Thread-safe help system state
struct HelpSystem {
    // UI state
    HelpState state;
    char inputBuffer[HELP_INPUT_MAX];
    int inputLength;
    int cursorPos;

    // Response storage
    char response[HELP_RESPONSE_MAX];
    int responseLength;
    bool responseReady;      // Set by worker thread when done

    // Error handling
    bool hasError;
    char errorMessage[256];

    // Quest context (set when opening help)
    int activeQuestIndex;

    // Threading (pointers to avoid zero-init issues with C++ types)
    std::thread* workerThread;
    std::atomic<bool>* requestInFlight;
    std::mutex* responseMutex;

    // Scroll state for long responses
    int scrollOffset;
    int maxScrollOffset;
};

// Initialize the help system
void InitHelpSystem(HelpSystem* help);

// Shutdown and cleanup (waits for any pending thread)
void ShutdownHelpSystem(HelpSystem* help);

// Open the help UI (call when 'H' pressed)
void OpenHelpUI(HelpSystem* help, const Quest* quests, int questCount,
                const PlayerState* playerState);

// Close the help UI
void CloseHelpUI(HelpSystem* help);

// Update help system (call every frame)
void UpdateHelpSystem(HelpSystem* help, const Quest* quests, int questCount,
                      const PlayerState* playerState);

// Draw help UI (call after other HUD)
void DrawHelpUI(const HelpSystem* help, int screenWidth, int screenHeight);
