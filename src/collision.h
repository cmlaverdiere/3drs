#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"
#include <math.h>

// Wall-local horizontal coordinates of a world point: lx along the width, lz along the depth
inline void WallLocal(const Wall& wall, float x, float z, float* lx, float* lz) {
    float dx = x - wall.position.x, dz = z - wall.position.z;
    if (wall.yaw == 0.0f) { *lx = dx; *lz = dz; return; }
    float c = cosf(wall.yaw), s = sinf(wall.yaw);
    *lx = dx * c - dz * s;
    *lz = dx * s + dz * c;
}

// World offset from the wall centre of a wall-local point
inline void WallWorld(const Wall& wall, float lx, float lz, float* dx, float* dz) {
    if (wall.yaw == 0.0f) { *dx = lx; *dz = lz; return; }
    float c = cosf(wall.yaw), s = sinf(wall.yaw);
    *dx = lx * c + lz * s;
    *dz = -lx * s + lz * c;
}

// Half extents of the wall's axis-aligned footprint
inline void WallFootprint(const Wall& wall, float* halfX, float* halfZ) {
    float c = fabsf(cosf(wall.yaw)), s = fabsf(sinf(wall.yaw));
    *halfX = c * wall.width * 0.5f + s * wall.depth * 0.5f;
    *halfZ = s * wall.width * 0.5f + c * wall.depth * 0.5f;
}

// Check if a point is inside a wall's footprint (with padding for player radius)
bool PointInWall(Vector3 point, const Wall& wall, float padding);

// Resolve collision between player and wall, returns adjusted position
Vector3 ResolveWallCollision(Vector3 pos, const Wall& wall, float padding);

#endif
