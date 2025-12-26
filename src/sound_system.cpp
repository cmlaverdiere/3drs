#include "sound_system.h"
#include <cmath>
#include <cstdlib>

static Sound sounds[SFX_COUNT];
static Music backgroundMusic = {};
static bool musicLoaded = false;

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

    // SFX_BURY - Soft earthy thud for burying bones
    wave = GenerateNoise(0.3f, 12.0f);  // Short, muffled noise
    sounds[SFX_BURY] = LoadSoundFromWave(wave);
    UnloadWave(wave);

    // SFX_SWING_HEAVY - Deep whoosh for heavy weapons
    {
        float duration = 0.35f;
        int sampleCount = (int)(SAMPLE_RATE * duration);
        wave = CreateWave(sampleCount);
        short* data = (short*)wave.data;

        for (int i = 0; i < sampleCount; i++) {
            float t = (float)i / SAMPLE_RATE;
            float progress = t / duration;

            // Lower frequency sweep than normal whoosh
            float freq = 400.0f - 300.0f * progress;

            // Envelope: quick attack, sustained, then fade
            float envelope = 1.0f;
            if (progress < 0.1f) {
                envelope = progress / 0.1f;
            } else if (progress > 0.6f) {
                envelope = (1.0f - progress) / 0.4f;
            }

            // Mix sweep with filtered noise for weight
            float sweep = sinf(2.0f * PI * freq * t);
            float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

            float sample = (sweep * 0.7f + noise * 0.3f) * envelope;
            data[i] = (short)(sample * 12000);
        }

        sounds[SFX_SWING_HEAVY] = LoadSoundFromWave(wave);
        UnloadWave(wave);
    }
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

// Procedural ambient music generation
static Sound ambientMusic = {};
static float musicVolume = 1.0f;

// Pentatonic scale notes (C minor pentatonic) for ambient feel
static const int AMBIENT_SCALE[] = { 48, 51, 53, 55, 58, 60, 63, 65, 67, 70 };  // C3 to Bb4
static const int AMBIENT_SCALE_SIZE = 10;

// Generate a soft pad tone (sine with slow attack/release)
static void AddPadTone(short* data, int sampleCount, float freq, float startTime, float duration, float amplitude) {
    int startSample = (int)(startTime * SAMPLE_RATE);
    int durationSamples = (int)(duration * SAMPLE_RATE);
    int endSample = startSample + durationSamples;
    if (endSample > sampleCount) endSample = sampleCount;

    float attackTime = 0.3f;
    float releaseTime = 0.5f;

    for (int i = startSample; i < endSample; i++) {
        float t = (float)(i - startSample) / SAMPLE_RATE;
        float progress = t / duration;

        // Soft envelope
        float envelope = 1.0f;
        if (t < attackTime) {
            envelope = t / attackTime;
        } else if (progress > (1.0f - releaseTime / duration)) {
            envelope = (duration - t) / releaseTime;
        }
        envelope = envelope * envelope;  // Smoother curve

        // Layered sines for pad sound
        float sample = sinf(2.0f * PI * freq * t) * 0.6f;
        sample += sinf(2.0f * PI * freq * 2.0f * t) * 0.2f;  // Octave
        sample += sinf(2.0f * PI * freq * 0.5f * t) * 0.2f;  // Sub-octave
        sample *= envelope * amplitude;

        // Mix into buffer
        int current = data[i];
        data[i] = (short)(current + sample * 8000);
    }
}

// Generate an arpeggio note
static void AddArpNote(short* data, int sampleCount, float freq, float startTime, float duration, float amplitude) {
    int startSample = (int)(startTime * SAMPLE_RATE);
    int durationSamples = (int)(duration * SAMPLE_RATE);
    int endSample = startSample + durationSamples;
    if (endSample > sampleCount) endSample = sampleCount;

    for (int i = startSample; i < endSample; i++) {
        float t = (float)(i - startSample) / SAMPLE_RATE;
        float progress = t / duration;

        // Quick attack, slow decay
        float envelope = expf(-3.0f * t);
        if (t < 0.01f) envelope = t / 0.01f;

        // Clean sine with slight detuned layer
        float sample = sinf(2.0f * PI * freq * t) * 0.7f;
        sample += sinf(2.0f * PI * freq * 1.003f * t) * 0.3f;  // Slight chorus
        sample *= envelope * amplitude;

        int current = data[i];
        data[i] = (short)(current + sample * 6000);
    }
}

// Generate the procedural ambient music
static Wave GenerateAmbientMusic() {
    // 30 seconds of music that loops seamlessly
    float musicDuration = 30.0f;
    int sampleCount = (int)(SAMPLE_RATE * musicDuration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;

    // Clear buffer
    for (int i = 0; i < sampleCount; i++) {
        data[i] = 0;
    }

    // Layer 1: Slow drone pad (root note)
    float droneFreq = NoteToFreq(36);  // C2 - low drone
    for (float t = 0.0f; t < musicDuration; t += 8.0f) {
        AddPadTone(data, sampleCount, droneFreq, t, 10.0f, 0.3f);
    }

    // Layer 2: Mid-range pad chords (change every 4 bars)
    int chordProgression[] = { 0, 3, 5, 3 };  // i - iv - v - iv in scale degrees
    for (int bar = 0; bar < 8; bar++) {
        float barStart = bar * 4.0f;
        int chordRoot = AMBIENT_SCALE[chordProgression[bar % 4]];

        // Play chord (root + fifth)
        AddPadTone(data, sampleCount, NoteToFreq(chordRoot), barStart, 5.0f, 0.25f);
        AddPadTone(data, sampleCount, NoteToFreq(chordRoot + 7), barStart + 0.5f, 4.5f, 0.15f);
    }

    // Layer 3: Gentle arpeggio pattern
    float noteTime = 0.5f;  // Half-second per note
    int arpPattern[] = { 0, 2, 4, 5, 4, 2 };  // Up and down pattern
    int arpLength = 6;

    for (float t = 2.0f; t < musicDuration - 2.0f; t += noteTime) {
        // Vary which notes play (some silence for space)
        if ((int)(t * 2) % 5 == 0) continue;  // Skip some beats

        int patternIndex = ((int)(t / noteTime)) % arpLength;
        int noteIndex = arpPattern[patternIndex];
        float freq = NoteToFreq(AMBIENT_SCALE[noteIndex + 2]);  // Start from higher octave

        // Vary amplitude slightly
        float amp = 0.2f + 0.1f * sinf(t * 0.3f);
        AddArpNote(data, sampleCount, freq, t, 0.8f, amp);
    }

    // Fade out last second for seamless loop
    int fadeStart = sampleCount - SAMPLE_RATE;
    for (int i = fadeStart; i < sampleCount; i++) {
        float fadeProgress = (float)(i - fadeStart) / SAMPLE_RATE;
        data[i] = (short)(data[i] * (1.0f - fadeProgress));
    }

    // Fade in first second
    for (int i = 0; i < SAMPLE_RATE; i++) {
        float fadeProgress = (float)i / SAMPLE_RATE;
        data[i] = (short)(data[i] * fadeProgress);
    }

    return wave;
}

void InitBackgroundMusic() {
    if (musicLoaded) {
        UnloadBackgroundMusic();
    }

    Wave musicWave = GenerateAmbientMusic();
    ambientMusic = LoadSoundFromWave(musicWave);
    UnloadWave(musicWave);

    SetSoundVolume(ambientMusic, musicVolume);
    PlaySound(ambientMusic);
    musicLoaded = true;

    TraceLog(LOG_INFO, "Procedural ambient music initialized");
}

void UpdateBackgroundMusic() {
    if (musicLoaded && !IsSoundPlaying(ambientMusic)) {
        // Loop the music
        PlaySound(ambientMusic);
    }
}

void SetMusicVolume(float volume) {
    musicVolume = volume;
    if (musicLoaded) {
        SetSoundVolume(ambientMusic, musicVolume);
    }
}

void UnloadBackgroundMusic() {
    if (musicLoaded) {
        StopSound(ambientMusic);
        UnloadSound(ambientMusic);
        musicLoaded = false;
    }
}

bool IsMusicLoaded() {
    return musicLoaded;
}
