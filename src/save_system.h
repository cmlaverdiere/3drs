#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include "types.h"

extern const char* SAVE_FILE;

// Save game state to file (quests needed to save progress by ID)
void SaveGame(const PlayerState& state, const Quest* quests, int questCount);

// Load game state from file (quests needed to restore progress by ID)
bool LoadGame(PlayerState& state, const Quest* quests, int questCount);

#endif
