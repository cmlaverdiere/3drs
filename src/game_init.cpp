#include "game_init.h"
#include "math_utils.h"
#include "enemy_ai.h"
#include "sound_system.h"
#include "voice_system.h"
#include "xp_system.h"
#include "shader_utils.h"
#include "rlgl.h"
#include "raymath.h"
#include <cstdlib>
#include <cmath>

void InitGameWindow(int* screenWidth, int* screenHeight) {
    // Get monitor size first
    InitWindow(1, 1, "3D RuneScape-style Game");
    int monitorWidth = GetMonitorWidth(0);
    int monitorHeight = GetMonitorHeight(0);
    CloseWindow();

    *screenWidth = monitorWidth;
    *screenHeight = monitorHeight - 80;
    InitWindow(*screenWidth, *screenHeight, "3D RuneScape-style Game");
    SetWindowPosition(0, 25);
    SetExitKey(0);  // Disable default ESC-to-close, handle manually

    InitAudioDevice();
    InitSoundSystem();
    InitVoiceSystem();
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

    // Grass/ground shader and ground model (uses #include for common lighting)
    res.grassShader = LoadShaderWithIncludes("shaders/grass.vs", "shaders/grass.fs");
    Mesh groundMesh = GenHeightmapMesh(512.0f, 512.0f, 256, 256);
    res.groundModel = LoadModelFromMesh(groundMesh);
    res.groundModel.materials[0].shader = res.grassShader;

    // Pass season to ground shader (0=Spring, 1=Summer, 2=Autumn, 3=Winter)
    int seasonLoc = GetShaderLocation(res.grassShader, "season");
    int seasonVal = (int)g_currentSeason;
    SetShaderValue(res.grassShader, seasonLoc, &seasonVal, SHADER_UNIFORM_INT);

    // Pass sand zone data to ground shader
    int sandZoneCountLoc = GetShaderLocation(res.grassShader, "sandZoneCount");
    int sandZonesLoc = GetShaderLocation(res.grassShader, "sandZones");
    int sandCount = mapData.sandCount;
    SetShaderValue(res.grassShader, sandZoneCountLoc, &sandCount, SHADER_UNIFORM_INT);
    // Pack sand zones as vec4 (x, z, width, length)
    for (int i = 0; i < mapData.sandCount && i < 16; i++) {
        float zoneData[4] = {
            mapData.sandZones[i].position.x,
            mapData.sandZones[i].position.z,
            mapData.sandZones[i].width,
            mapData.sandZones[i].length
        };
        SetShaderValue(res.grassShader, sandZonesLoc + i, zoneData, SHADER_UNIFORM_VEC4);
    }

    // Wall shaders (use #include for common lighting)
    res.wallShaders[WALL_WOOD] = LoadShaderWithIncludes("shaders/wall.vs", "shaders/wood.fs");
    res.wallShaders[WALL_STONE] = LoadShaderWithIncludes("shaders/wall.vs", "shaders/stone.fs");
    res.wallShaders[WALL_BRICK] = LoadShaderWithIncludes("shaders/wall.vs", "shaders/brick.fs");

    // Water shader
    res.waterShader = LoadShader("shaders/water.vs", "shaders/water.fs");
    res.waterTimeLoc = GetShaderLocation(res.waterShader, "time");

    // Entity shader for lit enemies/trees/items
    res.entityShader = LoadShaderWithIncludes("shaders/entity.vs", "shaders/entity.fs");

    // Depth shader for shadow map pass
    res.depthShader = LoadShader("shaders/depth.vs", "shaders/depth.fs");

    // Sky shader and model
    res.skyShader = LoadShader("shaders/sky.vs", "shaders/sky.fs");
    Mesh skyMesh = GenMeshSphere(500.0f, 32, 32);  // Large sphere around scene
    res.skyModel = LoadModelFromMesh(skyMesh);
    res.skyModel.materials[0].shader = res.skyShader;

    // Create primitive models for entity rendering (with proper normals)
    // Unit cube (1x1x1), will be scaled per draw call
    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    res.entityModels.cube = LoadModelFromMesh(cubeMesh);
    res.entityModels.cube.materials[0].shader = res.entityShader;

    // Unit sphere (radius 1), will be scaled per draw call
    Mesh sphereMesh = GenMeshSphere(1.0f, 16, 16);
    res.entityModels.sphere = LoadModelFromMesh(sphereMesh);
    res.entityModels.sphere.materials[0].shader = res.entityShader;

    // Unit cylinder (radius 1, height 1), will be scaled per draw call
    Mesh cylinderMesh = GenMeshCylinder(1.0f, 1.0f, 16);
    res.entityModels.cylinder = LoadModelFromMesh(cylinderMesh);
    res.entityModels.cylinder.materials[0].shader = res.entityShader;

    // Fire shader and billboard plane for campfires
    res.entityModels.fireShader = LoadShader("shaders/fire.vs", "shaders/fire.fs");
    res.entityModels.fireTimeLoc = GetShaderLocation(res.entityModels.fireShader, "time");
    Mesh firePlaneMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);
    res.entityModels.firePlane = LoadModelFromMesh(firePlaneMesh);
    res.entityModels.firePlane.materials[0].shader = res.entityModels.fireShader;

    res.entityModels.initialized = true;

    // Initialize grass blade system
    InitGrassSystem(&res.grass);

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

Mesh GenHeightmapMesh(float sizeX, float sizeZ, int resX, int resZ) {
    Mesh mesh = { 0 };

    int vertexCount = resX * resZ;
    int triangleCount = (resX - 1) * (resZ - 1) * 2;

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = triangleCount;
    mesh.vertices = (float*)RL_MALLOC(vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)RL_MALLOC(vertexCount * 2 * sizeof(float));
    mesh.normals = (float*)RL_MALLOC(vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)RL_MALLOC(triangleCount * 3 * sizeof(unsigned short));

    float halfX = sizeX / 2.0f;
    float halfZ = sizeZ / 2.0f;

    int vi = 0;
    for (int z = 0; z < resZ; z++) {
        for (int x = 0; x < resX; x++) {
            float worldX = -halfX + (x / (float)(resX - 1)) * sizeX;
            float worldZ = -halfZ + (z / (float)(resZ - 1)) * sizeZ;
            float height = GetTerrainHeight(worldX, worldZ);

            mesh.vertices[vi * 3 + 0] = worldX;
            mesh.vertices[vi * 3 + 1] = height;
            mesh.vertices[vi * 3 + 2] = worldZ;

            mesh.texcoords[vi * 2 + 0] = x / (float)(resX - 1);
            mesh.texcoords[vi * 2 + 1] = z / (float)(resZ - 1);

            mesh.normals[vi * 3 + 0] = 0.0f;
            mesh.normals[vi * 3 + 1] = 1.0f;
            mesh.normals[vi * 3 + 2] = 0.0f;

            vi++;
        }
    }

    int ii = 0;
    for (int z = 0; z < resZ - 1; z++) {
        for (int x = 0; x < resX - 1; x++) {
            int topLeft = z * resX + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * resX + x;
            int bottomRight = bottomLeft + 1;

            mesh.indices[ii++] = topLeft;
            mesh.indices[ii++] = bottomLeft;
            mesh.indices[ii++] = topRight;

            mesh.indices[ii++] = topRight;
            mesh.indices[ii++] = bottomLeft;
            mesh.indices[ii++] = bottomRight;
        }
    }

    UploadMesh(&mesh, false);
    return mesh;
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

// Generate a combined mesh with all grass blades baked in (single draw call)
static Mesh GenCombinedGrassMesh(float bladeWidth, float bladeHeight, int bladeCount, float spawnRadius) {
    Mesh mesh = { 0 };

    int vertsPerBlade = 4;
    int trisPerBlade = 2;

    mesh.vertexCount = bladeCount * vertsPerBlade;
    mesh.triangleCount = bladeCount * trisPerBlade;
    mesh.vertices = (float*)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float*)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)RL_MALLOC(mesh.triangleCount * 3 * sizeof(unsigned short));

    float halfWidth = bladeWidth * 0.5f;
    float tipWidth = bladeWidth * 0.15f;

    srand(12345);  // Fixed seed for consistent placement

    for (int i = 0; i < bladeCount; i++) {
        // Random position
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float dist = sqrtf((float)rand() / RAND_MAX) * spawnRadius;
        float x = cosf(angle) * dist;
        float z = sinf(angle) * dist;
        float y = GetTerrainHeight(x, z);

        // Random rotation and scale
        float rotY = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float scale = 0.8f + ((float)rand() / RAND_MAX) * 0.4f;
        float cosR = cosf(rotY);
        float sinR = sinf(rotY);

        float w = halfWidth * scale;
        float tw = tipWidth * scale;
        float h = bladeHeight * scale;

        int vi = i * vertsPerBlade * 3;
        int ti = i * vertsPerBlade * 2;
        int ii = i * trisPerBlade * 3;

        // Bottom left
        mesh.vertices[vi + 0] = x + (-w * cosR);
        mesh.vertices[vi + 1] = y;
        mesh.vertices[vi + 2] = z + (-w * sinR);
        mesh.texcoords[ti + 0] = 0.0f; mesh.texcoords[ti + 1] = 0.0f;

        // Bottom right
        mesh.vertices[vi + 3] = x + (w * cosR);
        mesh.vertices[vi + 4] = y;
        mesh.vertices[vi + 5] = z + (w * sinR);
        mesh.texcoords[ti + 2] = 1.0f; mesh.texcoords[ti + 3] = 0.0f;

        // Top left
        mesh.vertices[vi + 6] = x + (-tw * cosR);
        mesh.vertices[vi + 7] = y + h;
        mesh.vertices[vi + 8] = z + (-tw * sinR);
        mesh.texcoords[ti + 4] = 0.0f; mesh.texcoords[ti + 5] = 1.0f;

        // Top right
        mesh.vertices[vi + 9] = x + (tw * cosR);
        mesh.vertices[vi + 10] = y + h;
        mesh.vertices[vi + 11] = z + (tw * sinR);
        mesh.texcoords[ti + 6] = 1.0f; mesh.texcoords[ti + 7] = 1.0f;

        // Normals pointing up-ish
        for (int j = 0; j < vertsPerBlade; j++) {
            int ni = (i * vertsPerBlade + j) * 3;
            mesh.normals[ni + 0] = sinR * 0.3f;
            mesh.normals[ni + 1] = 0.9f;
            mesh.normals[ni + 2] = cosR * 0.3f;
        }

        // Indices
        unsigned short base = i * vertsPerBlade;
        mesh.indices[ii + 0] = base + 0;
        mesh.indices[ii + 1] = base + 1;
        mesh.indices[ii + 2] = base + 2;
        mesh.indices[ii + 3] = base + 1;
        mesh.indices[ii + 4] = base + 3;
        mesh.indices[ii + 5] = base + 2;
    }

    UploadMesh(&mesh, false);
    return mesh;
}

void InitGrassSystem(GrassSystem* grass) {
    // Load grass blade shader
    grass->bladeShader = LoadShader("shaders/grass_blade.vs", "shaders/grass_blade.fs");
    grass->timeLoc = GetShaderLocation(grass->bladeShader, "time");

    // Pass season to blade shader (0=Spring, 1=Summer, 2=Autumn, 3=Winter)
    int seasonLoc = GetShaderLocation(grass->bladeShader, "season");
    int seasonVal = (int)g_currentSeason;
    SetShaderValue(grass->bladeShader, seasonLoc, &seasonVal, SHADER_UNIFORM_INT);

    // Generate combined mesh with all blades baked in (single draw call!)
    grass->bladeMesh = GenCombinedGrassMesh(0.12f, 0.22f, GRASS_BLADE_COUNT, GRASS_SPAWN_RADIUS);

    // Setup material
    grass->bladeMaterial = LoadMaterialDefault();
    grass->bladeMaterial.shader = grass->bladeShader;

    // No transforms needed - positions baked into mesh
    grass->transforms = NULL;
    grass->bladeCount = GRASS_BLADE_COUNT;

    grass->initialized = true;
    TraceLog(LOG_INFO, "Grass system initialized with %d blades (baked mesh)", GRASS_BLADE_COUNT);
}

void DrawGrassBlades(GrassSystem* grass, float time) {
    if (!grass->initialized) return;

    // Update time uniform for wind animation
    SetShaderValue(grass->bladeShader, grass->timeLoc, &time, SHADER_UNIFORM_FLOAT);

    // Disable backface culling so grass is visible from both sides
    rlDisableBackfaceCulling();

    // Single draw call for all grass (positions baked into mesh)
    DrawMesh(grass->bladeMesh, grass->bladeMaterial, MatrixIdentity());

    // Restore state
    rlEnableBackfaceCulling();
}

void CleanupGrassSystem(GrassSystem* grass) {
    if (!grass->initialized) return;

    UnloadMesh(grass->bladeMesh);
    UnloadShader(grass->bladeShader);
    // transforms is NULL with baked mesh approach
    grass->initialized = false;
}

void CleanupGameResources(GameResources* res) {
    UnloadModel(res->groundModel);
    UnloadShader(res->grassShader);
    CleanupGrassSystem(&res->grass);

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
    UnloadShader(res->skyShader);
    UnloadModel(res->skyModel);

    // Unload entity primitive models
    if (res->entityModels.initialized) {
        UnloadModel(res->entityModels.cube);
        UnloadModel(res->entityModels.sphere);
        UnloadModel(res->entityModels.cylinder);
        UnloadModel(res->entityModels.firePlane);
        UnloadShader(res->entityModels.fireShader);
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

        // Draw snowflake as small white cube
        Color snowColor = { 255, 255, 255, 200 };
        float size = 0.05f + (float)((i % 3)) * 0.02f;  // Vary size slightly
        DrawCube(p->position, size, size, size, snowColor);
    }
}

// Falling leaves particle system implementation
void InitLeafSystem(LeafSystem* leaves, Vector3 centerPos) {
    for (int i = 0; i < LEAF_PARTICLE_COUNT; i++) {
        // Random position within spawn radius around center
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float dist = (float)GetRandomValue(0, (int)(LEAF_SPAWN_RADIUS * 100)) / 100.0f;
        leaves->particles[i].position.x = centerPos.x + cosf(angle) * dist;
        leaves->particles[i].position.z = centerPos.z + sinf(angle) * dist;
        leaves->particles[i].position.y = centerPos.y + (float)GetRandomValue(0, (int)(LEAF_HEIGHT * 100)) / 100.0f;

        // Random fall speed (slower than snow, leaves flutter)
        leaves->particles[i].fallSpeed = 0.8f + (float)GetRandomValue(0, 150) / 100.0f;
        // Random horizontal drift speed
        leaves->particles[i].driftSpeed = 1.0f + (float)GetRandomValue(0, 200) / 100.0f;
        // Random rotation
        leaves->particles[i].rotation = (float)GetRandomValue(0, 360);
        leaves->particles[i].rotationSpeed = (float)GetRandomValue(-200, 200);
        // Random color type
        leaves->particles[i].colorType = GetRandomValue(0, 2);
    }
    leaves->initialized = true;
}

void UpdateAndDrawLeaves(LeafSystem* leaves, Vector3 centerPos, float deltaTime) {
    if (!leaves->initialized) return;

    float time = (float)GetTime();

    // Leaf colors
    Color leafColors[] = {
        { 180, 45, 30, 220 },   // Red
        { 210, 120, 40, 220 },  // Orange
        { 200, 170, 50, 220 }   // Yellow/gold
    };

    for (int i = 0; i < LEAF_PARTICLE_COUNT; i++) {
        LeafParticle* p = &leaves->particles[i];

        // Fall down with slight oscillation
        p->position.y -= p->fallSpeed * deltaTime;

        // Flutter side-to-side (more than snow)
        float flutter = sinf(time * 3.0f + p->rotation * 0.01f) * p->driftSpeed * deltaTime;
        p->position.x += flutter;
        p->position.z += cosf(time * 2.5f + p->rotation * 0.02f) * p->driftSpeed * 0.7f * deltaTime;

        // Rotate the leaf
        p->rotation += p->rotationSpeed * deltaTime;

        // Respawn at top if below ground or too far from player
        float dx = p->position.x - centerPos.x;
        float dz = p->position.z - centerPos.z;
        float distSq = dx * dx + dz * dz;

        if (p->position.y < centerPos.y - 5.0f || distSq > LEAF_SPAWN_RADIUS * LEAF_SPAWN_RADIUS * 1.5f) {
            // Respawn at random position above player
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float dist = (float)GetRandomValue(0, (int)(LEAF_SPAWN_RADIUS * 100)) / 100.0f;
            p->position.x = centerPos.x + cosf(angle) * dist;
            p->position.z = centerPos.z + sinf(angle) * dist;
            p->position.y = centerPos.y + LEAF_HEIGHT;
            p->colorType = GetRandomValue(0, 2);
        }

        // Draw leaf as a small rotated rectangle
        Color leafColor = leafColors[p->colorType];

        // Use DrawCube with slight rotation for a leaf-like appearance
        rlPushMatrix();
        rlTranslatef(p->position.x, p->position.y, p->position.z);
        rlRotatef(p->rotation, 0, 1, 0);
        rlRotatef(p->rotation * 0.5f, 1, 0, 0);
        DrawCube((Vector3){0, 0, 0}, 0.12f, 0.02f, 0.08f, leafColor);
        rlPopMatrix();
    }
}
