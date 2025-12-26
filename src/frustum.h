#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "raylib.h"
#include "raymath.h"

// Frustum planes for culling (left, right, bottom, top, near, far)
struct Frustum {
    Vector4 planes[6];  // ax + by + cz + d = 0 form (normalized)
};

// Extract frustum planes from view-projection matrix
// Uses the Gribb/Hartmann method for plane extraction
void ExtractFrustumPlanes(Frustum* frustum, Matrix viewProj);

// Test if a point is inside the frustum
bool PointInFrustum(const Frustum* frustum, Vector3 point);

// Test if a sphere is inside or intersecting the frustum
// Returns true if any part of the sphere is visible
bool SphereInFrustum(const Frustum* frustum, Vector3 center, float radius);

// Test if an axis-aligned bounding box is inside or intersecting the frustum
bool AABBInFrustum(const Frustum* frustum, Vector3 min, Vector3 max);

#endif // FRUSTUM_H
