#include "collision.h"

bool PointInWall(Vector3 point, Wall& wall, float padding) {
    Vector3 wpos = wall.position;
    float halfW = wall.width / 2.0f + padding;
    float halfD = wall.depth / 2.0f + padding;

    return (point.x >= wpos.x - halfW && point.x <= wpos.x + halfW &&
            point.z >= wpos.z - halfD && point.z <= wpos.z + halfD);
}

Vector3 ResolveWallCollision(Vector3 pos, Wall& wall, float padding) {
    Vector3 wpos = wall.position;
    float halfW = wall.width / 2.0f + padding;
    float halfD = wall.depth / 2.0f + padding;

    float distLeft = pos.x - (wpos.x - halfW);
    float distRight = (wpos.x + halfW) - pos.x;
    float distBack = pos.z - (wpos.z - halfD);
    float distFront = (wpos.z + halfD) - pos.z;

    float minDist = distLeft;
    Vector3 result = pos;

    if (distRight < minDist) {
        minDist = distRight;
    }
    if (distBack < minDist) {
        minDist = distBack;
    }
    if (distFront < minDist) {
        minDist = distFront;
    }

    if (minDist == distLeft) {
        result.x = wpos.x - halfW;
    } else if (minDist == distRight) {
        result.x = wpos.x + halfW;
    } else if (minDist == distBack) {
        result.z = wpos.z - halfD;
    } else if (minDist == distFront) {
        result.z = wpos.z + halfD;
    }

    return result;
}
