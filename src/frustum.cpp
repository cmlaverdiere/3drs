#include "frustum.h"
#include <cmath>

// Extract frustum planes from combined view-projection matrix
// Uses the Gribb/Hartmann method
void ExtractFrustumPlanes(Frustum* frustum, Matrix vp) {
    // Left plane: row3 + row0
    frustum->planes[0] = (Vector4){
        vp.m3 + vp.m0,
        vp.m7 + vp.m4,
        vp.m11 + vp.m8,
        vp.m15 + vp.m12
    };

    // Right plane: row3 - row0
    frustum->planes[1] = (Vector4){
        vp.m3 - vp.m0,
        vp.m7 - vp.m4,
        vp.m11 - vp.m8,
        vp.m15 - vp.m12
    };

    // Bottom plane: row3 + row1
    frustum->planes[2] = (Vector4){
        vp.m3 + vp.m1,
        vp.m7 + vp.m5,
        vp.m11 + vp.m9,
        vp.m15 + vp.m13
    };

    // Top plane: row3 - row1
    frustum->planes[3] = (Vector4){
        vp.m3 - vp.m1,
        vp.m7 - vp.m5,
        vp.m11 - vp.m9,
        vp.m15 - vp.m13
    };

    // Near plane: row3 + row2
    frustum->planes[4] = (Vector4){
        vp.m3 + vp.m2,
        vp.m7 + vp.m6,
        vp.m11 + vp.m10,
        vp.m15 + vp.m14
    };

    // Far plane: row3 - row2
    frustum->planes[5] = (Vector4){
        vp.m3 - vp.m2,
        vp.m7 - vp.m6,
        vp.m11 - vp.m10,
        vp.m15 - vp.m14
    };

    // Normalize all planes
    for (int i = 0; i < 6; i++) {
        float len = sqrtf(
            frustum->planes[i].x * frustum->planes[i].x +
            frustum->planes[i].y * frustum->planes[i].y +
            frustum->planes[i].z * frustum->planes[i].z
        );
        if (len > 0.0f) {
            frustum->planes[i].x /= len;
            frustum->planes[i].y /= len;
            frustum->planes[i].z /= len;
            frustum->planes[i].w /= len;
        }
    }
}

bool PointInFrustum(const Frustum* frustum, Vector3 point) {
    for (int i = 0; i < 6; i++) {
        float dist = frustum->planes[i].x * point.x +
                    frustum->planes[i].y * point.y +
                    frustum->planes[i].z * point.z +
                    frustum->planes[i].w;
        if (dist < 0.0f) return false;  // Point is outside this plane
    }
    return true;
}

bool SphereInFrustum(const Frustum* frustum, Vector3 center, float radius) {
    for (int i = 0; i < 6; i++) {
        float dist = frustum->planes[i].x * center.x +
                    frustum->planes[i].y * center.y +
                    frustum->planes[i].z * center.z +
                    frustum->planes[i].w;
        if (dist < -radius) return false;  // Sphere is completely outside this plane
    }
    return true;
}

bool AABBInFrustum(const Frustum* frustum, Vector3 min, Vector3 max) {
    for (int i = 0; i < 6; i++) {
        // Find the corner of the AABB that is most in the direction of the plane normal
        Vector3 positive = {
            (frustum->planes[i].x >= 0) ? max.x : min.x,
            (frustum->planes[i].y >= 0) ? max.y : min.y,
            (frustum->planes[i].z >= 0) ? max.z : min.z
        };

        float dist = frustum->planes[i].x * positive.x +
                    frustum->planes[i].y * positive.y +
                    frustum->planes[i].z * positive.z +
                    frustum->planes[i].w;

        if (dist < 0.0f) return false;  // AABB is completely outside this plane
    }
    return true;
}
