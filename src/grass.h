#ifndef GRASS_H
#define GRASS_H

// Dense instanced grass: 12 m chunks of blades baked into static GPU buffers
// around the player. Blade density and colour follow the terrain shader's
// ground cover (ground_noise.h), blades are sorted by a LOD key so distant
// chunks draw a prefix, and the vertex shader bends them with gusting wind and
// away from the player.

#include "raylib.h"
#include "types.h"
#include "frustum.h"
#include <vector>

struct GrassFieldChunk {
    int cx, cz;
    unsigned int vbo;
    int count;
    float minY, maxY;
    int lastUsed;
};

struct GrassField {
    Mesh bladeMesh;
    Shader shader;
    std::vector<GrassFieldChunk> chunks;
    const Sand* sand;
    int sandCount;
    const Water* water;
    int waterCount;
    const Wall* walls;
    int wallCount;
    int season;
    int frame;
    bool initialized;
};

void InitGrassField(GrassField* grass, const Sand* sand, int sandCount, const Water* water, int waterCount,
                    const Wall* walls, int wallCount);
// Drops baked chunks (heightmap reload); season changes are detected automatically
void ResetGrassField(GrassField* grass);
void DrawGrassField(GrassField* grass, const Frustum* frustum, Vector3 cameraPos);
void UnloadGrassField(GrassField* grass);

#endif
