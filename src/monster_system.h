#pragma once

#include "types.h"

// Load all custom monsters from file
// Returns the number of monsters loaded
int LoadCustomMonsters(const char* filepath, CustomMonster* monsters, int maxMonsters);

// Save all custom monsters to file
// Returns true on success
bool SaveCustomMonsters(const char* filepath, const CustomMonster* monsters, int monsterCount);

// Add a new custom monster to the array
// Returns the index of the new monster, or -1 if array is full
int AddCustomMonster(CustomMonster* monsters, int* monsterCount, int maxMonsters,
                     const CustomMonster* newMonster);

// Remove a custom monster by index
// Returns true on success
bool RemoveCustomMonster(CustomMonster* monsters, int* monsterCount, int index);

// Generate a unique monster ID from name (e.g., "Forest Fox" -> "forest_fox")
void GenerateMonsterID(char* buffer, int bufferSize, const char* monsterName);

// Find a custom monster by ID
// Returns index or -1 if not found
int FindCustomMonsterByID(const CustomMonster* monsters, int monsterCount, const char* id);
