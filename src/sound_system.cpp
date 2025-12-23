#include "sound_system.h"
#include <cmath>
#include <cstdlib>

static Sound sounds[SFX_COUNT];

const int SAMPLE_RATE = 44100;
const int SAMPLE_SIZE = 16;  // 16-bit audio

// Generate a Wave struct with given parameters
static Wave CreateWave(int sampleCount) {
    Wave wave = {};
    wave.frameCount = sampleCount;
    wave.sampleRate = SAMPLE_RATE;
    wave.sampleSize = SAMPLE_SIZE;
    wave.channels = 1;
    wave.data = malloc(sampleCount * sizeof(short));
    return wave;
}

// MIDI note to frequency (A4 = 69 = 440Hz)
static float NoteToFreq(int note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

// Generate a simple square wave tone with decay
static Wave GenerateTone(float freq, float duration, float decay) {
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / SAMPLE_RATE;
        float envelope = expf(-decay * t);
        float sample = sinf(2.0f * PI * freq * t);

        // Add some harmonics for richer sound
        sample += 0.5f * sinf(4.0f * PI * freq * t);
        sample += 0.25f * sinf(6.0f * PI * freq * t);
        sample *= envelope;

        data[i] = (short)(sample * 16000);
    }

    return wave;
}

// Generate a square wave (more chiptune-like)
static Wave GenerateSquareWave(float freq, float duration, float decay) {
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / SAMPLE_RATE;
        float envelope = expf(-decay * t);
        float phase = fmodf(freq * t, 1.0f);
        float sample = (phase < 0.5f) ? 1.0f : -1.0f;
        sample *= envelope;
        data[i] = (short)(sample * 12000);
    }

    return wave;
}

// Generate noise burst (for hits)
static Wave GenerateNoise(float duration, float decay) {
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / SAMPLE_RATE;
        float envelope = expf(-decay * t);
        float sample = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        sample *= envelope;
        data[i] = (short)(sample * 10000);
    }

    return wave;
}

// Generate a pitch sweep (for whoosh/miss)
static Wave GenerateSweep(float startFreq, float endFreq, float duration) {
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;
        float freq = startFreq + (endFreq - startFreq) * progress;
        float envelope = 1.0f - progress;  // Linear decay
        float sample = sinf(2.0f * PI * freq * t) * envelope;
        data[i] = (short)(sample * 10000);
    }

    return wave;
}

// Generate arpeggio (for level up)
static Wave GenerateArpeggio(int* notes, int noteCount, float noteDuration) {
    float totalDuration = noteDuration * noteCount;
    int sampleCount = (int)(SAMPLE_RATE * totalDuration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;

    int samplesPerNote = (int)(SAMPLE_RATE * noteDuration);

    for (int i = 0; i < sampleCount; i++) {
        int noteIndex = i / samplesPerNote;
        if (noteIndex >= noteCount) noteIndex = noteCount - 1;

        float freq = NoteToFreq(notes[noteIndex]);
        float t = (float)i / SAMPLE_RATE;
        float noteT = (float)(i % samplesPerNote) / samplesPerNote;

        // Quick attack, sustain, quick release
        float envelope = 1.0f;
        if (noteT < 0.05f) {
            envelope = noteT / 0.05f;  // Attack
        } else if (noteT > 0.8f) {
            envelope = (1.0f - noteT) / 0.2f;  // Release
        }

        float sample = sinf(2.0f * PI * freq * t);
        sample += 0.3f * sinf(4.0f * PI * freq * t);  // Octave
        sample *= envelope * 0.7f;

        data[i] = (short)(sample * 14000);
    }

    return wave;
}

// Generate hit sound (attack + noise)
static Wave GenerateHitSound() {
    float duration = 0.15f;
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;

    float freq = NoteToFreq(48);  // C3

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / SAMPLE_RATE;
        float envelope = expf(-20.0f * t);

        // Mix of tone and noise for impact feel
        float tone = sinf(2.0f * PI * freq * t);
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        float sample = (tone * 0.6f + noise * 0.4f) * envelope;
        data[i] = (short)(sample * 14000);
    }

    return wave;
}

// Generate player hurt sound (low thud)
static Wave GenerateHurtSound() {
    float duration = 0.25f;
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / SAMPLE_RATE;
        float envelope = expf(-8.0f * t);

        // Low frequency with pitch drop
        float freq = 80.0f - t * 100.0f;
        if (freq < 30.0f) freq = 30.0f;

        float sample = sinf(2.0f * PI * freq * t);
        sample += 0.5f * sinf(4.0f * PI * freq * t);
        sample *= envelope;

        data[i] = (short)(sample * 16000);
    }

    return wave;
}

void InitSoundSystem() {
    Wave wave;

    // SFX_HIT - Short percussive hit
    wave = GenerateHitSound();
    sounds[SFX_HIT] = LoadSoundFromWave(wave);
    UnloadWave(wave);

    // SFX_MISS - Whoosh sound
    wave = GenerateSweep(800.0f, 200.0f, 0.2f);
    sounds[SFX_MISS] = LoadSoundFromWave(wave);
    UnloadWave(wave);

    // SFX_PICKUP - Positive ding (two quick notes)
    int pickupNotes[] = { 72, 76 };  // C5, E5
    wave = GenerateArpeggio(pickupNotes, 2, 0.08f);
    sounds[SFX_PICKUP] = LoadSoundFromWave(wave);
    UnloadWave(wave);

    // SFX_ENEMY_DEATH - Descending tone
    wave = GenerateSweep(600.0f, 100.0f, 0.4f);
    sounds[SFX_ENEMY_DEATH] = LoadSoundFromWave(wave);
    UnloadWave(wave);

    // SFX_LEVEL_UP - Ascending arpeggio fanfare
    int levelUpNotes[] = { 60, 64, 67, 72, 76, 79, 84 };  // C major arpeggio up 2 octaves
    wave = GenerateArpeggio(levelUpNotes, 7, 0.1f);
    sounds[SFX_LEVEL_UP] = LoadSoundFromWave(wave);
    UnloadWave(wave);

    // SFX_PLAYER_HURT - Low thud
    wave = GenerateHurtSound();
    sounds[SFX_PLAYER_HURT] = LoadSoundFromWave(wave);
    UnloadWave(wave);

    // SFX_XP_GAIN - Quick blip
    wave = GenerateSquareWave(NoteToFreq(84), 0.05f, 30.0f);  // C6 blip
    sounds[SFX_XP_GAIN] = LoadSoundFromWave(wave);
    UnloadWave(wave);
}

void UnloadSoundSystem() {
    for (int i = 0; i < SFX_COUNT; i++) {
        UnloadSound(sounds[i]);
    }
}

void PlaySoundEffect(SoundEffect sfx) {
    if (sfx >= 0 && sfx < SFX_COUNT) {
        PlaySound(sounds[sfx]);
    }
}
