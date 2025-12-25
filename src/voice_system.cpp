#include "voice_system.h"
#include <piper.h>
#include <raylib.h>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>

// Voice model paths (relative to executable)
static const char* VOICE_MODELS[] = {
    "voices/en_US-ryan-medium.onnx",  // MALE_DEEP - authoritative, guard-like
    "voices/en_US-joe-medium.onnx"    // MALE_NEUTRAL - friendly
};

static const char* ESPEAK_DATA_PATH = "external/piper/libpiper/install/espeak-ng-data";

// Preprocess text for more natural TTS pauses
static std::string PreprocessTextForSpeech(const char* text) {
    std::string result(text);

    // Replace " - " with "... " for a natural pause at clause breaks
    size_t pos = 0;
    while ((pos = result.find(" - ", pos)) != std::string::npos) {
        result.replace(pos, 3, "... ");
        pos += 4;
    }

    // Also handle "--" (em dash)
    pos = 0;
    while ((pos = result.find("--", pos)) != std::string::npos) {
        result.replace(pos, 2, "... ");
        pos += 4;
    }

    return result;
}

// Piper synthesizers for each voice type
static piper_synthesizer* synthesizers[static_cast<int>(VoiceType::VOICE_TYPE_COUNT)] = {nullptr};

// Currently playing voice
static Sound currentVoice = {0};
static bool voiceLoaded = false;
static bool systemInitialized = false;

void InitVoiceSystem() {
    if (systemInitialized) return;

    printf("[Voice] Initializing voice system...\n");

    // Load each voice model
    for (int i = 0; i < static_cast<int>(VoiceType::VOICE_TYPE_COUNT); i++) {
        const char* modelPath = VOICE_MODELS[i];
        printf("[Voice] Loading voice model: %s\n", modelPath);

        synthesizers[i] = piper_create(modelPath, nullptr, ESPEAK_DATA_PATH);
        if (!synthesizers[i]) {
            printf("[Voice] WARNING: Failed to load voice model: %s\n", modelPath);
        } else {
            printf("[Voice] Loaded voice model: %s\n", modelPath);
        }
    }

    systemInitialized = true;
    printf("[Voice] Voice system initialized\n");
}

void UnloadVoiceSystem() {
    if (!systemInitialized) return;

    StopSpeaking();

    // Free all synthesizers
    for (int i = 0; i < static_cast<int>(VoiceType::VOICE_TYPE_COUNT); i++) {
        if (synthesizers[i]) {
            piper_free(synthesizers[i]);
            synthesizers[i] = nullptr;
        }
    }

    systemInitialized = false;
    printf("[Voice] Voice system unloaded\n");
}

void SpeakText(const char* text, VoiceType voice) {
    if (!systemInitialized) return;

    int voiceIndex = static_cast<int>(voice);
    if (voiceIndex < 0 || voiceIndex >= static_cast<int>(VoiceType::VOICE_TYPE_COUNT)) {
        printf("[Voice] Invalid voice type: %d\n", voiceIndex);
        return;
    }

    piper_synthesizer* synth = synthesizers[voiceIndex];
    if (!synth) {
        printf("[Voice] Voice not loaded for type: %d\n", voiceIndex);
        return;
    }

    // Stop any current voice
    StopSpeaking();

    // Preprocess text for natural pauses at dashes/clauses
    std::string processedText = PreprocessTextForSpeech(text);

    printf("[Voice] Synthesizing: \"%s\"\n", processedText.c_str());

    // Start synthesis with tuned options for more natural speech
    piper_synthesize_options options = piper_default_synthesize_options(synth);
    options.length_scale = 1.05f;    // Slightly slower for more natural pacing
    options.noise_scale = 0.667f;    // Recommended for single-speaker models
    options.noise_w_scale = 0.8f;    // More phoneme length variation

    int result = piper_synthesize_start(synth, processedText.c_str(), &options);
    if (result != PIPER_OK) {
        printf("[Voice] Failed to start synthesis\n");
        return;
    }

    // Collect all audio samples
    std::vector<float> samples;
    int sampleRate = 22050;  // Default, will be updated from chunk

    // Silence samples to add between sentence chunks (100ms at 22050Hz)
    const int INTER_SENTENCE_SILENCE = 2205;

    piper_audio_chunk chunk;
    bool firstChunk = true;
    while (piper_synthesize_next(synth, &chunk) == PIPER_OK) {
        sampleRate = chunk.sample_rate;

        // Add brief silence between sentences for more natural flow
        if (!firstChunk && chunk.num_samples > 0) {
            int silenceSamples = static_cast<int>(sampleRate * 0.08f);  // 80ms pause
            samples.insert(samples.end(), silenceSamples, 0.0f);
        }
        firstChunk = false;

        samples.insert(samples.end(), chunk.samples, chunk.samples + chunk.num_samples);
    }

    if (samples.empty()) {
        printf("[Voice] No audio generated\n");
        return;
    }

    printf("[Voice] Generated %zu samples at %d Hz\n", samples.size(), sampleRate);

    // Convert float samples to 16-bit PCM for Raylib
    // Raylib's LoadSoundFromWave works better with 16-bit samples
    std::vector<short> pcmSamples(samples.size());
    for (size_t i = 0; i < samples.size(); i++) {
        float s = samples[i];
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        pcmSamples[i] = static_cast<short>(s * 32767.0f);
    }

    // Create Raylib Wave
    Wave wave = {0};
    wave.frameCount = static_cast<unsigned int>(pcmSamples.size());
    wave.sampleRate = static_cast<unsigned int>(sampleRate);
    wave.sampleSize = 16;  // 16-bit
    wave.channels = 1;     // Mono

    // Allocate and copy data (Raylib will free this)
    wave.data = RL_MALLOC(pcmSamples.size() * sizeof(short));
    memcpy(wave.data, pcmSamples.data(), pcmSamples.size() * sizeof(short));

    // Load sound from wave and play
    currentVoice = LoadSoundFromWave(wave);
    UnloadWave(wave);  // Wave data is copied to Sound, safe to unload

    voiceLoaded = true;
    PlaySound(currentVoice);

    printf("[Voice] Playing audio\n");
}

// Current pitch for the playing voice (used by SpeakAsNPC)
static float currentPitch = 1.0f;

void SpeakAsNPC(const char* text, NPCType npc) {
    VoiceType voice = GetVoiceForNPC(npc);
    float pitch = GetPitchForNPC(npc);

    SpeakText(text, voice);

    // Apply NPC-specific pitch
    if (voiceLoaded && pitch != 1.0f) {
        SetSoundPitch(currentVoice, pitch);
    }
    currentPitch = pitch;
}

void StopSpeaking() {
    if (voiceLoaded) {
        if (IsSoundPlaying(currentVoice)) {
            StopSound(currentVoice);
        }
        UnloadSound(currentVoice);
        voiceLoaded = false;
    }
}

bool IsSpeaking() {
    return voiceLoaded && IsSoundPlaying(currentVoice);
}

VoiceType GetVoiceForNPC(NPCType npc) {
    switch (npc) {
        case NPC_GUARD:
            return VoiceType::MALE_DEEP;
        default:
            return VoiceType::MALE_NEUTRAL;
    }
}

float GetPitchForNPC(NPCType npc) {
    // Slight pitch variations to give each NPC a distinct voice
    // Range: 0.85 (deeper) to 1.15 (higher)
    switch (npc) {
        case NPC_HANS:
            return 1.05f;   // Slightly higher, friendly
        case NPC_SHOPKEEPER:
            return 0.95f;   // Slightly lower, merchant
        case NPC_GUARD:
            return 0.88f;   // Deep, authoritative
        case NPC_COOK:
            return 1.10f;   // Higher, frantic
        case NPC_VARROCK_TRADER:
            return 0.92f;   // Lower, businessman
        case NPC_VARROCK_BARTENDER:
            return 1.00f;   // Normal
        case NPC_ALKHARID_SILK:
            return 1.08f;   // Higher, eager merchant
        case NPC_ALKHARID_SPICE:
            return 0.98f;   // Slightly lower
        default:
            return 1.0f;
    }
}
