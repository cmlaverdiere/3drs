#ifndef VEGETATION_H
#define VEGETATION_H

// Instanced trees and rocks. Canopies are clusters of camera-facing leaf cards
// around a solid core ("fluffy" stylised foliage), winter swaps in snowy pines,
// rocks are noise-displaced boulders with ore veins. Instance lists are rebuilt
// every frame from the live tree/rock arrays, so chopping and respawns show up
// immediately.

#include "raylib.h"
#include "types.h"
#include "gfx.h"
#include "frustum.h"
#include <vector>

struct LightingSystem;

constexpr int ROCK_MESH_VARIANTS = 4;

struct VegetationSystem {
    Mesh cardMesh;       // leaf cards on a unit sphere shell
    Mesh coreMesh;       // solid canopy core
    Mesh trunkMesh;      // tapered, flared unit trunk
    Mesh pineMesh;       // tiered evergreen, unit height
    Mesh rockMeshes[ROCK_MESH_VARIANTS];

    Shader cardShader, coreShader, trunkShader, pineShader, rockShader;
    Shader cardDepthShader, solidDepthShader, trunkDepthShader, pineDepthShader, rockDepthShader;

    // Per-frame instance data (3 vec4 per instance)
    std::vector<float> blobs, trunks, pines, rocks[ROCK_MESH_VARIANTS];
    InstanceStream blobStream, coreStream, trunkStream, pineStream, rockStreams[ROCK_MESH_VARIANTS];
    InstanceStream shadowStreams[4][5 + ROCK_MESH_VARIANTS];

    bool initialized;
};

void InitVegetation(VegetationSystem* veg);
void UnloadVegetation(VegetationSystem* veg);

// Scene pass: frustum/distance culled trees and rocks
void DrawVegetation(VegetationSystem* veg, const Tree* trees, int treeCount, const Rock* rocks, int rockCount,
                    const Frustum* frustum, Vector3 cameraPos);
// Shadow pass: casters overlapping one cascade
void DrawVegetationShadows(VegetationSystem* veg, const LightingSystem* lighting, int cascade,
                           const Tree* trees, int treeCount, const Rock* rocks, int rockCount);

#endif
