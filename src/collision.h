#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"

// Check if a point is inside a wall (with padding for player radius)
bool PointInWall(Vector3 point, Wall& wall, float padding);

// Resolve collision between player and wall, returns adjusted position
Vector3 ResolveWallCollision(Vector3 pos, Wall& wall, float padding);

#endif
