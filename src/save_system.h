#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include "types.h"

extern const char* SAVE_FILE;

// Save game state to file
void SaveGame(const PlayerState& state);

// Load game state from file
bool LoadGame(PlayerState& state);

#endif
