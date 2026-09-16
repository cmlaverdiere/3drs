#include "vegetation.h"
#include "lighting.h"
#include "math_utils.h"
#include "shader_utils.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstring>
#include <vector>

static const float TREE_DRAW_DISTANCE = 340.0f;
static const float CARD_DISTANCE = 150.0f;
static const float ROCK_DRAW_DISTANCE = 170.0f;

static float Hash1(float n) {
    float x = sinf(n) * 43758.5453f;
    return x - floorf(x);
}

static float Hash2(float x, float z) { return Hash1(x * 12.9898f + z * 78.233f); }

static Vector3 LinearColor(Color c) {
    return {powf(c.r / 255.0f, 2.2f), powf(c.g / 255.0f, 2.2f), powf(c.b / 255.0f, 2.2f)};
}

// ---------------------------------------------------------------------------
// Mesh generation
// ---------------------------------------------------------------------------
static Mesh AllocMesh(int vertexCount, int triangleCount, bool colors) {
    Mesh m = {};
    m.vertexCount = vertexCount;
    m.triangleCount = triangleCount;
    m.vertices = (float*)RL_CALLOC(vertexCount * 3, sizeof(float));
    m.normals = (float*)RL_CALLOC(vertexCount * 3, sizeof(float));
    m.texcoords = (float*)RL_CALLOC(vertexCount * 2, sizeof(float));
    if (colors) m.colors = (unsigned char*)RL_CALLOC(vertexCount * 4, 1);
    m.indices = (unsigned short*)RL_CALLOC(triangleCount * 3, sizeof(unsigned short));
    return m;
}

static void SetVertex(Mesh* m, int i, Vector3 p, Vector3 n, float u, float v) {
    m->vertices[i * 3 + 0] = p.x; m->vertices[i * 3 + 1] = p.y; m->vertices[i * 3 + 2] = p.z;
    m->normals[i * 3 + 0] = n.x; m->normals[i * 3 + 1] = n.y; m->normals[i * 3 + 2] = n.z;
    m->texcoords[i * 2 + 0] = u; m->texcoords[i * 2 + 1] = v;
}

// Leaf cards: quads centred on a Fibonacci sphere shell; the vertex shader
// billboards each card using the corner stored in the texcoords.
static Mesh GenLeafCardMesh(int cards) {
    Mesh m = AllocMesh(cards * 4, cards * 2, true);
    const float corners[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    for (int c = 0; c < cards; c++) {
        float y = 1.0f - (c + 0.5f) / cards * 2.0f;
        float r = sqrtf(fmaxf(0.0f, 1.0f - y * y));
        float phi = c * 2.39996323f;
        Vector3 dir = {cosf(phi) * r, y, sinf(phi) * r};
        float shell = 0.62f + 0.2f * Hash1(c * 3.7f);
        Vector3 center = Vector3Scale(dir, shell);
        for (int k = 0; k < 4; k++) {
            int i = c * 4 + k;
            SetVertex(&m, i, center, dir, corners[k][0], corners[k][1]);
            m.colors[i * 4 + 0] = (unsigned char)(Hash1(c * 1.3f) * 255);
            m.colors[i * 4 + 1] = (unsigned char)(Hash1(c * 7.1f + 2.0f) * 255);
            m.colors[i * 4 + 2] = (unsigned char)(Hash1(c * 5.9f + 4.0f) * 255);
            m.colors[i * 4 + 3] = (unsigned char)(Hash1(c * 2.3f + 9.0f) * 255);
        }
        unsigned short b = (unsigned short)(c * 4);
        unsigned short* idx = &m.indices[c * 6];
        idx[0] = b; idx[1] = b + 1; idx[2] = b + 2; idx[3] = b; idx[4] = b + 2; idx[5] = b + 3;
    }
    UploadMesh(&m, false);
    return m;
}

// Unit trunk: y in [0,1], unit radius; the shader tapers, flares and bends it.
static Mesh GenTrunkMesh(int sides, int rings) {
    int verts = (sides + 1) * (rings + 1);
    Mesh m = AllocMesh(verts, sides * rings * 2, false);
    for (int r = 0; r <= rings; r++) {
        float y = (float)r / rings;
        for (int s = 0; s <= sides; s++) {
            float a = (float)s / sides * 2.0f * PI;
            Vector3 n = {cosf(a), 0.0f, sinf(a)};
            SetVertex(&m, r * (sides + 1) + s, {n.x, y, n.z}, n, (float)s / sides, y);
        }
    }
    int t = 0;
    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < sides; s++) {
            unsigned short a = (unsigned short)(r * (sides + 1) + s), b = (unsigned short)(a + 1);
            unsigned short c = (unsigned short)(a + sides + 1), d = (unsigned short)(c + 1);
            m.indices[t++] = a; m.indices[t++] = c; m.indices[t++] = b;
            m.indices[t++] = b; m.indices[t++] = c; m.indices[t++] = d;
        }
    }
    UploadMesh(&m, false);
    return m;
}

// Tiered evergreen, unit radius at the lowest tier and unit height. Each tier
// is a drooping, jagged skirt with a closed underside. Vertex colour red holds
// baked occlusion (dark near the trunk and under each tier).
static Mesh GenPineMesh(int tiers, int segments) {
    int vertsPerTier = 1 + segments + 1 + segments;  // apex, outer rim, underside centre, underside rim
    int trisPerTier = segments * 2;
    Mesh m = AllocMesh(vertsPerTier * tiers, trisPerTier * tiers, true);
    int v = 0, t = 0;
    for (int k = 0; k < tiers; k++) {
        float f = (float)k / tiers;
        float bottom = f * 0.82f;
        float top = fminf(1.0f, bottom + 0.34f + 0.05f * (tiers - k) / tiers);
        float radius = 1.0f - f * 0.78f;
        int apex = v;
        SetVertex(&m, v, {0, top, 0}, {0, 1, 0}, 0, 0);
        m.colors[v * 4] = 255; v++;
        int rimStart = v;
        for (int s = 0; s < segments; s++) {
            float a = (float)s / segments * 2.0f * PI + k * 0.7f;
            float jag = (s % 2 == 0) ? 1.0f : 0.8f;
            jag *= 0.9f + 0.2f * Hash1(k * 31.0f + s * 7.0f);
            float r = radius * jag;
            float droop = -0.05f * jag;
            Vector3 p = {cosf(a) * r, bottom + droop, sinf(a) * r};
            Vector3 radial = {cosf(a), 0, sinf(a)};
            Vector3 n = Vector3Normalize(Vector3Add(Vector3Scale(radial, top - bottom), {0, r * 0.9f, 0}));
            SetVertex(&m, v, p, n, 1, 0);
            m.colors[v * 4] = (unsigned char)(170 + 85 * jag * 0.9f);
            v++;
        }
        int underCenter = v;
        SetVertex(&m, v, {0, bottom + 0.06f, 0}, {0, -1, 0}, 0, 1);
        m.colors[v * 4] = 40; v++;
        int underStart = v;
        for (int s = 0; s < segments; s++) {
            int src = rimStart + s;
            Vector3 p = {m.vertices[src * 3], m.vertices[src * 3 + 1], m.vertices[src * 3 + 2]};
            SetVertex(&m, v, p, {p.x * 0.3f, -1, p.z * 0.3f}, 1, 1);
            m.colors[v * 4] = 90;
            v++;
        }
        for (int s = 0; s < segments; s++) {
            int s1 = (s + 1) % segments;
            m.indices[t++] = (unsigned short)apex;
            m.indices[t++] = (unsigned short)(rimStart + s1);
            m.indices[t++] = (unsigned short)(rimStart + s);
            m.indices[t++] = (unsigned short)underCenter;
            m.indices[t++] = (unsigned short)(underStart + s);
            m.indices[t++] = (unsigned short)(underStart + s1);
        }
    }
    UploadMesh(&m, false);
    return m;
}

// Boulder: subdivided icosahedron displaced by value noise, flattened base.
static float RockNoise(Vector3 p, float seed) {
    float sum = 0.0f, amp = 0.5f, freq = 1.3f;
    for (int o = 0; o < 4; o++) {
        Vector3 q = Vector3Scale(p, freq);
        float n = sinf(q.x * 1.7f + seed) * sinf(q.y * 2.3f + seed * 1.3f) * sinf(q.z * 1.9f + seed * 0.7f);
        n += 0.5f * sinf(q.x * 3.1f + q.z * 2.2f + seed * 2.0f) * cosf(q.y * 2.7f - q.x * 1.1f);
        sum += n * amp;
        amp *= 0.5f;
        freq *= 2.1f;
    }
    return sum;
}

static Mesh GenRockMesh(float seed) {
    std::vector<Vector3> verts;
    std::vector<int> tris;
    const float t = (1.0f + sqrtf(5.0f)) / 2.0f;
    Vector3 base[12] = {{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t},
                        {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
    for (Vector3 v : base) verts.push_back(Vector3Normalize(v));
    int faces[20][3] = {{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4},
                        {11, 10, 2}, {10, 7, 6}, {7, 1, 8}, {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8},
                        {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};
    for (auto& f : faces) { tris.push_back(f[0]); tris.push_back(f[1]); tris.push_back(f[2]); }
    for (int level = 0; level < 3; level++) {
        std::vector<int> next;
        std::vector<std::pair<long long, int>> cache;
        auto midpoint = [&](int a, int b) {
            long long key = a < b ? ((long long)a << 32) | b : ((long long)b << 32) | a;
            for (auto& e : cache) if (e.first == key) return e.second;
            verts.push_back(Vector3Normalize(Vector3Scale(Vector3Add(verts[a], verts[b]), 0.5f)));
            int idx = (int)verts.size() - 1;
            cache.push_back({key, idx});
            return idx;
        };
        for (size_t i = 0; i < tris.size(); i += 3) {
            int a = tris[i], b = tris[i + 1], c = tris[i + 2];
            int ab = midpoint(a, b), bc = midpoint(b, c), ca = midpoint(c, a);
            int nt[12] = {a, ab, ca, b, bc, ab, c, ca, bc, ab, bc, ca};
            next.insert(next.end(), nt, nt + 12);
        }
        tris.swap(next);
    }
    // Displace: lumpy boulder, a few flat facets, squashed and seated on its base
    for (Vector3& v : verts) {
        float d = 1.0f + 0.28f * RockNoise(v, seed);
        Vector3 facet = Vector3Normalize({Hash1(seed * 3.1f) - 0.5f, 0.6f, Hash1(seed * 5.3f) - 0.5f});
        float cut = Vector3DotProduct(v, facet);
        if (cut > 0.72f) d -= (cut - 0.72f) * 0.8f;
        v = Vector3Scale(v, d);
        v.y *= 0.72f;
        if (v.y < -0.22f) v.y = -0.22f + (v.y + 0.22f) * 0.25f;
        v.x *= 1.0f + 0.2f * Hash1(seed * 7.7f);
    }
    std::vector<Vector3> normals(verts.size(), {0, 0, 0});
    for (size_t i = 0; i < tris.size(); i += 3) {
        Vector3 a = verts[tris[i]], b = verts[tris[i + 1]], c = verts[tris[i + 2]];
        Vector3 n = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
        for (int k = 0; k < 3; k++) normals[tris[i + k]] = Vector3Add(normals[tris[i + k]], n);
    }
    // Make winding counter-clockwise from outside
    Mesh m = AllocMesh((int)verts.size(), (int)tris.size() / 3, false);
    for (size_t i = 0; i < verts.size(); i++) {
        Vector3 n = Vector3Normalize(normals[i]);
        if (Vector3DotProduct(n, verts[i]) < 0.0f) n = Vector3Negate(n);
        SetVertex(&m, (int)i, Vector3Add(verts[i], {0, 0.22f, 0}), n, 0, 0);
    }
    for (size_t i = 0; i < tris.size(); i += 3) {
        Vector3 a = verts[tris[i]], b = verts[tris[i + 1]], c = verts[tris[i + 2]];
        Vector3 n = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
        bool outward = Vector3DotProduct(n, Vector3Add(Vector3Add(a, b), c)) > 0.0f;
        m.indices[i + 0] = (unsigned short)tris[i];
        m.indices[i + 1] = (unsigned short)(outward ? tris[i + 1] : tris[i + 2]);
        m.indices[i + 2] = (unsigned short)(outward ? tris[i + 2] : tris[i + 1]);
    }
    UploadMesh(&m, false);
    return m;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
static Shader LoadVegShader(const char* defines) {
    return RegisterSceneShader(nullptr, LoadShaderVariant("shaders/vegetation.vs", "shaders/vegetation.fs", defines));
}

void InitVegetation(VegetationSystem* veg) {
    veg->cardMesh = GenLeafCardMesh(88);
    veg->coreMesh = GenMeshSphere(1.0f, 10, 14);
    veg->trunkMesh = GenTrunkMesh(12, 6);
    veg->pineMesh = GenPineMesh(6, 22);
    for (int i = 0; i < ROCK_MESH_VARIANTS; i++) veg->rockMeshes[i] = GenRockMesh(1.7f + i * 2.9f);

    veg->cardShader = LoadVegShader("CARDS");
    veg->coreShader = LoadVegShader("CORE");
    veg->trunkShader = LoadVegShader("TRUNK");
    veg->pineShader = LoadVegShader("PINE");
    veg->rockShader = LoadVegShader("ROCK");
    veg->cardDepthShader = LoadVegShader("CARDS;SHADOW");
    veg->solidDepthShader = LoadVegShader("CORE;SHADOW");
    veg->trunkDepthShader = LoadVegShader("TRUNK;SHADOW");
    veg->pineDepthShader = LoadVegShader("PINE;SHADOW");
    veg->rockDepthShader = LoadVegShader("ROCK;SHADOW");
    veg->initialized = true;
}

void UnloadVegetation(VegetationSystem* veg) {
    if (!veg->initialized) return;
    UnloadMesh(veg->cardMesh);
    UnloadMesh(veg->coreMesh);
    UnloadMesh(veg->trunkMesh);
    UnloadMesh(veg->pineMesh);
    for (int i = 0; i < ROCK_MESH_VARIANTS; i++) UnloadMesh(veg->rockMeshes[i]);
    for (Shader s : {veg->cardShader, veg->coreShader, veg->trunkShader, veg->pineShader, veg->rockShader,
                     veg->cardDepthShader, veg->solidDepthShader, veg->trunkDepthShader, veg->pineDepthShader,
                     veg->rockDepthShader}) {
        UnloadShader(s);
    }
    InstanceStream* streams[] = {&veg->blobStream, &veg->coreStream, &veg->trunkStream, &veg->pineStream};
    for (InstanceStream* s : streams) UnloadInstanceStream(s);
    for (int i = 0; i < ROCK_MESH_VARIANTS; i++) UnloadInstanceStream(&veg->rockStreams[i]);
    for (auto& cascade : veg->shadowStreams) for (InstanceStream& s : cascade) UnloadInstanceStream(&s);
    veg->initialized = false;
}

// ---------------------------------------------------------------------------
// Instance building
// ---------------------------------------------------------------------------
static void Push(std::vector<float>& out, Vector3 a, float aw, Vector3 b, float bw, Vector3 c, float cw) {
    float v[12] = {a.x, a.y, a.z, aw, b.x, b.y, b.z, bw, c.x, c.y, c.z, cw};
    out.insert(out.end(), v, v + 12);
}

struct CanopyBlob { float x, y, z, r; bool dark; };

// Canopy layout and colours match the original primitive trees
static void BuildTree(VegetationSystem* veg, const Tree& tree, bool cards, bool coresOnly) {
    Vector3 pos = tree.position;
    pos.y = GetTerrainHeight(pos.x, pos.z);
    float seed = Hash2(pos.x, pos.z);

    if (IsWinterMode()) {
        float scale = (tree.type == TREE_OAK) ? 1.3f : 1.0f;
        scale *= 0.9f + 0.25f * seed;
        Vector3 bark = LinearColor({60, 40, 25, 255});
        Push(veg->trunks, pos, 1.6f * scale, {0.2f * scale, 0.12f * scale, seed}, 0, bark, 0);
        Vector3 needles = Vector3Scale(LinearColor({30, 72, 45, 255}), 0.8f + 0.3f * seed);
        Push(veg->pines, {pos.x, pos.y + 0.75f * scale, pos.z}, 3.9f * scale, {1.9f * scale, seed, 0}, 0, needles, 0);
        return;
    }

    Color leaves, leavesDark;
    if (g_currentSeason == SEASON_SPRING) {
        leaves = tree.type == TREE_OAK ? Color{70, 165, 55, 255} : Color{85, 185, 65, 255};
        leavesDark = tree.type == TREE_OAK ? Color{50, 135, 40, 255} : Color{60, 155, 50, 255};
    } else if (g_currentSeason == SEASON_AUTUMN) {
        float treeHash = fmodf(fabsf(pos.x * 12.9898f + pos.z * 78.233f), 1.0f);
        if (treeHash > 0.7f) { leaves = {190, 52, 30, 255}; leavesDark = {150, 35, 22, 255}; }
        else if (treeHash > 0.4f) { leaves = {220, 125, 40, 255}; leavesDark = {185, 92, 30, 255}; }
        else { leaves = {215, 175, 55, 255}; leavesDark = {178, 140, 40, 255}; }
    } else {
        leaves = tree.type == TREE_OAK ? Color{52, 125, 40, 255} : Color{62, 145, 48, 255};
        leavesDark = tree.type == TREE_OAK ? Color{38, 100, 30, 255} : Color{45, 118, 36, 255};
    }
    Vector3 light = LinearColor(leaves), dark = LinearColor(leavesDark);
    // Per-tree hue variation keeps a forest from looking cloned
    Vector3 jitter = {0.9f + 0.2f * Hash1(seed * 13.0f), 0.9f + 0.2f * Hash1(seed * 17.0f), 0.85f + 0.3f * Hash1(seed * 19.0f)};
    light = {light.x * jitter.x, light.y * jitter.y, light.z * jitter.z};
    dark = {dark.x * jitter.x, dark.y * jitter.y, dark.z * jitter.z};

    const CanopyBlob normal[] = {{0, 3.5f, 0, 1.5f, false}, {-0.5f, 3.0f, 0.5f, 1.0f, true},
                                 {0.5f, 3.0f, -0.5f, 1.0f, true}, {0, 4.2f, 0, 0.8f, false}};
    const CanopyBlob oak[] = {{0, 5.0f, 0, 2.2f, false}, {-1.0f, 4.2f, 0.8f, 1.6f, true},
                              {1.0f, 4.2f, -0.8f, 1.6f, true}, {0.5f, 4.5f, 1.0f, 1.3f, false},
                              {-0.5f, 4.5f, -1.0f, 1.3f, false}, {0, 6.0f, 0, 1.2f, false}};
    bool isOak = tree.type == TREE_OAK;
    const CanopyBlob* blobs = isOak ? oak : normal;
    int count = isOak ? 6 : 4;
    float height = isOak ? 3.9f : 2.9f;
    float rb = isOak ? 0.55f : 0.34f, rt = isOak ? 0.38f : 0.22f;
    Vector3 bark = LinearColor(isOak ? Color{80, 56, 34, 255} : Color{105, 76, 50, 255});
    Push(veg->trunks, pos, height, {rb, rt, seed}, 0, bark, 0);
    float rot = seed * 6.283f;
    float c = cosf(rot), s = sinf(rot);
    for (int i = 0; i < count; i++) {
        const CanopyBlob& b = blobs[i];
        Vector3 center = {pos.x + b.x * c - b.z * s, pos.y + b.y, pos.z + b.x * s + b.z * c};
        float bseed = Hash1(seed * 31.0f + i * 7.0f);
        // Slightly larger than the old spheres: cards give a softer, broken silhouette
        float r = b.r * 1.08f;
        Push(veg->blobs, center, r, b.dark ? dark : light, bseed, {pos.x, pos.y, pos.z}, (cards && !coresOnly) ? 0.8f : 1.0f);
    }
}

static void BuildRock(VegetationSystem* veg, const Rock& rock) {
    Vector3 pos = rock.position;
    pos.y = GetTerrainHeight(pos.x, pos.z);
    float seed = Hash2(pos.x * 1.7f, pos.z * 2.3f);
    int variant = (int)(seed * ROCK_MESH_VARIANTS) % ROCK_MESH_VARIANTS;
    Vector3 ore = rock.type == ROCK_COPPER ? LinearColor({196, 104, 52, 255}) : LinearColor({176, 180, 176, 255});
    Push(veg->rocks[variant], pos, 0.78f + 0.12f * seed, ore, seed, {seed * 6.283f, (float)rock.type, 0}, 0);
}

static void ClearLists(VegetationSystem* veg) {
    veg->blobs.clear();
    veg->trunks.clear();
    veg->pines.clear();
    for (auto& r : veg->rocks) r.clear();
}

static int Count(const std::vector<float>& v) { return (int)(v.size() / 12); }

// Card-bearing blobs have w != 1 in instC; partition them to the front.
static int PartitionCardBlobs(std::vector<float>& blobs) {
    int n = Count(blobs), front = 0;
    for (int i = 0; i < n; i++) {
        if (blobs[i * 12 + 11] < 0.99f) {
            if (i != front) {
                float tmp[12];
                memcpy(tmp, &blobs[front * 12], sizeof(tmp));
                memcpy(&blobs[front * 12], &blobs[i * 12], sizeof(tmp));
                memcpy(&blobs[i * 12], tmp, sizeof(tmp));
            }
            front++;
        }
    }
    return front;
}

void DrawVegetation(VegetationSystem* veg, const Tree* trees, int treeCount, const Rock* rocks, int rockCount,
                    const Frustum* frustum, Vector3 cameraPos) {
    ClearLists(veg);
    for (int i = 0; i < treeCount; i++) {
        if (!trees[i].alive) continue;
        float dx = trees[i].position.x - cameraPos.x, dz = trees[i].position.z - cameraPos.z;
        float dist2 = dx * dx + dz * dz;
        if (dist2 > TREE_DRAW_DISTANCE * TREE_DRAW_DISTANCE) continue;
        Vector3 p = trees[i].position;
        p.y = GetTerrainHeight(p.x, p.z) + 4.0f;
        if (!SphereInFrustum(frustum, p, trees[i].type == TREE_OAK ? 6.5f : 5.0f)) continue;
        BuildTree(veg, trees[i], dist2 < CARD_DISTANCE * CARD_DISTANCE, false);
    }
    for (int i = 0; i < rockCount; i++) {
        if (!rocks[i].alive) continue;
        float dx = rocks[i].position.x - cameraPos.x, dz = rocks[i].position.z - cameraPos.z;
        if (dx * dx + dz * dz > ROCK_DRAW_DISTANCE * ROCK_DRAW_DISTANCE) continue;
        Vector3 p = rocks[i].position;
        p.y = GetTerrainHeight(p.x, p.z) + 0.4f;
        if (!SphereInFrustum(frustum, p, 1.4f)) continue;
        BuildRock(veg, rocks[i]);
    }
    int cardBlobs = PartitionCardBlobs(veg->blobs);
    DrawMeshInstancedData(veg->trunkMesh, veg->trunkShader, &veg->trunkStream, veg->trunks.data(), Count(veg->trunks), 3, false);
    DrawMeshInstancedData(veg->coreMesh, veg->coreShader, &veg->coreStream, veg->blobs.data(), Count(veg->blobs), 3, false);
    DrawMeshInstancedData(veg->cardMesh, veg->cardShader, &veg->blobStream, veg->blobs.data(), cardBlobs, 3, true);
    DrawMeshInstancedData(veg->pineMesh, veg->pineShader, &veg->pineStream, veg->pines.data(), Count(veg->pines), 3, false);
    for (int v = 0; v < ROCK_MESH_VARIANTS; v++) {
        DrawMeshInstancedData(veg->rockMeshes[v], veg->rockShader, &veg->rockStreams[v], veg->rocks[v].data(),
                              Count(veg->rocks[v]), 3, false);
    }
}

void DrawVegetationShadows(VegetationSystem* veg, const LightingSystem* lighting, int cascade,
                           const Tree* trees, int treeCount, const Rock* rocks, int rockCount) {
    ClearLists(veg);
    bool cards = cascade < 3;
    for (int i = 0; i < treeCount; i++) {
        if (!trees[i].alive) continue;
        Vector3 p = trees[i].position;
        p.y = GetTerrainHeight(p.x, p.z) + 4.0f;
        if (!IsShadowCasterVisible(lighting, cascade, p, trees[i].type == TREE_OAK ? 6.5f : 5.0f)) continue;
        BuildTree(veg, trees[i], cards, !cards);
    }
    for (int i = 0; i < rockCount; i++) {
        if (!rocks[i].alive) continue;
        Vector3 p = rocks[i].position;
        p.y = GetTerrainHeight(p.x, p.z) + 0.4f;
        if (!IsShadowCasterVisible(lighting, cascade, p, 1.4f)) continue;
        BuildRock(veg, rocks[i]);
    }
    int cardBlobs = PartitionCardBlobs(veg->blobs);
    InstanceStream* s = veg->shadowStreams[cascade];
    DrawMeshInstancedData(veg->trunkMesh, veg->trunkDepthShader, &s[0], veg->trunks.data(), Count(veg->trunks), 3, false);
    // Cores are shrunk inside card blobs; far cascades use full-size cores alone
    DrawMeshInstancedData(veg->coreMesh, veg->solidDepthShader, &s[1], veg->blobs.data(), Count(veg->blobs), 3, false);
    DrawMeshInstancedData(veg->cardMesh, veg->cardDepthShader, &s[2], veg->blobs.data(), cardBlobs, 3, true);
    DrawMeshInstancedData(veg->pineMesh, veg->pineDepthShader, &s[3], veg->pines.data(), Count(veg->pines), 3, false);
    for (int v = 0; v < ROCK_MESH_VARIANTS; v++) {
        DrawMeshInstancedData(veg->rockMeshes[v], veg->rockDepthShader, &s[5 + v], veg->rocks[v].data(),
                              Count(veg->rocks[v]), 3, false);
    }
}
