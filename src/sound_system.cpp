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
static int currentMusicSeason = -1;

// Scale definitions for different moods
static const int SCALE_MINOR_PENT[] = { 48, 51, 53, 55, 58, 60, 63, 65, 67, 70 };  // C minor pentatonic
static const int SCALE_MAJOR[] = { 48, 50, 52, 53, 55, 57, 59, 60, 62, 64 };       // C major
static const int SCALE_DORIAN[] = { 48, 50, 51, 53, 55, 57, 58, 60, 62, 63 };      // C dorian (minor but brighter)
static const int SCALE_LYDIAN[] = { 48, 50, 52, 54, 55, 57, 59, 60, 62, 64 };      // C lydian (dreamy major)

// Generate a soft pad tone (sine with slow attack/release)
static void AddPadTone(short* data, int sampleCount, float freq, float startTime, float duration, float amplitude, float attack = 0.3f, float release = 0.5f) {
    int startSample = (int)(startTime * SAMPLE_RATE);
    int durationSamples = (int)(duration * SAMPLE_RATE);
    int endSample = startSample + durationSamples;
    if (endSample > sampleCount) endSample = sampleCount;

    for (int i = startSample; i < endSample; i++) {
        float t = (float)(i - startSample) / SAMPLE_RATE;
        float progress = t / duration;

        float envelope = 1.0f;
        if (t < attack) {
            envelope = t / attack;
        } else if (progress > (1.0f - release / duration)) {
            envelope = (duration - t) / release;
        }
        envelope = envelope * envelope;

        float sample = sinf(2.0f * PI * freq * t) * 0.6f;
        sample += sinf(2.0f * PI * freq * 2.0f * t) * 0.2f;
        sample += sinf(2.0f * PI * freq * 0.5f * t) * 0.2f;
        sample *= envelope * amplitude;

        int current = data[i];
        data[i] = (short)(current + sample * 8000);
    }
}

// Generate an arpeggio note
static void AddArpNote(short* data, int sampleCount, float freq, float startTime, float duration, float amplitude, float decay = 3.0f) {
    int startSample = (int)(startTime * SAMPLE_RATE);
    int durationSamples = (int)(duration * SAMPLE_RATE);
    int endSample = startSample + durationSamples;
    if (endSample > sampleCount) endSample = sampleCount;

    for (int i = startSample; i < endSample; i++) {
        float t = (float)(i - startSample) / SAMPLE_RATE;

        float envelope = expf(-decay * t);
        if (t < 0.01f) envelope = t / 0.01f;

        float sample = sinf(2.0f * PI * freq * t) * 0.7f;
        sample += sinf(2.0f * PI * freq * 1.003f * t) * 0.3f;
        sample *= envelope * amplitude;

        int current = data[i];
        data[i] = (short)(current + sample * 6000);
    }
}

// Add a plucky string sound
static void AddPluck(short* data, int sampleCount, float freq, float startTime, float duration, float amplitude) {
    int startSample = (int)(startTime * SAMPLE_RATE);
    int durationSamples = (int)(duration * SAMPLE_RATE);
    int endSample = startSample + durationSamples;
    if (endSample > sampleCount) endSample = sampleCount;

    for (int i = startSample; i < endSample; i++) {
        float t = (float)(i - startSample) / SAMPLE_RATE;
        float envelope = expf(-5.0f * t);
        if (t < 0.005f) envelope = t / 0.005f;

        // Karplus-Strong-ish: fundamental + decaying harmonics
        float sample = sinf(2.0f * PI * freq * t);
        sample += 0.5f * sinf(4.0f * PI * freq * t) * expf(-8.0f * t);
        sample += 0.25f * sinf(6.0f * PI * freq * t) * expf(-12.0f * t);
        sample *= envelope * amplitude;

        int current = data[i];
        data[i] = (short)(current + sample * 7000);
    }
}

// Add crossfade for seamless looping
static void ApplyLoopFades(short* data, int sampleCount) {
    int fadeLen = SAMPLE_RATE;
    for (int i = 0; i < fadeLen; i++) {
        float progress = (float)i / fadeLen;
        data[i] = (short)(data[i] * progress);
        data[sampleCount - fadeLen + i] = (short)(data[sampleCount - fadeLen + i] * (1.0f - progress));
    }
}

// ============ SUMMER: Bright, joyful, major key ============
static Wave GenerateSummerMusic() {
    float duration = 32.0f;
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;
    for (int i = 0; i < sampleCount; i++) data[i] = 0;

    // Bright, higher drone on C
    float droneFreq = NoteToFreq(48);  // C3
    for (float t = 0.0f; t < duration; t += 8.0f) {
        AddPadTone(data, sampleCount, droneFreq, t, 10.0f, 0.2f, 0.5f, 0.5f);
        AddPadTone(data, sampleCount, droneFreq * 1.5f, t + 0.3f, 9.0f, 0.12f, 0.5f, 0.5f);  // Fifth for brightness
    }

    // Happy major chords with consistent timing
    for (int bar = 0; bar < 8; bar++) {
        float barStart = bar * 4.0f;
        // I - V - vi - IV (happy pop progression)
        int roots[] = { 0, 7, 9, 5 };
        int root = SCALE_MAJOR[roots[bar % 4] % 10];
        AddPadTone(data, sampleCount, NoteToFreq(root + 12), barStart, 4.0f, 0.18f, 0.2f, 0.3f);
        AddPadTone(data, sampleCount, NoteToFreq(root + 16), barStart, 4.0f, 0.12f, 0.2f, 0.3f);  // Major third
        AddPadTone(data, sampleCount, NoteToFreq(root + 19), barStart, 4.0f, 0.10f, 0.2f, 0.3f);  // Fifth
    }

    // Bouncy, rhythmic arpeggio - ascending pattern
    int arpNotes[] = { 0, 2, 4, 7, 4, 2 };
    float noteLen = 0.4f;
    for (int beat = 0; beat < 80; beat++) {
        float t = 1.0f + beat * noteLen;
        if (t > duration - 2.0f) break;
        int noteIdx = arpNotes[beat % 6];
        float freq = NoteToFreq(SCALE_MAJOR[noteIdx] + 24);  // High octave for brightness
        AddPluck(data, sampleCount, freq, t, 0.35f, 0.25f);
    }

    // Occasional high sparkle notes
    for (float t = 2.0f; t < duration - 2.0f; t += 3.2f) {
        int sparkleNote = SCALE_MAJOR[(int)(t * 1.3f) % 5 + 4];
        AddPluck(data, sampleCount, NoteToFreq(sparkleNote + 24), t, 0.5f, 0.18f);
    }

    ApplyLoopFades(data, sampleCount);
    return wave;
}

// ============ AUTUMN: Melancholic, sparse, dorian mode ============
static Wave GenerateAutumnMusic() {
    float duration = 36.0f;
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;
    for (int i = 0; i < sampleCount; i++) data[i] = 0;

    // Low, somber drone
    float droneFreq = NoteToFreq(41);  // F2
    for (float t = 0.0f; t < duration; t += 10.0f) {
        AddPadTone(data, sampleCount, droneFreq, t, 12.0f, 0.3f, 0.5f, 0.8f);
    }

    // Sparse minor chords with longer sustain
    int chordRoots[] = { 0, 5, 3, 0 };
    for (int bar = 0; bar < 6; bar++) {
        float barStart = bar * 6.0f;
        int root = SCALE_DORIAN[chordRoots[bar % 4]];
        AddPadTone(data, sampleCount, NoteToFreq(root), barStart, 7.0f, 0.22f, 0.6f, 1.0f);
        AddPadTone(data, sampleCount, NoteToFreq(root + 7), barStart + 1.0f, 6.0f, 0.15f, 0.6f, 1.0f);
    }

    // Falling leaf-like notes - descending patterns with gaps
    int pattern[] = { 7, 5, 4, 2, 0 };
    for (int phrase = 0; phrase < 4; phrase++) {
        float phraseStart = 3.0f + phrase * 8.0f;
        for (int i = 0; i < 5; i++) {
            // Add silence gaps
            if (i == 2) continue;
            float t = phraseStart + i * 1.0f;
            if (t > duration - 2.0f) break;
            int note = SCALE_DORIAN[pattern[i]];
            AddArpNote(data, sampleCount, NoteToFreq(note + 12), t, 1.5f, 0.25f, 2.0f);
        }
    }

    ApplyLoopFades(data, sampleCount);
    return wave;
}

// ============ WINTER: Cold, ethereal, rhythmically aligned ============
static Wave GenerateWinterMusic() {
    float duration = 32.0f;  // Even duration for clean loop
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;
    for (int i = 0; i < sampleCount; i++) data[i] = 0;

    // Low drone - aligned to 8-bar phrases
    float droneFreq = NoteToFreq(36);  // C2
    for (float t = 0.0f; t < duration; t += 8.0f) {
        AddPadTone(data, sampleCount, droneFreq, t, 9.0f, 0.3f, 0.5f, 0.5f);
    }

    // Minor pad chords - 4 seconds each, aligned
    int chordRoots[] = { 0, 3, 5, 3 };  // i - iv - v - iv
    for (int bar = 0; bar < 8; bar++) {
        float barStart = bar * 4.0f;
        int rootIdx = chordRoots[bar % 4];
        int root = SCALE_MINOR_PENT[rootIdx];
        AddPadTone(data, sampleCount, NoteToFreq(root), barStart, 4.5f, 0.25f, 0.4f, 0.4f);
        AddPadTone(data, sampleCount, NoteToFreq(root + 7), barStart, 4.5f, 0.15f, 0.4f, 0.4f);
    }

    // Gentle arpeggio - strict rhythmic grid (8th notes = 0.5s at 60bpm)
    int arpPattern[] = { 0, 2, 4, 5, 4, 2 };
    float noteTime = 0.5f;
    for (int beat = 0; beat < 56; beat++) {  // 28 seconds of arpeggios
        float t = 2.0f + beat * noteTime;
        if (t > duration - 2.0f) break;

        // Skip every 5th note for breathing room
        if (beat % 5 == 4) continue;

        int patternIndex = beat % 6;
        int noteIndex = arpPattern[patternIndex];
        float freq = NoteToFreq(SCALE_MINOR_PENT[noteIndex + 2]);
        AddArpNote(data, sampleCount, freq, t, 0.8f, 0.22f, 3.0f);
    }

    ApplyLoopFades(data, sampleCount);
    return wave;
}

// ============ SPRING: Fresh, hopeful, lydian mode ============
static Wave GenerateSpringMusic() {
    float duration = 28.0f;
    int sampleCount = (int)(SAMPLE_RATE * duration);
    Wave wave = CreateWave(sampleCount);
    short* data = (short*)wave.data;
    for (int i = 0; i < sampleCount; i++) data[i] = 0;

    // Light, airy drone
    float droneFreq = NoteToFreq(48);  // C3 - higher than others
    for (float t = 0.0f; t < duration; t += 7.0f) {
        AddPadTone(data, sampleCount, droneFreq, t, 9.0f, 0.2f, 0.4f, 0.6f);
        AddPadTone(data, sampleCount, droneFreq * 1.5f, t + 0.5f, 8.0f, 0.12f, 0.4f, 0.6f);  // Fifth
    }

    // Lydian chords - dreamy, uplifting
    int chordRoots[] = { 0, 4, 2, 5 };
    for (int bar = 0; bar < 7; bar++) {
        float barStart = bar * 4.0f;
        int root = SCALE_LYDIAN[chordRoots[bar % 4]];
        AddPadTone(data, sampleCount, NoteToFreq(root + 12), barStart, 4.5f, 0.2f, 0.3f, 0.5f);
        AddPadTone(data, sampleCount, NoteToFreq(root + 16), barStart + 0.3f, 4.0f, 0.12f, 0.3f, 0.5f);
    }

    // Birdsong-like quick arpeggios - ascending patterns
    int birdPattern[] = { 0, 2, 4, 7, 9 };
    for (int phrase = 0; phrase < 6; phrase++) {
        float phraseStart = 1.5f + phrase * 4.5f;
        for (int i = 0; i < 5; i++) {
            float t = phraseStart + i * 0.2f;
            if (t > duration - 2.0f) break;
            int note = SCALE_LYDIAN[birdPattern[i]];
            AddPluck(data, sampleCount, NoteToFreq(note + 24), t, 0.4f, 0.22f);  // High register
        }
    }

    // Gentle flowing melody
    int melody[] = { 4, 5, 7, 9, 7, 5 };
    for (int i = 0; i < 10; i++) {
        float t = 3.0f + i * 2.0f;
        if (t > duration - 3.0f) break;
        int note = SCALE_LYDIAN[melody[i % 6]];
        AddArpNote(data, sampleCount, NoteToFreq(note + 12), t, 1.5f, 0.25f, 2.5f);
    }

    ApplyLoopFades(data, sampleCount);
    return wave;
}

static Wave GenerateMusicForSeason(int season) {
    switch (season) {
        case 0: return GenerateSummerMusic();
        case 1: return GenerateAutumnMusic();
        case 2: return GenerateWinterMusic();
        case 3: return GenerateSpringMusic();
        default: return GenerateSummerMusic();
    }
}

void InitBackgroundMusic(int season) {
    if (musicLoaded) {
        UnloadBackgroundMusic();
    }

    Wave musicWave = GenerateMusicForSeason(season);
    ambientMusic = LoadSoundFromWave(musicWave);
    UnloadWave(musicWave);

    SetSoundVolume(ambientMusic, musicVolume);
    PlaySound(ambientMusic);
    musicLoaded = true;
    currentMusicSeason = season;

    const char* seasonNames[] = { "Summer", "Autumn", "Winter", "Spring" };
    TraceLog(LOG_INFO, "Background music initialized: %s theme", seasonNames[season]);
}

void UpdateBackgroundMusic(int currentSeason) {
    // Switch music if season changed
    if (currentSeason != currentMusicSeason) {
        InitBackgroundMusic(currentSeason);
        return;
    }

    // Loop current music
    if (musicLoaded && !IsSoundPlaying(ambientMusic)) {
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
