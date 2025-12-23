#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "raylib.h"
#include <cmath>

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
