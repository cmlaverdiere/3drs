#ifndef VOICE_SYSTEM_H
#define VOICE_SYSTEM_H

#include "types.h"

// Voice types for different NPC characters
enum class VoiceType {
    MALE_DEEP,      // Guard-like, authoritative NPCs (Ryan voice)
    MALE_NEUTRAL,   // Friendly NPCs like Hans, traders (Joe voice)
    VOICE_TYPE_COUNT
};

// Initialize the voice system (call after InitAudioDevice)
void InitVoiceSystem();

// Cleanup the voice system (call before CloseAudioDevice)
void UnloadVoiceSystem();

// Start speaking text with a given voice type
// This will stop any currently playing voice
void SpeakText(const char* text, VoiceType voice);

// Stop any currently playing voice
void StopSpeaking();

// Check if voice is currently playing
bool IsSpeaking();

// Get the appropriate voice type for an NPC
VoiceType GetVoiceForNPC(NPCType npc);

#endif // VOICE_SYSTEM_H
