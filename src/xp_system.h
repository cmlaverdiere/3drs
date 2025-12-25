#ifndef XP_SYSTEM_H
#define XP_SYSTEM_H

// OSRS XP table (XP required for each level 1-99)
extern const int XP_TABLE[100];

// Get level from XP
int GetLevelFromXP(int xp);

// OSRS max hit calculation (simplified - no equipment)
int CalculateMaxHit(int strengthLevel);

// Random hit from 0 to max (inclusive), 0 = miss
int RollDamage(int maxHit);

// Get max hitpoints for a given combat level (10 HP at level 1, +1 per level)
int GetMaxHitpoints(int combatLevel);

#endif
