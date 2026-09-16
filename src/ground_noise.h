#ifndef GROUND_NOISE_H
#define GROUND_NOISE_H

// C++ port of the noise and ground-cover logic in shaders/common/frame.glsl and
// shaders/common/terrain.glsl, so grass blades are placed and coloured exactly
// where the terrain shader paints grass. Keep the two in sync.

#include "types.h"
#include <cmath>

namespace ground {

inline float Fract(float x) { return x - floorf(x); }
inline float Saturate(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
inline float SmoothStep(float a, float b, float x) {
    float t = Saturate((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}
inline float Mix(float a, float b, float t) { return a + (b - a) * t; }

inline float Hash(float px, float py) {
    float p3x = Fract(px * 0.1031f), p3y = Fract(py * 0.1031f), p3z = Fract(px * 0.1031f);
    float d = p3x * (p3y + 33.33f) + p3y * (p3z + 33.33f) + p3z * (p3x + 33.33f);
    p3x += d; p3y += d; p3z += d;
    return Fract((p3x + p3y) * p3z);
}

inline float Noise(float px, float py) {
    float ix = floorf(px), iy = floorf(py);
    float fx = px - ix, fy = py - iy;
    float ux = fx * fx * fx * (fx * (fx * 6.0f - 15.0f) + 10.0f);
    float uy = fy * fy * fy * (fy * (fy * 6.0f - 15.0f) + 10.0f);
    float a = Hash(ix, iy), b = Hash(ix + 1.0f, iy);
    float c = Hash(ix, iy + 1.0f), d = Hash(ix + 1.0f, iy + 1.0f);
    return Mix(Mix(a, b, ux), Mix(c, d, ux), uy);
}

inline float Fbm(float px, float py, int octaves) {
    float value = 0.0f, amplitude = 0.5f;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * Noise(px, py);
        float rx = 0.8f * px - 0.6f * py, ry = 0.6f * px + 0.8f * py;
        px = rx * 2.03f + 17.1f;
        py = ry * 2.03f + 17.1f;
        amplitude *= 0.5f;
    }
    return value;
}

inline float BoxDistance(float x, float z, float cx, float cz, float w, float l) {
    float dx = fabsf(x - cx) - w * 0.5f, dz = fabsf(z - cz) - l * 0.5f;
    float ox = dx > 0 ? dx : 0, oz = dz > 0 ? dz : 0;
    float inside = fmaxf(dx, dz);
    return sqrtf(ox * ox + oz * oz) + (inside < 0 ? inside : 0);
}

struct GrassCover {
    float density;     // 0 bare .. 1 full grass
    float r, g, b;     // linear albedo of the grass
    float tallness;    // 0 short lawn .. 1 tall meadow
    float flower;      // > 0: spring flower hue selector
};

inline void GrassColor(int season, float variation, float dryness, float* r, float* g, float* b) {
    float a[3], c[3], dry[3];
    if (season == 0) {
        float A[3] = {0.07f, 0.19f, 0.025f}, B[3] = {0.19f, 0.34f, 0.04f}, D[3] = {0.30f, 0.32f, 0.08f};
        for (int i = 0; i < 3; i++) { a[i] = A[i]; c[i] = B[i]; dry[i] = D[i]; }
    } else if (season == 2) {
        float A[3] = {0.14f, 0.12f, 0.03f}, B[3] = {0.32f, 0.21f, 0.05f}, D[3] = {0.42f, 0.27f, 0.08f};
        for (int i = 0; i < 3; i++) { a[i] = A[i]; c[i] = B[i]; dry[i] = D[i]; }
    } else {
        float A[3] = {0.05f, 0.15f, 0.022f}, B[3] = {0.13f, 0.28f, 0.035f}, D[3] = {0.28f, 0.26f, 0.08f};
        for (int i = 0; i < 3; i++) { a[i] = A[i]; c[i] = B[i]; dry[i] = D[i]; }
    }
    float out[3];
    for (int i = 0; i < 3; i++) out[i] = Mix(Mix(a[i], c[i], variation), dry[i], dryness);
    *r = out[0]; *g = out[1]; *b = out[2];
}

// Mirrors sampleGround() for the grass-relevant terms.
inline GrassCover SampleGrass(float x, float z, float slope, int season,
                              const Sand* sand, int sandCount, const Water* water, int waterCount) {
    GrassCover gc = {};
    float macro = Fbm(x * 0.011f, z * 0.011f, 3);
    float meso = Fbm(x * 0.085f + 13.0f, z * 0.085f + 13.0f, 3);
    float micro = Noise(x * 1.1f, z * 1.1f);
    float dryness = SmoothStep(0.55f, 0.78f, macro) * 0.55f + SmoothStep(0.6f, 0.9f, meso) * 0.25f;
    GrassColor(season, Saturate(meso * 1.25f - 0.1f + (micro - 0.5f) * 0.3f), dryness, &gc.r, &gc.g, &gc.b);
    float tone = 0.8f + 0.35f * micro;
    gc.r *= tone; gc.g *= tone; gc.b *= tone;

    float waterDist = 1e4f;
    for (int i = 0; i < waterCount && i < 16; i++) {
        waterDist = fminf(waterDist, BoxDistance(x, z, water[i].position.x, water[i].position.z, water[i].width, water[i].length));
    }
    float shore = 1.0f - SmoothStep(0.0f, 3.5f + micro * 2.0f, waterDist);
    float worn = SmoothStep(0.68f, 0.8f, Fbm(x * 0.045f + 31.0f, z * 0.045f + 31.0f, 3)) * 0.8f;
    float dirt = fmaxf(fmaxf(worn, SmoothStep(0.18f, 0.35f, slope)), shore * 0.85f);

    float sandAmount = 0.0f;
    if (sandCount > 0) {
        float wobble = (Noise(x * 0.11f, z * 0.11f) - 0.5f) * 7.0f + (Noise(x * 0.6f, z * 0.6f) - 0.5f) * 1.5f;
        for (int i = 0; i < sandCount && i < 16; i++) {
            float d = BoxDistance(x, z, sand[i].position.x, sand[i].position.z, sand[i].width, sand[i].length) + wobble;
            sandAmount = fmaxf(sandAmount, 1.0f - SmoothStep(-5.0f, 1.5f, d));
        }
    }
    float density = (1.0f - dirt) * (1.0f - sandAmount);
    if (waterDist < 0.2f) density = 0.0f;
    if (season == 3) {
        float bare = SmoothStep(0.08f, 0.02f, Noise(x * 0.35f + 50.0f, z * 0.35f + 50.0f)) * 0.9f + shore * 0.6f;
        float snow = Saturate(1.0f - bare - SmoothStep(0.4f, 0.7f, slope));
        // Only clumps of dry, straw-coloured stalks poke through the snow
        float tuft = SmoothStep(0.66f, 0.74f, Noise(x * 0.45f + 80.0f, z * 0.45f + 80.0f));
        density *= 1.0f - snow * (1.0f - 0.3f * tuft);
        gc.r = 0.30f; gc.g = 0.24f; gc.b = 0.12f;
    }
    gc.density = Saturate(density);
    gc.tallness = Saturate(SmoothStep(0.35f, 0.7f, Fbm(x * 0.03f + 7.0f, z * 0.03f + 7.0f, 2)) * (1.0f - worn));
    gc.flower = 0.0f;
    return gc;
}

}  // namespace ground

#endif
