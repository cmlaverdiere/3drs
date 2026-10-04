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
    Texture2D groundMap;   // r = paved road, g = dirt path; id 0 when the map has none
    bool initialized;
};

// Builds the chunk meshes; shader receives sand/water zone uniforms.
void InitTerrain(TerrainSystem* terrain, Shader shader, Shader depthShader,
                 const Sand* sandZones, int sandCount, const Water* waterBodies, int waterCount);
// Rebuilds geometry after the heightmap changes (map reload)
void RebuildTerrain(TerrainSystem* terrain);
// Uploads sand zones used by the terrain shader (water needs none: it lies in carved channels)
void SetTerrainZoneUniforms(Shader shader, const Sand* sandZones, int sandCount,
                            const Water* waterBodies, int waterCount);
// Loads the road/path ground map covering world rectangle (x0, z0, sizeX, sizeZ); also fills
// the CPU copy used for grass placement. Call before the grass field is built.
void LoadTerrainGroundMap(TerrainSystem* terrain, const char* path, float x0, float z0, float sizeX, float sizeZ);

void DrawTerrain(const TerrainSystem* terrain, const Frustum* frustum, Vector3 cameraPos);
// Shadow pass: caller supplies the per-cascade visibility test
void DrawTerrainShadow(const TerrainSystem* terrain, bool (*visible)(Vector3 center, float radius, void* user), void* user);

void UnloadTerrain(TerrainSystem* terrain);

#endif
