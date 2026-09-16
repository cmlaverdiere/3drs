#include "terrain.h"
#include "math_utils.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstring>

static const float CHUNK_SIZE = 64.0f;
static const float INNER_EXTENT = 832.0f;   // 1-unit detail inside this half-size
static const float OUTER_EXTENT = 1216.0f;  // coarse ring out to the far plane
static const float SKIRT_DEPTH = 3.0f;

static Vector3 TerrainNormal(float x, float z, float spacing) {
    float e = spacing;
    float hl = GetTerrainHeight(x - e, z), hr = GetTerrainHeight(x + e, z);
    float hd = GetTerrainHeight(x, z - e), hu = GetTerrainHeight(x, z + e);
    return Vector3Normalize({hl - hr, 2.0f * e, hd - hu});
}

// Grid of (cells+1)^2 vertices plus a skirt ring around the border.
static Mesh BuildChunkMesh(float x0, float z0, float spacing, float* minY, float* maxY) {
    int cells = (int)(CHUNK_SIZE / spacing);
    int side = cells + 1;
    int gridVerts = side * side;
    int skirtVerts = side * 4;
    Mesh mesh = {};
    mesh.vertexCount = gridVerts + skirtVerts;
    mesh.triangleCount = cells * cells * 2 + cells * 4 * 2;
    mesh.vertices = (float*)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.normals = (float*)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
    mesh.indices = (unsigned short*)RL_MALLOC(mesh.triangleCount * 3 * sizeof(unsigned short));

    *minY = 1e9f;
    *maxY = -1e9f;
    auto put = [&](int i, float x, float y, float z, Vector3 n) {
        mesh.vertices[i * 3 + 0] = x;
        mesh.vertices[i * 3 + 1] = y;
        mesh.vertices[i * 3 + 2] = z;
        mesh.normals[i * 3 + 0] = n.x;
        mesh.normals[i * 3 + 1] = n.y;
        mesh.normals[i * 3 + 2] = n.z;
        mesh.texcoords[i * 2 + 0] = x;
        mesh.texcoords[i * 2 + 1] = z;
    };
    for (int j = 0; j < side; j++) {
        for (int i = 0; i < side; i++) {
            float x = x0 + i * spacing, z = z0 + j * spacing;
            float y = GetTerrainHeight(x, z);
            *minY = fminf(*minY, y);
            *maxY = fmaxf(*maxY, y);
            put(j * side + i, x, y, z, TerrainNormal(x, z, fminf(spacing, 1.0f)));
        }
    }
    // Skirt: border vertices duplicated and dropped (edges: south, east, north, west)
    int borderIndex[4][80];
    for (int e = 0; e < 4; e++) {
        for (int k = 0; k < side; k++) {
            int i, j;
            switch (e) {
                case 0: i = k; j = 0; break;
                case 1: i = cells; j = k; break;
                case 2: i = cells - k; j = cells; break;
                default: i = 0; j = cells - k; break;
            }
            int src = j * side + i;
            int dst = gridVerts + e * side + k;
            borderIndex[e][k] = src;
            put(dst, mesh.vertices[src * 3], mesh.vertices[src * 3 + 1] - SKIRT_DEPTH, mesh.vertices[src * 3 + 2],
                {mesh.normals[src * 3], mesh.normals[src * 3 + 1], mesh.normals[src * 3 + 2]});
        }
    }
    *minY -= SKIRT_DEPTH;

    int t = 0;
    for (int j = 0; j < cells; j++) {
        for (int i = 0; i < cells; i++) {
            unsigned short a = (unsigned short)(j * side + i), b = (unsigned short)(a + 1);
            unsigned short c = (unsigned short)((j + 1) * side + i), d = (unsigned short)(c + 1);
            // Alternate the diagonal to reduce directional faceting
            if (((i + j) & 1) == 0) {
                mesh.indices[t++] = a; mesh.indices[t++] = c; mesh.indices[t++] = b;
                mesh.indices[t++] = b; mesh.indices[t++] = c; mesh.indices[t++] = d;
            } else {
                mesh.indices[t++] = a; mesh.indices[t++] = c; mesh.indices[t++] = d;
                mesh.indices[t++] = a; mesh.indices[t++] = d; mesh.indices[t++] = b;
            }
        }
    }
    for (int e = 0; e < 4; e++) {
        for (int k = 0; k < cells; k++) {
            unsigned short top0 = (unsigned short)borderIndex[e][k], top1 = (unsigned short)borderIndex[e][k + 1];
            unsigned short bot0 = (unsigned short)(gridVerts + e * side + k), bot1 = (unsigned short)(bot0 + 1);
            // Outward-facing skirt quads
            mesh.indices[t++] = top0; mesh.indices[t++] = top1; mesh.indices[t++] = bot0;
            mesh.indices[t++] = top1; mesh.indices[t++] = bot1; mesh.indices[t++] = bot0;
        }
    }
    UploadMesh(&mesh, false);
    return mesh;
}

static void FreeChunks(TerrainSystem* terrain) {
    for (TerrainChunk& chunk : terrain->chunks) UnloadMesh(chunk.mesh);
    terrain->chunks.clear();
}

void RebuildTerrain(TerrainSystem* terrain) {
    FreeChunks(terrain);
    for (float z0 = -OUTER_EXTENT; z0 < OUTER_EXTENT; z0 += CHUNK_SIZE) {
        for (float x0 = -OUTER_EXTENT; x0 < OUTER_EXTENT; x0 += CHUNK_SIZE) {
            bool inner = x0 >= -INNER_EXTENT && x0 + CHUNK_SIZE <= INNER_EXTENT &&
                         z0 >= -INNER_EXTENT && z0 + CHUNK_SIZE <= INNER_EXTENT;
            float minY, maxY;
            TerrainChunk chunk;
            chunk.mesh = BuildChunkMesh(x0, z0, inner ? 1.0f : 4.0f, &minY, &maxY);
            float half = CHUNK_SIZE * 0.5f;
            chunk.center = {x0 + half, (minY + maxY) * 0.5f, z0 + half};
            float halfY = (maxY - minY) * 0.5f;
            chunk.radius = sqrtf(half * half * 2.0f + halfY * halfY);
            terrain->chunks.push_back(chunk);
        }
    }
    TraceLog(LOG_INFO, "TERRAIN: Built %d chunks", (int)terrain->chunks.size());
}

void SetTerrainZoneUniforms(Shader shader, const Sand* sandZones, int sandCount,
                            const Water* waterBodies, int waterCount) {
    float sand[16 * 4] = {}, water[16 * 4] = {};
    int ns = sandCount < 16 ? sandCount : 16, nw = waterCount < 16 ? waterCount : 16;
    for (int i = 0; i < ns; i++) {
        sand[i * 4 + 0] = sandZones[i].position.x;
        sand[i * 4 + 1] = sandZones[i].position.z;
        sand[i * 4 + 2] = sandZones[i].width;
        sand[i * 4 + 3] = sandZones[i].length;
    }
    for (int i = 0; i < nw; i++) {
        water[i * 4 + 0] = waterBodies[i].position.x;
        water[i * 4 + 1] = waterBodies[i].position.z;
        water[i * 4 + 2] = waterBodies[i].width;
        water[i * 4 + 3] = waterBodies[i].length;
    }
    SetShaderValue(shader, GetShaderLocation(shader, "uSandCount"), &ns, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "uWaterCount"), &nw, SHADER_UNIFORM_INT);
    if (ns) SetShaderValueV(shader, GetShaderLocation(shader, "uSandZones"), sand, SHADER_UNIFORM_VEC4, ns);
    if (nw) SetShaderValueV(shader, GetShaderLocation(shader, "uWaterZones"), water, SHADER_UNIFORM_VEC4, nw);
}

void InitTerrain(TerrainSystem* terrain, Shader shader, Shader depthShader,
                 const Sand* sandZones, int sandCount, const Water* waterBodies, int waterCount) {
    terrain->shader = shader;
    terrain->material = LoadMaterialDefault();
    terrain->material.shader = shader;
    terrain->depthMaterial = LoadMaterialDefault();
    terrain->depthMaterial.shader = depthShader;
    SetTerrainZoneUniforms(shader, sandZones, sandCount, waterBodies, waterCount);
    RebuildTerrain(terrain);
    terrain->initialized = true;
}

void DrawTerrain(const TerrainSystem* terrain, const Frustum* frustum, Vector3 cameraPos) {
    for (const TerrainChunk& chunk : terrain->chunks) {
        float dx = chunk.center.x - cameraPos.x, dz = chunk.center.z - cameraPos.z;
        if (sqrtf(dx * dx + dz * dz) - chunk.radius > 1000.0f) continue;
        if (!SphereInFrustum(frustum, chunk.center, chunk.radius)) continue;
        DrawMesh(chunk.mesh, terrain->material, MatrixIdentity());
    }
}

void DrawTerrainShadow(const TerrainSystem* terrain, bool (*visible)(Vector3, float, void*), void* user) {
    for (const TerrainChunk& chunk : terrain->chunks) {
        if (!visible(chunk.center, chunk.radius, user)) continue;
        DrawMesh(chunk.mesh, terrain->depthMaterial, MatrixIdentity());
    }
}

void UnloadTerrain(TerrainSystem* terrain) {
    FreeChunks(terrain);
    terrain->initialized = false;
}
