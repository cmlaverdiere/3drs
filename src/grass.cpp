#include "grass.h"
#include "ground_noise.h"
#include "lighting.h"
#include "math_utils.h"
#include "shader_utils.h"
#include "gfx.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

static const float CHUNK = 12.0f;
static const float RADIUS = 50.0f;           // must match grass.vs
static const float BLADES_PER_M2 = 58.0f;
static const int MAX_BUILDS_PER_FRAME = 6;
static const size_t CACHE_LIMIT = 220;

// Kept fraction of blades at a distance (grass.vs uses the same curve)
static float KeepFraction(float dist) {
    return Clamp(1.0f - (dist - 12.0f) / 42.0f, 0.06f, 1.0f);
}

struct Rng {
    uint32_t s;
    float Next() {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (s & 0xFFFFFF) / 16777216.0f;
    }
};

static Mesh GenBladeMesh() {
    Mesh m = {};
    m.vertexCount = 7;
    m.triangleCount = 5;
    m.vertices = (float*)RL_CALLOC(7 * 3, sizeof(float));
    m.texcoords = (float*)RL_CALLOC(7 * 2, sizeof(float));
    m.normals = (float*)RL_CALLOC(7 * 3, sizeof(float));
    m.indices = (unsigned short*)RL_CALLOC(15, sizeof(unsigned short));
    const float levels[3] = {0.0f, 0.36f, 0.7f};
    for (int l = 0; l < 3; l++) {
        for (int side = 0; side < 2; side++) {
            int i = l * 2 + side;
            m.vertices[i * 3 + 0] = side ? 0.5f : -0.5f;
            m.vertices[i * 3 + 1] = levels[l];
            m.texcoords[i * 2 + 0] = (float)side;
            m.texcoords[i * 2 + 1] = levels[l];
            m.normals[i * 3 + 2] = 1.0f;
        }
    }
    m.vertices[6 * 3 + 1] = 1.0f;
    m.texcoords[6 * 2 + 0] = 0.5f;
    m.texcoords[6 * 2 + 1] = 1.0f;
    m.normals[6 * 3 + 2] = 1.0f;
    const unsigned short idx[15] = {0, 1, 2, 2, 1, 3, 2, 3, 4, 4, 3, 5, 4, 5, 6};
    for (int i = 0; i < 15; i++) m.indices[i] = idx[i];
    UploadMesh(&m, false);
    return m;
}

void InitGrassField(GrassField* grass, const Sand* sand, int sandCount, const Water* water, int waterCount,
                    const Wall* walls, int wallCount) {
    grass->bladeMesh = GenBladeMesh();
    grass->shader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/grass.vs", "shaders/grass.fs"));
    grass->sand = sand;
    grass->sandCount = sandCount;
    grass->water = water;
    grass->waterCount = waterCount;
    grass->walls = walls;
    grass->wallCount = wallCount;
    grass->season = (int)g_currentSeason;
    grass->frame = 0;
    grass->initialized = true;
}

void ResetGrassField(GrassField* grass) {
    for (GrassFieldChunk& c : grass->chunks) DeleteInstanceBuffer(c.vbo);
    grass->chunks.clear();
}

static void BuildChunk(GrassField* grass, int cx, int cz, GrassFieldChunk* out) {
    float x0 = cx * CHUNK, z0 = cz * CHUNK;
    Rng rng = {(uint32_t)(cx * 73856093) ^ (uint32_t)(cz * 19349663) ^ 0x9E3779B9u};
    if (!rng.s) rng.s = 1;

    // Walls whose footprint overlaps this chunk (decks, floors, buildings)
    std::vector<int> walls;
    for (int i = 0; i < grass->wallCount; i++) {
        const Wall& w = grass->walls[i];
        if (w.position.x + w.width * 0.5f < x0 || w.position.x - w.width * 0.5f > x0 + CHUNK) continue;
        if (w.position.z + w.depth * 0.5f < z0 || w.position.z - w.depth * 0.5f > z0 + CHUNK) continue;
        walls.push_back(i);
    }

    int candidates = (int)(CHUNK * CHUNK * BLADES_PER_M2);
    std::vector<float> data;
    data.reserve(candidates * 12);
    int season = grass->season;
    float minY = 1e9f, maxY = -1e9f;
    for (int i = 0; i < candidates; i++) {
        float x = x0 + rng.Next() * CHUNK, z = z0 + rng.Next() * CHUNK;
        float keepRoll = rng.Next();
        bool blocked = false;
        for (int w : walls) {
            const Wall& wall = grass->walls[w];
            if (fabsf(x - wall.position.x) < wall.width * 0.5f + 0.05f &&
                fabsf(z - wall.position.z) < wall.depth * 0.5f + 0.05f) { blocked = true; break; }
        }
        if (blocked) continue;
        float y = GetTerrainHeight(x, z);
        float hx = GetTerrainHeight(x + 0.5f, z) - GetTerrainHeight(x - 0.5f, z);
        float hz = GetTerrainHeight(x, z + 0.5f) - GetTerrainHeight(x, z - 0.5f);
        float slope = 1.0f - 1.0f / sqrtf(1.0f + hx * hx + hz * hz);
        ground::GrassCover cover = ground::SampleGrass(x, z, slope, season, grass->sand, grass->sandCount,
                                                       grass->water, grass->waterCount);
        if (keepRoll > cover.density) continue;
        float height = (0.2f + 0.36f * cover.tallness) * (0.6f + 0.75f * rng.Next()) * (0.65f + 0.35f * cover.density);
        float width = 0.03f + 0.028f * rng.Next();
        float lod = rng.Next();
        float curve = rng.Next() * 0.35f;
        float tone = 0.82f + 0.36f * rng.Next();
        float r = cover.r * tone * (0.94f + 0.12f * rng.Next()), g = cover.g * tone, b = cover.b * tone;
        float flower = 0.0f;
        float flowerChance = season == 0 ? 0.06f : (season == 1 ? 0.012f : 0.0f);
        if (rng.Next() < flowerChance * cover.density) {
            flower = 1.0f + floorf(rng.Next() * 3.0f);
            height = fmaxf(height, 0.3f) * 1.15f;
        }
        float rot = rng.Next() * 6.2831853f;
        float v[12] = {x, y, z, rot, height, width, lod, curve, r, g, b, flower};
        data.insert(data.end(), v, v + 12);
        minY = fminf(minY, y);
        maxY = fmaxf(maxY, y + height);
    }
    // Sort by LOD key so any prefix is an evenly thinned subset
    int count = (int)(data.size() / 12);
    std::vector<int> order(count);
    for (int i = 0; i < count; i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return data[a * 12 + 6] < data[b * 12 + 6]; });
    std::vector<float> sorted(data.size());
    for (int i = 0; i < count; i++) std::copy(&data[order[i] * 12], &data[order[i] * 12] + 12, &sorted[i * 12]);

    out->cx = cx;
    out->cz = cz;
    out->count = count;
    out->vbo = count ? CreateStaticInstanceBuffer(sorted.data(), sorted.size() * sizeof(float)) : 0;
    out->minY = count ? minY : GetTerrainHeight(x0 + CHUNK * 0.5f, z0 + CHUNK * 0.5f);
    out->maxY = count ? maxY : out->minY + 0.5f;
    out->lastUsed = grass->frame;
}

void DrawGrassField(GrassField* grass, const Frustum* frustum, Vector3 cameraPos) {
    if (!grass->initialized) return;
    grass->frame++;
    if (grass->season != (int)g_currentSeason) {
        ResetGrassField(grass);
        grass->season = (int)g_currentSeason;
    }
    int ccx = (int)floorf(cameraPos.x / CHUNK), ccz = (int)floorf(cameraPos.z / CHUNK);
    int range = (int)ceilf(RADIUS / CHUNK) + 1;
    const float halfDiag = CHUNK * 0.7072f;
    int builds = grass->chunks.empty() ? 100000 : MAX_BUILDS_PER_FRAME;

    for (int dz = -range; dz <= range; dz++) {
        for (int dx = -range; dx <= range; dx++) {
            int cx = ccx + dx, cz = ccz + dz;
            float centerX = (cx + 0.5f) * CHUNK, centerZ = (cz + 0.5f) * CHUNK;
            float dist = sqrtf((centerX - cameraPos.x) * (centerX - cameraPos.x) +
                               (centerZ - cameraPos.z) * (centerZ - cameraPos.z));
            float nearest = fmaxf(0.0f, dist - halfDiag);
            if (nearest > RADIUS) continue;
            GrassFieldChunk* chunk = nullptr;
            for (GrassFieldChunk& c : grass->chunks) {
                if (c.cx == cx && c.cz == cz) { chunk = &c; break; }
            }
            float groundY = chunk ? (chunk->minY + chunk->maxY) * 0.5f : GetTerrainHeight(centerX, centerZ);
            if (!SphereInFrustum(frustum, {centerX, groundY, centerZ}, halfDiag + 2.0f)) continue;
            if (!chunk) {
                if (builds <= 0) continue;
                builds--;
                GrassFieldChunk built = {};
                BuildChunk(grass, cx, cz, &built);
                grass->chunks.push_back(built);
                chunk = &grass->chunks.back();
            }
            chunk->lastUsed = grass->frame;
            int drawCount = (int)ceilf(chunk->count * KeepFraction(nearest));
            DrawMeshInstancedBuffer(grass->bladeMesh, grass->shader, chunk->vbo, std::min(drawCount, chunk->count), 3, true);
        }
    }

    // Evict chunks that have not been drawn for a while
    if (grass->chunks.size() > CACHE_LIMIT) {
        for (size_t i = 0; i < grass->chunks.size();) {
            if (grass->frame - grass->chunks[i].lastUsed > 240) {
                DeleteInstanceBuffer(grass->chunks[i].vbo);
                grass->chunks[i] = grass->chunks.back();
                grass->chunks.pop_back();
            } else {
                i++;
            }
        }
    }
}

void UnloadGrassField(GrassField* grass) {
    if (!grass->initialized) return;
    ResetGrassField(grass);
    UnloadMesh(grass->bladeMesh);
    UnloadShader(grass->shader);
    grass->initialized = false;
}
