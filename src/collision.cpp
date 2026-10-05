#include "collision.h"

bool PointInWall(Vector3 point, const Wall& wall, float padding) {
    float lx, lz;
    WallLocal(wall, point.x, point.z, &lx, &lz);
    return fabsf(lx) <= wall.width / 2.0f + padding && fabsf(lz) <= wall.depth / 2.0f + padding;
}

// Push the point out through the nearest face, in the wall's own frame
Vector3 ResolveWallCollision(Vector3 pos, const Wall& wall, float padding) {
    float halfW = wall.width / 2.0f + padding;
    float halfD = wall.depth / 2.0f + padding;
    float lx, lz;
    WallLocal(wall, pos.x, pos.z, &lx, &lz);

    float distLeft = lx + halfW;
    float distRight = halfW - lx;
    float distBack = lz + halfD;
    float distFront = halfD - lz;

    float minDist = fminf(fminf(distLeft, distRight), fminf(distBack, distFront));
    if (minDist == distLeft) {
        lx = -halfW;
    } else if (minDist == distRight) {
        lx = halfW;
    } else if (minDist == distBack) {
        lz = -halfD;
    } else {
        lz = halfD;
    }

    float dx, dz;
    WallWorld(wall, lx, lz, &dx, &dz);
    Vector3 result = pos;
    result.x = wall.position.x + dx;
    result.z = wall.position.z + dz;
    return result;
}
