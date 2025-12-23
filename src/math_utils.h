#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "raylib.h"
#include <cmath>

// Terrain noise functions (must match grass.vs shader)
inline float TerrainHash(float x, float y) {
    float v = sinf(x * 127.1f + y * 311.7f) * 43758.5453f;
    return v - floorf(v);  // GLSL fract equivalent
}

inline float TerrainNoise(float x, float y) {
    float ix = floorf(x);
    float iy = floorf(y);
    float fx = x - ix;
    float fy = y - iy;

    // Smoothstep
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

inline float GetTerrainHeight(float x, float z) {
    float height = 0.0f;

    // Large rolling hills (must match shader)
    height += TerrainNoise(x * 0.02f, z * 0.02f) * 2.5f;

    // Medium bumps
    height += TerrainNoise(x * 0.08f, z * 0.08f) * 0.8f;

    // Small details
    height += TerrainNoise(x * 0.2f, z * 0.2f) * 0.3f;

    return height;
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
