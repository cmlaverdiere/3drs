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

// Orthographic light matrices centred on `center`, snapped to whole shadow
// texels in light space so the projection is stable while the camera moves.
inline ShadowMatrices CalculateShadowMatrices(Vector3 center, Vector3 direction,
                                              float orthoSize = SHADOW_ORTHO_SIZE,
                                              int resolution = SHADOW_MAP_RESOLUTION) {
    direction = Vector3Normalize(direction);
    Vector3 referenceUp = fabsf(direction.y) > 0.99f ? Vector3{0, 0, 1} : Vector3{0, 1, 0};
    Vector3 right = Vector3Normalize(Vector3CrossProduct(direction, referenceUp));
    Vector3 up = Vector3CrossProduct(right, direction);
    const float texel = orthoSize / resolution;
    float x = Vector3DotProduct(center, right);
    float y = Vector3DotProduct(center, up);
    center = Vector3Add(center, Vector3Scale(right, roundf(x / texel) * texel - x));
    center = Vector3Add(center, Vector3Scale(up, roundf(y / texel) * texel - y));
    ShadowMatrices result;
    result.view = MatrixLookAt(Vector3Subtract(center, Vector3Scale(direction, SHADOW_FAR * 0.5f)), center, up);
    const float half = orthoSize * 0.5f;
    result.projection = MatrixOrtho(-half, half, -half, half, SHADOW_NEAR, SHADOW_FAR);
    result.viewProjection = MatrixMultiply(result.view, result.projection);
    return result;
}

// Bounding sphere of the view-frustum slice [nearDist, farDist]. The slice is
// symmetric about the view axis, so the radius is rotation invariant and the
// cascade does not resize (or shimmer) as the camera turns.
inline void FrustumSliceSphere(Vector3 eye, Vector3 forward, float tanHalfFovY, float aspect,
                               float nearDist, float farDist, Vector3* center, float* radius) {
    float tanX = tanHalfFovY * aspect;
    float nearDiag2 = nearDist * nearDist * (tanX * tanX + tanHalfFovY * tanHalfFovY);
    float farDiag2 = farDist * farDist * (tanX * tanX + tanHalfFovY * tanHalfFovY);
    // Centre on the axis at distance d, equidistant to near and far corners.
    float d = 0.5f * (nearDist + farDist) + 0.5f * (farDiag2 - nearDiag2) / (farDist - nearDist);
    d = fminf(d, farDist);
    float r2 = (farDist - d) * (farDist - d) + farDiag2;
    *center = Vector3Add(eye, Vector3Scale(forward, d));
    *radius = sqrtf(r2);
}

#endif
