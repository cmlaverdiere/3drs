#ifndef TERRAIN_H
#define TERRAIN_H

// Chunked terrain mesh built from GetTerrainHeight(): 1-unit spacing near the
// playable world, 4-unit spacing in the outer ring, skirts hide LOD cracks.

#include "raylib.h"
#include "types.h"
#include "frustum.h"
#include <vector>

struct TerrainChunk {
    Mesh mesh;
    Vector3 center;
    float radius;
};

struct TerrainSystem {
    std::vector<TerrainChunk> chunks;
    Material material;
    Material depthMaterial;
    Shader shader;
    bool initialized;
};

// Builds the chunk meshes; shader receives sand/water zone uniforms.
void InitTerrain(TerrainSystem* terrain, Shader shader, Shader depthShader,
                 const Sand* sandZones, int sandCount, const Water* waterBodies, int waterCount);
// Rebuilds geometry after the heightmap changes (map reload)
void RebuildTerrain(TerrainSystem* terrain);
// Uploads sand/water zones used by terrain-coloured shaders (terrain, grass)
void SetTerrainZoneUniforms(Shader shader, const Sand* sandZones, int sandCount,
                            const Water* waterBodies, int waterCount);

void DrawTerrain(const TerrainSystem* terrain, const Frustum* frustum, Vector3 cameraPos);
// Shadow pass: caller supplies the per-cascade visibility test
void DrawTerrainShadow(const TerrainSystem* terrain, bool (*visible)(Vector3 center, float radius, void* user), void* user);

void UnloadTerrain(TerrainSystem* terrain);

#endif
