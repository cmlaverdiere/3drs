#ifndef LIGHTING_MATH_H
#define LIGHTING_MATH_H

#include "raymath.h"
#include <cmath>

constexpr int SHADOW_MAP_RESOLUTION = 4096;
constexpr float SHADOW_ORTHO_SIZE = 200.0f;
constexpr float SHADOW_NEAR = 0.1f;
constexpr float SHADOW_FAR = 400.0f;

struct ShadowMatrices {
    Matrix view;
    Matrix projection;
    Matrix viewProjection;
};

inline ShadowMatrices CalculateShadowMatrices(Vector3 center, Vector3 direction) {
    direction = Vector3Normalize(direction);
    Vector3 referenceUp = fabsf(direction.y) > 0.99f ? Vector3{0, 0, 1} : Vector3{0, 1, 0};
    Vector3 right = Vector3Normalize(Vector3CrossProduct(direction, referenceUp));
    Vector3 up = Vector3CrossProduct(right, direction);
    const float texel = SHADOW_ORTHO_SIZE / SHADOW_MAP_RESOLUTION;
    float x = Vector3DotProduct(center, right);
    float y = Vector3DotProduct(center, up);
    center = Vector3Add(center, Vector3Scale(right, roundf(x / texel) * texel - x));
    center = Vector3Add(center, Vector3Scale(up, roundf(y / texel) * texel - y));
    ShadowMatrices result;
    result.view = MatrixLookAt(Vector3Subtract(center, Vector3Scale(direction, SHADOW_FAR * 0.5f)), center, up);
    const float half = SHADOW_ORTHO_SIZE * 0.5f;
    result.projection = MatrixOrtho(-half, half, -half, half, SHADOW_NEAR, SHADOW_FAR);
    result.viewProjection = MatrixMultiply(result.view, result.projection);
    return result;
}

#endif
