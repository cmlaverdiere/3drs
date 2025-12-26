#ifndef SOUND_SYSTEM_H
#define SOUND_SYSTEM_H

#include "raylib.h"

// Sound effect types
enum SoundEffect {
    SFX_HIT = 0,        // Attack connects
    SFX_MISS,           // Attack misses / whoosh
    SFX_PICKUP,         // Item picked up
    SFX_ENEMY_DEATH,    // Enemy dies
    SFX_LEVEL_UP,       // Level up fanfare
    SFX_PLAYER_HURT,    // Player takes damage
    SFX_XP_GAIN,        // XP gained
    SFX_BURY,           // Bury bones
    SFX_SWING_HEAVY,    // Heavy weapon swing (2H sword)
    SFX_COUNT
};

// Initialize the sound system (call after InitAudioDevice)
void InitSoundSystem();

// Cleanup sounds
void UnloadSoundSystem();

// Play a sound effect
void PlaySoundEffect(SoundEffect sfx);

// Music functions (procedurally generated, per-season)
void InitBackgroundMusic(int season);  // 0=Summer, 1=Autumn, 2=Winter, 3=Spring
void UpdateBackgroundMusic(int currentSeason);  // Switches song if season changed
void SetMusicVolume(float volume);  // 0.0 to 1.0
void UnloadBackgroundMusic();
bool IsMusicLoaded();

#endif
