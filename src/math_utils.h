#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "raylib.h"
#include <cmath>

// Heightmap system for terrain
const int HEIGHTMAP_SIZE = 512;  // 512x512 grid
const float HEIGHTMAP_SCALE = 1.0f;  // 1 unit per cell (covers -256 to +256)
const float HEIGHTMAP_OFFSET = 256.0f;  // Center offset

// Global heightmap data (initialized in main.cpp)
extern float g_heightmap[HEIGHTMAP_SIZE][HEIGHTMAP_SIZE];
extern bool g_heightmapInitialized;

// Terrain noise functions for initial generation
inline float TerrainHash(float x, float y) {
    float v = sinf(x * 127.1f + y * 311.7f) * 43758.5453f;
    return v - floorf(v);
}

inline float TerrainNoise(float x, float y) {
    float ix = floorf(x);
    float iy = floorf(y);
    float fx = x - ix;
    float fy = y - iy;

    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);

    float a = TerrainHash(ix, iy);
    float b = TerrainHash(ix + 1.0f, iy);
    float c = TerrainHash(ix, iy + 1.0f);
    float d = TerrainHash(ix + 1.0f, iy + 1.0f);

    float ab = a + (b - a) * fx;
    float cd = c + (d - c) * fx;
    return ab + (cd - ab) * fy;
}

// Generate base procedural height at a point
inline float GenerateProceduralHeight(float x, float z) {
    float height = 0.0f;
    height += TerrainNoise(x * 0.02f, z * 0.02f) * 2.0f;
    height += TerrainNoise(x * 0.08f, z * 0.08f) * 0.6f;
    height += TerrainNoise(x * 0.2f, z * 0.2f) * 0.2f;
    return height;
}

// Sample heightmap with bilinear interpolation
inline float GetTerrainHeight(float x, float z) {
    if (!g_heightmapInitialized) {
        return GenerateProceduralHeight(x, z);
    }

    // Convert world coords to heightmap coords
    float hx = (x + HEIGHTMAP_OFFSET) / HEIGHTMAP_SCALE;
    float hz = (z + HEIGHTMAP_OFFSET) / HEIGHTMAP_SCALE;

    // Clamp to valid range
    if (hx < 0) hx = 0;
    if (hz < 0) hz = 0;
    if (hx >= HEIGHTMAP_SIZE - 1) hx = HEIGHTMAP_SIZE - 1.001f;
    if (hz >= HEIGHTMAP_SIZE - 1) hz = HEIGHTMAP_SIZE - 1.001f;

    // Bilinear interpolation
    int ix = (int)hx;
    int iz = (int)hz;
    float fx = hx - ix;
    float fz = hz - iz;

    float h00 = g_heightmap[iz][ix];
    float h10 = g_heightmap[iz][ix + 1];
    float h01 = g_heightmap[iz + 1][ix];
    float h11 = g_heightmap[iz + 1][ix + 1];

    float h0 = h00 + (h10 - h00) * fx;
    float h1 = h01 + (h11 - h01) * fx;
    return h0 + (h1 - h0) * fz;
}

// Distance helper
inline float Distance3D(Vector3 a, Vector3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

// Vector normalization
inline Vector3 Normalize3D(Vector3 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len > 0) {
        return { v.x/len, v.y/len, v.z/len };
    }
    return { 0, 0, 0 };
}

// Dot product
inline float Dot3D(Vector3 a, Vector3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

// Random float in range
inline float RandomFloat(float min, float max) {
    return min + (float)GetRandomValue(0, 10000) / 10000.0f * (max - min);
}

// Check if player is facing a target (within ~60 degree cone)
inline bool IsFacing(Camera3D& camera, Vector3 targetPos) {
    Vector3 forward = {
        camera.target.x - camera.position.x,
        0,
        camera.target.z - camera.position.z
    };
    forward = Normalize3D(forward);

    Vector3 toTarget = {
        targetPos.x - camera.position.x,
        0,
        targetPos.z - camera.position.z
    };
    toTarget = Normalize3D(toTarget);

    float dot = Dot3D(forward, toTarget);
    return dot > 0.5f;
}

#endif
