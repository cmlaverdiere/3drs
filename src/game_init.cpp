#include "game_init.h"
#include "math_utils.h"
#include "enemy_ai.h"
#include "sound_system.h"
#include "voice_system.h"
#include "xp_system.h"
#include "shader_utils.h"
#include "lighting.h"
#include "rlgl.h"
#include "raymath.h"
#include <cstdlib>
#include <cmath>
#include <climits>
#include <cstring>

void InitGameWindow(int* screenWidth, int* screenHeight) {
    // Keep the same native window alive while querying the monitor size.
    InitWindow(1, 1, "3D RuneScape-style Game");
    int monitorWidth = GetMonitorWidth(0);
    int monitorHeight = GetMonitorHeight(0);

    *screenWidth = monitorWidth;
    *screenHeight = monitorHeight - 80;
    SetWindowSize(*screenWidth, *screenHeight);
    SetWindowPosition(0, 25);
    rlSetClipPlanes(0.1, 1000.0);
    if (!IsWindowHidden()) {
        SetWindowFocused();
    }
    SetExitKey(0);  // Disable default ESC-to-close, handle manually

    InitAudioDevice();
    InitSoundSystem();
    InitVoiceSystem();
    // Music is initialized later in main() after season is determined
}

void InitPlayerState(PlayerState* state) {
    state->posX = 0.0f;
    state->posY = PLAYER_EYE_HEIGHT;
    state->posZ = 0.0f;
    state->targetX = 0.0f;
    state->targetY = PLAYER_EYE_HEIGHT;
    state->targetZ = 1.0f;

    for (int i = 0; i < SKILL_COUNT; i++) {
        state->skillXP[i] = 0;
    }

    for (int i = 0; i < INV_SLOTS; i++) {
        state->inventory[i] = ITEM_NONE;
        state->inventoryCount[i] = 0;
    }

    state->equippedWeapon = ITEM_NONE;
    state->swordPickedUp = false;
    // HP based on combat level (10 HP at level 1, +1 per level)
    int combatLevel = GetLevelFromXP(state->skillXP[SKILL_COMBAT]);
    state->maxHP = GetMaxHitpoints(combatLevel);
    state->currentHP = state->maxHP;
    state->timeOfDay = 0.5f;  // Start at midday
}

void InitCamera(Camera3D* camera, const PlayerState* state) {
    camera->position = (Vector3){ state->posX, state->posY, state->posZ };
    camera->target = (Vector3){ state->targetX, state->targetY, state->targetZ };
    camera->up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera->fovy = 60.0f;
    camera->projection = CAMERA_PERSPECTIVE;
}

GameResources LoadGameResources(const MapData& mapData, Wall* walls, Water* waterBodies, Sand* sandZones) {
    GameResources res = {};

    // Terrain: chunked heightmap mesh with the procedural ground shader
    res.grassShader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/terrain.vs", "shaders/terrain.fs"));

    // Wall shaders (use #include for common lighting)
    res.wallShaders[WALL_WOOD] = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/wall.vs", "shaders/wood.fs"));
    res.wallShaders[WALL_STONE] = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/wall.vs", "shaders/stone.fs"));
    res.wallShaders[WALL_BRICK] = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/wall.vs", "shaders/brick.fs"));
    for (int i = 0; i < WALL_MATERIAL_COUNT; i++) {
        res.wallBaseLocs[i] = GetShaderLocation(res.wallShaders[i], "uWallBase");
    }

    // Water shader
    res.waterShader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/water.vs", "shaders/water.fs"));
    res.waterTimeLoc = GetShaderLocation(res.waterShader, "time");

    // Entity shaders for lit enemies/trees/items (cubes get rounded-edge shading)
    res.entityShader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/entity.vs", "shaders/entity.fs"));
    res.entityModels.entityBevelShader = RegisterSceneShader(nullptr,
        LoadShaderVariant("shaders/entity.vs", "shaders/entity.fs", "BEVEL"));
    res.entityModels.entityEmissiveLoc = GetShaderLocation(res.entityShader, "uEmissive");
    res.entityModels.entityBevelEmissiveLoc = GetShaderLocation(res.entityModels.entityBevelShader, "uEmissive");

    // Depth shader for shadow map pass
    res.depthShader = LoadShaderWithIncludes("shaders/depth.vs", "shaders/depth.fs");

    // Create primitive models for entity rendering (with proper normals)
    // Store entity shader reference in EntityModels for restoration after monster shader
    res.entityModels.entityShader = res.entityShader;
    res.entityModels.depthShader = res.depthShader;

    // Unit cube (1x1x1), will be scaled per draw call
    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    res.entityModels.cube = LoadModelFromMesh(cubeMesh);
    res.entityModels.cube.materials[0].shader = res.entityModels.entityBevelShader;

    // Unit sphere (radius 1), will be scaled per draw call
    Mesh sphereMesh = GenMeshSphere(1.0f, 16, 16);
    res.entityModels.sphere = LoadModelFromMesh(sphereMesh);
    res.entityModels.sphere.materials[0].shader = res.entityShader;

    // Unit cylinder (radius 1, height 1), will be scaled per draw call
    Mesh cylinderMesh = GenMeshCylinder(1.0f, 1.0f, 16);
    res.entityModels.cylinder = LoadModelFromMesh(cylinderMesh);
    res.entityModels.cylinder.materials[0].shader = res.entityShader;

    // Fire shader and billboard plane for campfires
    res.entityModels.fireShader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/fire.vs", "shaders/fire.fs"));
    res.entityModels.fireParamsLoc = GetShaderLocation(res.entityModels.fireShader, "uFire");
    Mesh firePlaneMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);
    res.entityModels.firePlane = LoadModelFromMesh(firePlaneMesh);
    res.entityModels.firePlane.materials[0].shader = res.entityModels.fireShader;

    // Monster shader for procedural textures (scales, fur, stone, etc.)
    res.entityModels.monsterShader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/monster.vs", "shaders/monster.fs"));
    res.entityModels.monsterMaterialLoc = GetShaderLocation(res.entityModels.monsterShader, "materialType");
    res.entityModels.monsterSeedLoc = GetShaderLocation(res.entityModels.monsterShader, "monsterSeed");

    res.entityModels.initialized = true;


    // Terrain geometry (needs the heightmap) and zone uniforms for ground colouring
    InitTerrain(&res.terrain, res.grassShader, res.depthShader, mapData.sandZones, mapData.sandCount,
                mapData.waterBodies, mapData.waterCount);

    // Instanced trees and rocks
    InitVegetation(&res.vegetation);

    // Create wall models
    res.wallCount = mapData.wallCount;
    for (int i = 0; i < res.wallCount; i++) {
        walls[i] = mapData.walls[i];
        Mesh wallMesh = GenMeshCube(walls[i].width, walls[i].height, walls[i].depth);
        res.wallModels[i] = LoadModelFromMesh(wallMesh);
        res.wallModels[i].materials[0].shader = res.wallShaders[walls[i].material];
    }

    // Create water models
    res.waterCount = mapData.waterCount;
    for (int i = 0; i < res.waterCount; i++) {
        waterBodies[i] = mapData.waterBodies[i];
        Mesh waterMesh = GenMeshPlane(waterBodies[i].width, waterBodies[i].length, 20, 20);
        res.waterModels[i] = LoadModelFromMesh(waterMesh);
        res.waterModels[i].materials[0].shader = res.waterShader;
    }

    // Copy sand zones (still needed for spatial data, but no longer rendered as separate models)
    res.sandCount = mapData.sandCount;
    for (int i = 0; i < res.sandCount; i++) {
        sandZones[i] = mapData.sandZones[i];
    }

    // Grass blades follow the terrain's ground cover and avoid wall footprints
    InitGrassField(&res.grass, sandZones, res.sandCount, waterBodies, res.waterCount, walls, res.wallCount);

    return res;
}

void InitializeHeightmap(const MapData& mapData) {
    // Generate base procedural terrain
    for (int z = 0; z < HEIGHTMAP_SIZE; z++) {
        for (int x = 0; x < HEIGHTMAP_SIZE; x++) {
            float worldX = (x * HEIGHTMAP_SCALE) - HEIGHTMAP_OFFSET;
            float worldZ = (z * HEIGHTMAP_SCALE) - HEIGHTMAP_OFFSET;
            g_heightmap[z][x] = GenerateProceduralHeight(worldX, worldZ);
        }
    }

    // Apply valleys from map data
    for (int v = 0; v < mapData.valleyCount; v++) {
        const Valley& valley = mapData.valleys[v];

        for (int z = 0; z < HEIGHTMAP_SIZE; z++) {
            for (int x = 0; x < HEIGHTMAP_SIZE; x++) {
                float worldX = (x * HEIGHTMAP_SCALE) - HEIGHTMAP_OFFSET;
                float worldZ = (z * HEIGHTMAP_SCALE) - HEIGHTMAP_OFFSET;

                float dist;
                float extentPos;  // Position along the perpendicular axis
                if (valley.axis == 0) {
                    // X-axis valley: runs along Z, check Z extent
                    dist = fabsf(worldX - valley.position);
                    extentPos = worldZ;
                } else {
                    // Z-axis valley: runs along X, check X extent
                    dist = fabsf(worldZ - valley.position);
                    extentPos = worldX;
                }

                // Check if within the valley's extent range
                if (extentPos < valley.minExtent || extentPos > valley.maxExtent) {
                    continue;
                }

                if (dist < valley.width) {
                    float t = dist / valley.width;
                    float valleyFactor = 1.0f - t * t;
                    g_heightmap[z][x] -= valley.depth * valleyFactor;
                }
            }
        }
    }

    g_heightmapInitialized = true;
}

void InitEnemiesFromMap(Enemy* enemies, int* enemyCount, const MapData& mapData) {
    *enemyCount = mapData.enemyCount;
    for (int i = 0; i < *enemyCount; i++) {
        InitEnemy(&enemies[i], mapData.enemyTypes[i], mapData.enemySpawns[i]);
    }
}

void InitTreesFromMap(Tree* trees, int* treeCount, const MapData& mapData) {
    *treeCount = mapData.treeCount;
    for (int i = 0; i < *treeCount; i++) {
        trees[i].position = mapData.treeSpawns[i];
        trees[i].type = mapData.treeTypes[i];
        trees[i].health = TREE_MAX_HEALTH;
        trees[i].alive = true;
        trees[i].respawnTimer = 0.0f;
    }
}

void InitRocksFromMap(Rock* rocks, int* rockCount, const MapData& mapData) {
    *rockCount = mapData.rockCount;
    for (int i = 0; i < *rockCount; i++) {
        rocks[i].position = mapData.rockSpawns[i];
        rocks[i].type = mapData.rockTypes[i];
        rocks[i].health = ROCK_MAX_HEALTH;
        rocks[i].alive = true;
        rocks[i].respawnTimer = 0.0f;
    }
}

void InitItemsFromMap(WorldItem* items, int* itemCount, const MapData& mapData, bool swordPickedUp) {
    *itemCount = mapData.itemCount;
    for (int i = 0; i < *itemCount; i++) {
        items[i].type = mapData.itemTypes[i];
        items[i].position = mapData.itemSpawns[i];
        items[i].spawnPosition = mapData.itemSpawns[i];
        items[i].pickedUp = false;
        items[i].respawnTimer = 0.0f;
        items[i].canRespawn = true;  // Map-spawned items respawn
        items[i].quantity = 1;       // Map items spawn with quantity 1
    }
    if (*itemCount > 0) {
        items[0].pickedUp = swordPickedUp;
    }
}

void PopulateSpatialHash(WorldSpatialData* spatial, const Wall* walls, int wallCount,
                         const Enemy* enemies, int enemyCount,
                         const Tree* trees, int treeCount) {
    spatial->Clear();
    for (int i = 0; i < wallCount; i++) {
        spatial->walls.InsertBox(i, walls[i].position.x, walls[i].position.z,
                                  walls[i].width, walls[i].depth);
    }
    for (int i = 0; i < enemyCount; i++) {
        spatial->enemies.Insert(i, enemies[i].position.x, enemies[i].position.z);
    }
    for (int i = 0; i < treeCount; i++) {
        spatial->trees.Insert(i, trees[i].position.x, trees[i].position.z);
    }
    TraceLog(LOG_INFO, "Spatial hash populated: %d walls, %d enemies, %d trees",
             wallCount, enemyCount, treeCount);
}

void CleanupGameResources(GameResources* res) {
    UnloadTerrain(&res->terrain);
    UnloadVegetation(&res->vegetation);
    UnloadShader(res->grassShader);
    UnloadGrassField(&res->grass);

    for (int i = 0; i < res->wallCount; i++) {
        UnloadModel(res->wallModels[i]);
    }
    for (int i = 0; i < WALL_MATERIAL_COUNT; i++) {
        UnloadShader(res->wallShaders[i]);
    }

    for (int i = 0; i < res->waterCount; i++) {
        UnloadModel(res->waterModels[i]);
    }
    UnloadShader(res->waterShader);

    UnloadShader(res->entityShader);
    UnloadShader(res->depthShader);

    // Unload entity primitive models
    if (res->entityModels.initialized) {
        UnloadModel(res->entityModels.cube);
        UnloadModel(res->entityModels.sphere);
        UnloadModel(res->entityModels.cylinder);
        UnloadModel(res->entityModels.firePlane);
        UnloadShader(res->entityModels.fireShader);
        UnloadShader(res->entityModels.monsterShader);
        UnloadShader(res->entityModels.entityBevelShader);
    }

    UnloadSoundSystem();
    UnloadVoiceSystem();
    CloseAudioDevice();
}

// Snow particle system implementation
void InitSnowSystem(SnowSystem* snow, Vector3 centerPos) {
    for (int i = 0; i < SNOW_PARTICLE_COUNT; i++) {
        // Random position within spawn radius around center
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float dist = (float)GetRandomValue(0, (int)(SNOW_SPAWN_RADIUS * 100)) / 100.0f;
        snow->particles[i].position.x = centerPos.x + cosf(angle) * dist;
        snow->particles[i].position.z = centerPos.z + sinf(angle) * dist;
        snow->particles[i].position.y = centerPos.y + (float)GetRandomValue(0, (int)(SNOW_HEIGHT * 100)) / 100.0f;

        // Random fall speed
        snow->particles[i].speed = 1.5f + (float)GetRandomValue(0, 200) / 100.0f;
        // Random wobble phase
        snow->particles[i].wobble = (float)GetRandomValue(0, 628) / 100.0f;
    }
    Mesh quad = {};
    quad.vertexCount = 4;
    quad.triangleCount = 2;
    quad.vertices = (float*)RL_CALLOC(12, sizeof(float));
    quad.texcoords = (float*)RL_MALLOC(8 * sizeof(float));
    quad.indices = (unsigned short*)RL_MALLOC(6 * sizeof(unsigned short));
    const float corners[8] = {-1, -1, 1, -1, 1, 1, -1, 1};
    const unsigned short idx[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 8; i++) quad.texcoords[i] = corners[i];
    for (int i = 0; i < 6; i++) quad.indices[i] = idx[i];
    UploadMesh(&quad, false);
    snow->flakeMesh = quad;
    snow->shader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/snow.vs", "shaders/snow.fs"));
    snow->initialized = true;
}

void UpdateAndDrawSnow(SnowSystem* snow, Vector3 centerPos, float deltaTime) {
    if (!snow->initialized) return;

    float time = (float)GetTime();

    for (int i = 0; i < SNOW_PARTICLE_COUNT; i++) {
        SnowParticle* p = &snow->particles[i];

        // Fall down
        p->position.y -= p->speed * deltaTime;

        // Gentle side-to-side drift
        float drift = sinf(time * 2.0f + p->wobble) * 0.5f * deltaTime;
        p->position.x += drift;
        p->position.z += cosf(time * 1.5f + p->wobble * 0.7f) * 0.3f * deltaTime;

        // Respawn at top if below ground or too far from player
        float dx = p->position.x - centerPos.x;
        float dz = p->position.z - centerPos.z;
        float distSq = dx * dx + dz * dz;

        if (p->position.y < centerPos.y - 5.0f || distSq > SNOW_SPAWN_RADIUS * SNOW_SPAWN_RADIUS * 1.5f) {
            // Respawn at random position above player
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float dist = (float)GetRandomValue(0, (int)(SNOW_SPAWN_RADIUS * 100)) / 100.0f;
            p->position.x = centerPos.x + cosf(angle) * dist;
            p->position.z = centerPos.z + sinf(angle) * dist;
            p->position.y = centerPos.y + SNOW_HEIGHT;
        }

        // Soft billboard flake (drawn instanced below)
        float size = 0.035f + (float)((i % 3)) * 0.016f;
        float v[4] = {p->position.x, p->position.y, p->position.z, size};
        snow->instances.insert(snow->instances.end(), v, v + 4);
    }
    DrawMeshInstancedData(snow->flakeMesh, snow->shader, &snow->stream, snow->instances.data(),
                          (int)(snow->instances.size() / 4), 1, true);
    snow->instances.clear();
}

// Falling leaves particle system implementation
void InitLeafSystem(LeafSystem* leaves, Vector3 centerPos) {
    // Load leaf shader
    leaves->leafShader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/leaf.vs", "shaders/leaf.fs"));

    // Create a simple quad mesh for leaves
    leaves->leafMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);

    // Initialize particles
    for (int i = 0; i < LEAF_PARTICLE_COUNT; i++) {
        // Random position within spawn radius around center
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float dist = (float)GetRandomValue(0, (int)(LEAF_SPAWN_RADIUS * 100)) / 100.0f;
        leaves->particles[i].position.x = centerPos.x + cosf(angle) * dist;
        leaves->particles[i].position.z = centerPos.z + sinf(angle) * dist;
        leaves->particles[i].position.y = centerPos.y + (float)GetRandomValue(0, (int)(LEAF_HEIGHT * 100)) / 100.0f;

        // Random fall speed (slower than snow, leaves flutter)
        leaves->particles[i].fallSpeed = 0.6f + (float)GetRandomValue(0, 120) / 100.0f;
        // Random horizontal drift speed
        leaves->particles[i].driftSpeed = 0.8f + (float)GetRandomValue(0, 180) / 100.0f;
        // Random rotations
        leaves->particles[i].rotationY = (float)GetRandomValue(0, 360);
        leaves->particles[i].rotationTumble = (float)GetRandomValue(0, 360);
        leaves->particles[i].rotationSpeed = (float)GetRandomValue(-150, 150);
        leaves->particles[i].tumbleSpeed = (float)GetRandomValue(50, 200);
        // Random size (0.15 to 0.25)
        leaves->particles[i].size = 0.15f + (float)GetRandomValue(0, 100) / 1000.0f;
        // Random color type
        leaves->particles[i].colorType = GetRandomValue(0, 2);
    }
    leaves->initialized = true;
}

void UpdateAndDrawLeaves(LeafSystem* leaves, Vector3 centerPos, Vector3 viewPos,
                         Vector3 fogColor, float fogDensity, float deltaTime) {
    if (!leaves->initialized) return;

    float time = (float)GetTime();

    for (int i = 0; i < LEAF_PARTICLE_COUNT; i++) {
        LeafParticle* p = &leaves->particles[i];

        // Fall down with gentle oscillation
        p->position.y -= p->fallSpeed * deltaTime;

        // Flutter side-to-side
        float flutter = sinf(time * 2.5f + p->rotationY * 0.01f) * p->driftSpeed * deltaTime;
        p->position.x += flutter;
        p->position.z += cosf(time * 2.0f + p->rotationY * 0.02f) * p->driftSpeed * 0.6f * deltaTime;

        // Rotate the leaf
        p->rotationY += p->rotationSpeed * deltaTime;
        p->rotationTumble += p->tumbleSpeed * deltaTime;

        // Respawn at top if below ground or too far from player
        float dx = p->position.x - centerPos.x;
        float dz = p->position.z - centerPos.z;
        float distSq = dx * dx + dz * dz;

        if (p->position.y < centerPos.y - 5.0f || distSq > LEAF_SPAWN_RADIUS * LEAF_SPAWN_RADIUS * 1.5f) {
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float dist = (float)GetRandomValue(0, (int)(LEAF_SPAWN_RADIUS * 100)) / 100.0f;
            p->position.x = centerPos.x + cosf(angle) * dist;
            p->position.z = centerPos.z + sinf(angle) * dist;
            p->position.y = centerPos.y + LEAF_HEIGHT;
            p->colorType = GetRandomValue(0, 2);
        }

        float inst[8] = {p->position.x, p->position.y, p->position.z, p->size,
                         p->rotationY * DEG2RAD, p->rotationTumble * DEG2RAD, 1.0f, (float)p->colorType};
        leaves->instances.insert(leaves->instances.end(), inst, inst + 8);
    }
    DrawMeshInstancedData(leaves->leafMesh, leaves->leafShader, &leaves->stream, leaves->instances.data(),
                          (int)(leaves->instances.size() / 8), 2, true);
    leaves->instances.clear();
}

void CleanupLeafSystem(LeafSystem* leaves) {
    if (!leaves->initialized) return;
    UnloadShader(leaves->leafShader);
    UnloadMesh(leaves->leafMesh);
    UnloadInstanceStream(&leaves->stream);
    leaves->initialized = false;
}

// ============================================================================
// Leaf Burst Particle System (for tree chopping in autumn)
// ============================================================================

void InitLeafBurstSystem(LeafBurstSystem* system) {
    // Load leaf shader
    system->leafShader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/leaf.vs", "shaders/leaf.fs"));

    // Create a simple quad mesh for leaves
    system->leafMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);

    // Initialize all bursts as inactive
    for (int i = 0; i < MAX_LEAF_BURSTS; i++) {
        system->bursts[i].active = false;
    }

    system->initialized = true;
}

void SpawnLeafBurst(LeafBurstSystem* system, Vector3 position, float treeHeight) {
    if (!system->initialized) return;

    // Find an inactive burst slot
    int slot = -1;
    for (int i = 0; i < MAX_LEAF_BURSTS; i++) {
        if (!system->bursts[i].active) {
            slot = i;
            break;
        }
    }

    // If no slot available, use the oldest one (first in array)
    if (slot < 0) slot = 0;

    LeafBurst* burst = &system->bursts[slot];
    burst->active = true;
    burst->lifetime = LEAF_BURST_LIFETIME;

    // Spawn particles from tree canopy area
    float canopyY = position.y + treeHeight * 0.7f;  // Middle-upper part of tree
    float canopyRadius = 2.0f;  // Spread within tree canopy

    for (int i = 0; i < LEAF_BURST_PARTICLE_COUNT; i++) {
        LeafBurstParticle* p = &burst->particles[i];
        p->active = true;

        // Random position within tree canopy
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float dist = (float)GetRandomValue(0, (int)(canopyRadius * 100)) / 100.0f;
        float heightOffset = (float)GetRandomValue(-100, 100) / 100.0f * treeHeight * 0.3f;

        p->position.x = position.x + cosf(angle) * dist;
        p->position.z = position.z + sinf(angle) * dist;
        p->position.y = canopyY + heightOffset;

        // Initial velocity - burst outward and up, then fall
        float speed = 2.0f + (float)GetRandomValue(0, 400) / 100.0f;
        float vertAngle = (float)GetRandomValue(20, 70) * DEG2RAD;  // Launch angle
        float horizAngle = (float)GetRandomValue(0, 360) * DEG2RAD;

        p->velocity.x = cosf(horizAngle) * cosf(vertAngle) * speed;
        p->velocity.z = sinf(horizAngle) * cosf(vertAngle) * speed;
        p->velocity.y = sinf(vertAngle) * speed * 0.8f;  // Upward burst

        // Random rotations
        p->rotationY = (float)GetRandomValue(0, 360);
        p->rotationTumble = (float)GetRandomValue(0, 360);
        p->rotationSpeed = (float)GetRandomValue(-300, 300);
        p->tumbleSpeed = (float)GetRandomValue(100, 400);

        // Random size (0.12 to 0.22)
        p->size = 0.12f + (float)GetRandomValue(0, 100) / 1000.0f;

        // Random color type
        p->colorType = GetRandomValue(0, 2);
    }
}

void UpdateAndDrawLeafBursts(LeafBurstSystem* system, Vector3 viewPos,
                              Vector3 fogColor, float fogDensity, float deltaTime) {
    if (!system->initialized) return;

    // Check if any burst is active
    bool anyActive = false;
    for (int i = 0; i < MAX_LEAF_BURSTS; i++) {
        if (system->bursts[i].active) {
            anyActive = true;
            break;
        }
    }
    if (!anyActive) return;

    const float gravity = 3.5f;
    const float drag = 0.5f;

    for (int b = 0; b < MAX_LEAF_BURSTS; b++) {
        LeafBurst* burst = &system->bursts[b];
        if (!burst->active) continue;

        burst->lifetime -= deltaTime;
        if (burst->lifetime <= 0) {
            burst->active = false;
            continue;
        }

        // Fade alpha based on remaining lifetime
        float alpha = fminf(1.0f, burst->lifetime / 0.5f);  // Fade in last 0.5s

        for (int i = 0; i < LEAF_BURST_PARTICLE_COUNT; i++) {
            LeafBurstParticle* p = &burst->particles[i];
            if (!p->active) continue;

            // Apply gravity
            p->velocity.y -= gravity * deltaTime;

            // Apply drag (air resistance) - more on horizontal
            p->velocity.x *= (1.0f - drag * deltaTime);
            p->velocity.z *= (1.0f - drag * deltaTime);
            p->velocity.y *= (1.0f - drag * 0.3f * deltaTime);

            // Add some flutter
            float time = (float)GetTime();
            float flutter = sinf(time * 3.0f + p->rotationY * 0.05f) * 1.5f * deltaTime;
            p->velocity.x += flutter;
            p->velocity.z += cosf(time * 2.5f + p->rotationTumble * 0.03f) * 1.0f * deltaTime;

            // Update position
            p->position.x += p->velocity.x * deltaTime;
            p->position.y += p->velocity.y * deltaTime;
            p->position.z += p->velocity.z * deltaTime;

            // Rotate the leaf (faster when moving fast)
            float speed = sqrtf(p->velocity.x * p->velocity.x + p->velocity.z * p->velocity.z);
            p->rotationY += p->rotationSpeed * deltaTime;
            p->rotationTumble += p->tumbleSpeed * (1.0f + speed * 0.5f) * deltaTime;

            // Deactivate if below ground
            float groundY = GetTerrainHeight(p->position.x, p->position.z);
            if (p->position.y < groundY) {
                p->active = false;
                continue;
            }

            float inst[8] = {p->position.x, p->position.y, p->position.z, p->size,
                             p->rotationY * DEG2RAD, p->rotationTumble * DEG2RAD, alpha, (float)p->colorType};
            system->instances.insert(system->instances.end(), inst, inst + 8);
        }
    }
    if (!system->instances.empty()) {
        DrawMeshInstancedData(system->leafMesh, system->leafShader, &system->stream, system->instances.data(),
                              (int)(system->instances.size() / 8), 2, true);
        system->instances.clear();
    }
}

void CleanupLeafBurstSystem(LeafBurstSystem* system) {
    if (!system->initialized) return;
    UnloadShader(system->leafShader);
    UnloadMesh(system->leafMesh);
    UnloadInstanceStream(&system->stream);
    system->initialized = false;
}

// ============================================================================
// Blood Splatter Particle System
// ============================================================================

void InitBloodSplatterSystem(BloodSplatterSystem* system) {
    // Load blood shader
    system->bloodShader = RegisterSceneShader(nullptr, LoadShaderWithIncludes("shaders/blood.vs", "shaders/blood.fs"));
    system->viewPosLoc = GetShaderLocation(system->bloodShader, "viewPos");
    system->stretchLoc = GetShaderLocation(system->bloodShader, "stretch");

    // Create a simple quad mesh for blood droplets
    system->bloodMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);

    // Setup material with shader
    system->bloodMaterial = LoadMaterialDefault();
    system->bloodMaterial.shader = system->bloodShader;

    for (int i = 0; i < MAX_BLOOD_SPLATTERS; i++) {
        system->splatters[i].active = false;
    }
    system->initialized = true;
}

void SpawnBloodSplatter(BloodSplatterSystem* system, Vector3 position, Vector3 hitDirection) {
    if (!system->initialized) return;

    // Find an inactive splatter slot
    int slot = -1;
    for (int i = 0; i < MAX_BLOOD_SPLATTERS; i++) {
        if (!system->splatters[i].active) {
            slot = i;
            break;
        }
    }

    // If no slot available, use the oldest one
    if (slot < 0) slot = 0;

    BloodSplatter* splatter = &system->splatters[slot];
    splatter->active = true;
    splatter->lifetime = BLOOD_SPLATTER_LIFETIME;

    // Normalize hit direction (direction attack came from)
    float hitLen = sqrtf(hitDirection.x * hitDirection.x +
                         hitDirection.y * hitDirection.y +
                         hitDirection.z * hitDirection.z);
    if (hitLen > 0.01f) {
        hitDirection.x /= hitLen;
        hitDirection.y /= hitLen;
        hitDirection.z /= hitLen;
    }

    for (int i = 0; i < BLOOD_PARTICLE_COUNT; i++) {
        BloodParticle* p = &splatter->particles[i];
        p->active = true;

        // Start at hit position with slight random offset
        p->position.x = position.x + (float)GetRandomValue(-20, 20) / 100.0f;
        p->position.y = position.y + (float)GetRandomValue(-20, 20) / 100.0f;
        p->position.z = position.z + (float)GetRandomValue(-20, 20) / 100.0f;

        // Velocity - spray in direction of hit with spread
        float speed = 3.0f + (float)GetRandomValue(0, 500) / 100.0f;
        float spreadAngle = (float)GetRandomValue(-60, 60) * DEG2RAD;
        float vertAngle = (float)GetRandomValue(-30, 45) * DEG2RAD;

        // Base direction is opposite to hit direction (blood sprays away from attacker)
        float baseAngle = atan2f(hitDirection.x, hitDirection.z) + PI;

        p->velocity.x = sinf(baseAngle + spreadAngle) * cosf(vertAngle) * speed;
        p->velocity.z = cosf(baseAngle + spreadAngle) * cosf(vertAngle) * speed;
        p->velocity.y = sinf(vertAngle) * speed + 2.0f;  // Slight upward bias

        // Random particle size (small droplets)
        p->size = 0.04f + (float)GetRandomValue(0, 30) / 1000.0f;

        // Initial rotation based on velocity direction
        p->rotation = atan2f(p->velocity.x, p->velocity.z) * RAD2DEG;

        // Initial stretch based on speed
        p->stretchFactor = speed / 5.0f;
    }
}

void UpdateAndDrawBloodSplatters(BloodSplatterSystem* system, Vector3 viewPos, float deltaTime) {
    if (!system->initialized) return;

    // Check if any splatter is active
    bool anyActive = false;
    for (int b = 0; b < MAX_BLOOD_SPLATTERS; b++) {
        if (system->splatters[b].active) {
            anyActive = true;
            break;
        }
    }
    if (!anyActive) return;

    // Set shader uniforms
    float viewPosArr[3] = { viewPos.x, viewPos.y, viewPos.z };
    SetShaderValue(system->bloodShader, system->viewPosLoc, viewPosArr, SHADER_UNIFORM_VEC3);

    // Blood colors (dark red to bright red)
    Color bloodColors[] = {
        { 139, 0, 0, 255 },    // Dark red
        { 178, 34, 34, 255 },  // Firebrick
        { 200, 20, 20, 255 },  // Bright red
    };

    const float BLOOD_GRAVITY = 12.0f;

    // Disable backface culling for double-sided droplets
    rlDisableBackfaceCulling();

    for (int b = 0; b < MAX_BLOOD_SPLATTERS; b++) {
        BloodSplatter* splatter = &system->splatters[b];
        if (!splatter->active) continue;

        splatter->lifetime -= deltaTime;
        if (splatter->lifetime <= 0) {
            splatter->active = false;
            continue;
        }

        // Fade out near end of lifetime
        float alpha = (splatter->lifetime < 0.5f) ? (splatter->lifetime / 0.5f) : 1.0f;

        for (int i = 0; i < BLOOD_PARTICLE_COUNT; i++) {
            BloodParticle* p = &splatter->particles[i];
            if (!p->active) continue;

            // Apply gravity
            p->velocity.y -= BLOOD_GRAVITY * deltaTime;

            // Update position
            p->position.x += p->velocity.x * deltaTime;
            p->position.y += p->velocity.y * deltaTime;
            p->position.z += p->velocity.z * deltaTime;

            // Deactivate if below ground
            float groundY = GetTerrainHeight(p->position.x, p->position.z);
            if (p->position.y < groundY) {
                p->active = false;
                continue;
            }

            // Update rotation to face velocity direction
            float horizSpeed = sqrtf(p->velocity.x * p->velocity.x + p->velocity.z * p->velocity.z);
            if (horizSpeed > 0.1f) {
                p->rotation = atan2f(p->velocity.x, p->velocity.z) * RAD2DEG;
            }

            // Update stretch based on current speed (subtle effect)
            float totalSpeed = sqrtf(p->velocity.x * p->velocity.x +
                                     p->velocity.y * p->velocity.y +
                                     p->velocity.z * p->velocity.z);
            p->stretchFactor = fminf(totalSpeed / 10.0f, 0.5f);

            // Set stretch uniform
            SetShaderValue(system->bloodShader, system->stretchLoc, &p->stretchFactor, SHADER_UNIFORM_FLOAT);

            // Set color with alpha
            Color color = bloodColors[i % 3];
            color.a = (unsigned char)(255 * alpha);
            system->bloodMaterial.maps[MATERIAL_MAP_DIFFUSE].color = color;

            // Simple billboard: rotate to face camera using yaw only (Y-axis rotation)
            // This keeps droplets upright which looks natural for falling blood
            float dx = viewPos.x - p->position.x;
            float dz = viewPos.z - p->position.z;
            float yaw = atan2f(dx, dz);

            // Scale with stretch in Y direction (droplet elongation)
            float scaleX = p->size;
            float scaleY = p->size * (1.0f + p->stretchFactor * 0.5f);

            // Build transform: scale, rotate to vertical, rotate to face camera, translate
            Matrix matScale = MatrixScale(scaleX, scaleY, scaleX);
            Matrix matRotateVertical = MatrixRotateX(-90.0f * DEG2RAD);  // Plane horizontal -> vertical
            Matrix matRotateY = MatrixRotateY(yaw);
            Matrix matTranslate = MatrixTranslate(p->position.x, p->position.y, p->position.z);

            Matrix transform = MatrixMultiply(matScale, matRotateVertical);
            transform = MatrixMultiply(transform, matRotateY);
            transform = MatrixMultiply(transform, matTranslate);

            // Draw the blood droplet
            DrawMesh(system->bloodMesh, system->bloodMaterial, transform);
        }
    }

    rlEnableBackfaceCulling();
}

void CleanupBloodSplatterSystem(BloodSplatterSystem* system) {
    if (!system->initialized) return;
    UnloadShader(system->bloodShader);
    UnloadMesh(system->bloodMesh);
    system->initialized = false;
}
