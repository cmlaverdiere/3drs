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

    // Grass/ground shader and ground model (uses #include for common lighting)
    // Ground must be large enough to cover all regions:
    // - Wilderness: X -650 to -50, Z -300 to +300
    // - Varrock: Z offset -200
    // - Al Kharid: X offset +100, Z offset +50
    // Total span: ~1600x1600 units centered at origin
    // NOTE: Resolution capped at 255x255 because Mesh.indices uses unsigned short (max 65535)
    res.grassShader = LoadShaderWithIncludes("shaders/grass.vs", "shaders/grass.fs");
    Mesh groundMesh = GenHeightmapMesh(1600.0f, 1600.0f, 255, 255);
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
    res.waterShader = LoadShaderWithIncludes("shaders/water.vs", "shaders/water.fs");
    res.waterTimeLoc = GetShaderLocation(res.waterShader, "time");

    // Entity shader for lit enemies/trees/items
    res.entityShader = LoadShaderWithIncludes("shaders/entity.vs", "shaders/entity.fs");

    // Depth shader for shadow map pass
    res.depthShader = LoadShader("shaders/depth.vs", "shaders/depth.fs");

    // Sky shader and model
    res.skyShader = LoadShader("shaders/sky.vs", "shaders/sky.fs");
    Mesh skyMesh = GenMeshSphere(1000.0f, 32, 32);  // Large sphere around scene (covers 1600x1600 ground)
    res.skyModel = LoadModelFromMesh(skyMesh);
    res.skyModel.materials[0].shader = res.skyShader;

    // Create primitive models for entity rendering (with proper normals)
    // Store entity shader reference in EntityModels for restoration after monster shader
    res.entityModels.entityShader = res.entityShader;
    res.entityModels.depthShader = res.depthShader;

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

    // Monster shader for procedural textures (scales, fur, stone, etc.)
    res.entityModels.monsterShader = LoadShaderWithIncludes("shaders/monster.vs", "shaders/monster.fs");
    res.entityModels.monsterMaterialLoc = GetShaderLocation(res.entityModels.monsterShader, "materialType");
    res.entityModels.monsterSeedLoc = GetShaderLocation(res.entityModels.monsterShader, "monsterSeed");

    // Tree shaders - foliage (leafy canopy) and wood (bark texture)
    // Use entity.vs for both since they have the same interface
    res.entityModels.foliageShader = LoadShaderWithIncludes("shaders/entity.vs", "shaders/foliage.fs");
    res.entityModels.woodShader = LoadShaderWithIncludes("shaders/entity.vs", "shaders/wood.fs");

    // Foliage sphere - for tree canopy
    Mesh foliageSphereMesh = GenMeshSphere(1.0f, 16, 16);
    res.entityModels.foliageSphere = LoadModelFromMesh(foliageSphereMesh);
    res.entityModels.foliageSphere.materials[0].shader = res.entityModels.foliageShader;

    // Wood cylinder - for tree trunk
    Mesh woodCylinderMesh = GenMeshCylinder(1.0f, 1.0f, 16);
    res.entityModels.woodCylinder = LoadModelFromMesh(woodCylinderMesh);
    res.entityModels.woodCylinder.materials[0].shader = res.entityModels.woodShader;

    res.entityModels.initialized = true;

    // Initialize grass blade system (pass sand/water zones for exclusion)
    InitGrassSystem(&res.grass, sandZones, mapData.sandCount, waterBodies, mapData.waterCount);

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

// Generate a single grass blade mesh (unit size, to be instanced)
static Mesh GenSingleGrassBladeMesh(float bladeWidth, float bladeHeight) {
    Mesh mesh = { 0 };

    mesh.vertexCount = 4;
    mesh.triangleCount = 2;
    mesh.vertices = (float*)RL_MALLOC(4 * 3 * sizeof(float));
    mesh.texcoords = (float*)RL_MALLOC(4 * 2 * sizeof(float));
    mesh.normals = (float*)RL_MALLOC(4 * 3 * sizeof(float));
    mesh.indices = (unsigned short*)RL_MALLOC(6 * sizeof(unsigned short));

    float halfW = bladeWidth * 0.5f;
    float tipW = bladeWidth * 0.15f;

    // Bottom left (base)
    mesh.vertices[0] = -halfW; mesh.vertices[1] = 0.0f; mesh.vertices[2] = 0.0f;
    mesh.texcoords[0] = 0.0f; mesh.texcoords[1] = 0.0f;

    // Bottom right (base)
    mesh.vertices[3] = halfW; mesh.vertices[4] = 0.0f; mesh.vertices[5] = 0.0f;
    mesh.texcoords[2] = 1.0f; mesh.texcoords[3] = 0.0f;

    // Top left (tip)
    mesh.vertices[6] = -tipW; mesh.vertices[7] = bladeHeight; mesh.vertices[8] = 0.0f;
    mesh.texcoords[4] = 0.0f; mesh.texcoords[5] = 1.0f;

    // Top right (tip)
    mesh.vertices[9] = tipW; mesh.vertices[10] = bladeHeight; mesh.vertices[11] = 0.0f;
    mesh.texcoords[6] = 1.0f; mesh.texcoords[7] = 1.0f;

    // Normals (pointing forward, will be rotated by instance transform)
    for (int i = 0; i < 4; i++) {
        mesh.normals[i*3 + 0] = 0.0f;
        mesh.normals[i*3 + 1] = 0.3f;
        mesh.normals[i*3 + 2] = 0.95f;
    }

    // Indices
    mesh.indices[0] = 0; mesh.indices[1] = 1; mesh.indices[2] = 2;
    mesh.indices[3] = 1; mesh.indices[4] = 3; mesh.indices[5] = 2;

    UploadMesh(&mesh, false);
    return mesh;
}

// Check if position is inside a sand zone
static bool IsInSandZone(float x, float z, Sand* sandZones, int sandCount) {
    for (int i = 0; i < sandCount; i++) {
        float halfW = sandZones[i].width * 0.5f;
        float halfL = sandZones[i].length * 0.5f;
        float cx = sandZones[i].position.x;
        float cz = sandZones[i].position.z;
        if (x >= cx - halfW && x <= cx + halfW &&
            z >= cz - halfL && z <= cz + halfL) {
            return true;
        }
    }
    return false;
}

// Check if position is inside a water body
static bool IsInWaterZone(float x, float z, Water* waterBodies, int waterCount) {
    for (int i = 0; i < waterCount; i++) {
        float halfW = waterBodies[i].width * 0.5f;
        float halfL = waterBodies[i].length * 0.5f;
        float cx = waterBodies[i].position.x;
        float cz = waterBodies[i].position.z;
        if (x >= cx - halfW && x <= cx + halfW &&
            z >= cz - halfL && z <= cz + halfL) {
            return true;
        }
    }
    return false;
}

// Generate grass transforms for a chunk
static void GenerateGrassChunk(GrassChunk* chunk, int chunkX, int chunkZ,
                                Sand* sandZones, int sandCount,
                                Water* waterBodies, int waterCount) {
    chunk->chunkX = chunkX;
    chunk->chunkZ = chunkZ;
    chunk->transforms = (Matrix*)RL_MALLOC(GRASS_BLADES_PER_CHUNK * sizeof(Matrix));
    chunk->bladeCount = 0;

    // World position of chunk corner (heightmap centered at origin)
    float worldX = (chunkX * GRASS_CHUNK_SIZE) - HEIGHTMAP_OFFSET;
    float worldZ = (chunkZ * GRASS_CHUNK_SIZE) - HEIGHTMAP_OFFSET;

    // Deterministic seed for this chunk (consistent across runs)
    unsigned int seed = (unsigned int)(chunkX * 73856093 + chunkZ * 19349663);
    srand(seed);

    for (int i = 0; i < GRASS_BLADES_PER_CHUNK; i++) {
        // Random position within chunk
        float localX = ((float)rand() / RAND_MAX) * GRASS_CHUNK_SIZE;
        float localZ = ((float)rand() / RAND_MAX) * GRASS_CHUNK_SIZE;
        float x = worldX + localX;
        float z = worldZ + localZ;

        // Skip grass in excluded zones
        if (IsInSandZone(x, z, sandZones, sandCount)) continue;
        if (IsInWaterZone(x, z, waterBodies, waterCount)) continue;

        float y = GetTerrainHeight(x, z);

        // Random rotation and scale
        float rotY = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float scale = 0.8f + ((float)rand() / RAND_MAX) * 0.4f;

        // Build transform matrix: Scale * RotationY * Translation
        Matrix matScale = MatrixScale(scale, scale, scale);
        Matrix matRot = MatrixRotateY(rotY);
        Matrix matTrans = MatrixTranslate(x, y, z);

        // Combine: first scale, then rotate, then translate
        Matrix transform = MatrixMultiply(matScale, matRot);
        transform = MatrixMultiply(transform, matTrans);

        chunk->transforms[chunk->bladeCount] = transform;
        chunk->bladeCount++;
    }

    chunk->loaded = true;
}

// Find a chunk in cache or return NULL
static GrassChunk* FindChunkInCache(GrassSystem* grass, int chunkX, int chunkZ) {
    for (int i = 0; i < grass->chunkCacheSize; i++) {
        if (grass->chunkCache[i].loaded &&
            grass->chunkCache[i].chunkX == chunkX &&
            grass->chunkCache[i].chunkZ == chunkZ) {
            return &grass->chunkCache[i];
        }
    }
    return NULL;
}

// Get or generate a chunk (uses cache)
static GrassChunk* GetOrGenerateChunk(GrassSystem* grass, int chunkX, int chunkZ) {
    // First check cache
    GrassChunk* existing = FindChunkInCache(grass, chunkX, chunkZ);
    if (existing) return existing;

    // Need to generate new chunk - find a slot
    int slot = -1;

    // First try to find an empty slot
    if (grass->chunkCacheSize < GRASS_CHUNK_CACHE_CAPACITY) {
        slot = grass->chunkCacheSize;
        grass->chunkCacheSize++;
    } else {
        // Cache is full, find LRU slot (simple: just use slot 0 and shift)
        // For simplicity, just overwrite a random old chunk
        slot = rand() % GRASS_CHUNK_CACHE_CAPACITY;
        // Free old chunk's transforms
        if (grass->chunkCache[slot].transforms) {
            RL_FREE(grass->chunkCache[slot].transforms);
            grass->chunkCache[slot].transforms = NULL;
        }
    }

    // Generate new chunk
    GenerateGrassChunk(&grass->chunkCache[slot], chunkX, chunkZ,
                       grass->sandZones, grass->sandCount,
                       grass->waterBodies, grass->waterCount);

    return &grass->chunkCache[slot];
}

void InitGrassSystem(GrassSystem* grass, Sand* sandZones, int sandCount, Water* waterBodies, int waterCount) {
    // Load instanced grass blade shader
    grass->bladeShader = LoadShaderWithIncludes("shaders/grass_blade_instanced.vs", "shaders/grass_blade.fs");

    // CRITICAL: Bind instanceTransform as a vertex attribute for instancing
    grass->bladeShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(grass->bladeShader, "instanceTransform");

    grass->timeLoc = GetShaderLocation(grass->bladeShader, "time");

    // Pass season to blade shader (0=Spring, 1=Summer, 2=Autumn, 3=Winter)
    int seasonLoc = GetShaderLocation(grass->bladeShader, "season");
    int seasonVal = (int)g_currentSeason;
    SetShaderValue(grass->bladeShader, seasonLoc, &seasonVal, SHADER_UNIFORM_INT);

    // Generate single blade mesh (to be instanced)
    grass->bladeMesh = GenSingleGrassBladeMesh(0.12f, 0.22f);

    // Setup material
    grass->bladeMaterial = LoadMaterialDefault();
    grass->bladeMaterial.shader = grass->bladeShader;

    // Allocate instance buffer for visible blades
    grass->visibleTransforms = (Matrix*)RL_MALLOC(GRASS_MAX_BLADES * sizeof(Matrix));
    grass->visibleBladeCount = 0;

    // Allocate chunk cache
    grass->chunkCache = (GrassChunk*)RL_CALLOC(GRASS_CHUNK_CACHE_CAPACITY, sizeof(GrassChunk));
    grass->chunkCacheSize = 0;

    // Store exclusion zone references
    grass->sandZones = sandZones;
    grass->sandCount = sandCount;
    grass->waterBodies = waterBodies;
    grass->waterCount = waterCount;

    // Initialize player tracking to force first update
    grass->lastPlayerChunkX = INT_MIN;
    grass->lastPlayerChunkZ = INT_MIN;

    grass->initialized = true;
    TraceLog(LOG_INFO, "Instanced grass system initialized (max %d blades, %d chunk cache)",
             GRASS_MAX_BLADES, GRASS_CHUNK_CACHE_CAPACITY);
}

void UpdateGrassSystem(GrassSystem* grass, Vector3 playerPos) {
    if (!grass->initialized) return;

    // Calculate player's current chunk coordinates
    int playerChunkX = (int)floorf((playerPos.x + HEIGHTMAP_OFFSET) / GRASS_CHUNK_SIZE);
    int playerChunkZ = (int)floorf((playerPos.z + HEIGHTMAP_OFFSET) / GRASS_CHUNK_SIZE);

    // Only rebuild visible transforms if player moved to a new chunk
    if (playerChunkX == grass->lastPlayerChunkX && playerChunkZ == grass->lastPlayerChunkZ) {
        return;
    }

    grass->lastPlayerChunkX = playerChunkX;
    grass->lastPlayerChunkZ = playerChunkZ;

    // Determine visible chunk range
    int minCX = playerChunkX - GRASS_VISIBLE_RADIUS;
    int maxCX = playerChunkX + GRASS_VISIBLE_RADIUS;
    int minCZ = playerChunkZ - GRASS_VISIBLE_RADIUS;
    int maxCZ = playerChunkZ + GRASS_VISIBLE_RADIUS;

    // Clamp to world bounds (0 to GRASS_CHUNKS_PER_SIDE-1)
    if (minCX < 0) minCX = 0;
    if (maxCX >= GRASS_CHUNKS_PER_SIDE) maxCX = GRASS_CHUNKS_PER_SIDE - 1;
    if (minCZ < 0) minCZ = 0;
    if (maxCZ >= GRASS_CHUNKS_PER_SIDE) maxCZ = GRASS_CHUNKS_PER_SIDE - 1;

    // Collect visible transforms from all visible chunks
    grass->visibleBladeCount = 0;

    for (int cz = minCZ; cz <= maxCZ; cz++) {
        for (int cx = minCX; cx <= maxCX; cx++) {
            // Get or generate chunk
            GrassChunk* chunk = GetOrGenerateChunk(grass, cx, cz);
            if (!chunk || !chunk->loaded) continue;

            // Copy transforms to visible buffer
            int copyCount = chunk->bladeCount;
            if (grass->visibleBladeCount + copyCount > GRASS_MAX_BLADES) {
                copyCount = GRASS_MAX_BLADES - grass->visibleBladeCount;
            }

            if (copyCount > 0) {
                memcpy(&grass->visibleTransforms[grass->visibleBladeCount],
                       chunk->transforms,
                       copyCount * sizeof(Matrix));
                grass->visibleBladeCount += copyCount;
            }
        }
    }
}

void DrawGrassBlades(GrassSystem* grass, float time) {
    if (!grass->initialized || grass->visibleBladeCount == 0) return;

    // Update time uniform for wind animation
    SetShaderValue(grass->bladeShader, grass->timeLoc, &time, SHADER_UNIFORM_FLOAT);

    // Disable backface culling so grass is visible from both sides
    rlDisableBackfaceCulling();

    // Single instanced draw call for all visible grass
    DrawMeshInstanced(grass->bladeMesh, grass->bladeMaterial,
                      grass->visibleTransforms, grass->visibleBladeCount);

    // Restore state
    rlEnableBackfaceCulling();
}

void CleanupGrassSystem(GrassSystem* grass) {
    if (!grass->initialized) return;

    UnloadMesh(grass->bladeMesh);
    UnloadShader(grass->bladeShader);

    // Free visible transforms buffer
    if (grass->visibleTransforms) {
        RL_FREE(grass->visibleTransforms);
        grass->visibleTransforms = NULL;
    }

    // Free chunk cache and all chunk transforms
    if (grass->chunkCache) {
        for (int i = 0; i < grass->chunkCacheSize; i++) {
            if (grass->chunkCache[i].transforms) {
                RL_FREE(grass->chunkCache[i].transforms);
            }
        }
        RL_FREE(grass->chunkCache);
        grass->chunkCache = NULL;
    }

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
        UnloadShader(res->entityModels.monsterShader);
        // Tree models and shaders
        UnloadModel(res->entityModels.foliageSphere);
        UnloadModel(res->entityModels.woodCylinder);
        UnloadShader(res->entityModels.foliageShader);
        UnloadShader(res->entityModels.woodShader);
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
    // Load leaf shader
    leaves->leafShader = LoadShader("shaders/leaf.vs", "shaders/leaf.fs");
    leaves->viewPosLoc = GetShaderLocation(leaves->leafShader, "viewPos");
    leaves->fogColorLoc = GetShaderLocation(leaves->leafShader, "fogColor");
    leaves->fogDensityLoc = GetShaderLocation(leaves->leafShader, "fogDensity");

    // Create a simple quad mesh for leaves
    leaves->leafMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);

    // Setup material with shader
    leaves->leafMaterial = LoadMaterialDefault();
    leaves->leafMaterial.shader = leaves->leafShader;

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

    // Set shader uniforms
    float viewPosArr[3] = { viewPos.x, viewPos.y, viewPos.z };
    float fogColorArr[3] = { fogColor.x, fogColor.y, fogColor.z };
    SetShaderValue(leaves->leafShader, leaves->viewPosLoc, viewPosArr, SHADER_UNIFORM_VEC3);
    SetShaderValue(leaves->leafShader, leaves->fogColorLoc, fogColorArr, SHADER_UNIFORM_VEC3);
    SetShaderValue(leaves->leafShader, leaves->fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);

    // Leaf colors (RGB normalized)
    Color leafColors[] = {
        { 180, 45, 30, 255 },   // Red
        { 210, 120, 40, 255 },  // Orange
        { 200, 170, 50, 255 }   // Yellow/gold
    };

    // Disable backface culling for double-sided leaves
    rlDisableBackfaceCulling();

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

        // Set leaf color
        leaves->leafMaterial.maps[MATERIAL_MAP_DIFFUSE].color = leafColors[p->colorType];

        // Build transform matrix for this leaf
        Matrix matTranslate = MatrixTranslate(p->position.x, p->position.y, p->position.z);
        Matrix matRotateY = MatrixRotateY(p->rotationY * DEG2RAD);
        Matrix matRotateX = MatrixRotateX(p->rotationTumble * DEG2RAD);
        Matrix matScale = MatrixScale(p->size, p->size, p->size);
        // Rotate to make plane vertical (plane is horizontal by default)
        Matrix matRotateToVertical = MatrixRotateX(90.0f * DEG2RAD);

        Matrix transform = MatrixMultiply(matScale, matRotateToVertical);
        transform = MatrixMultiply(transform, matRotateX);
        transform = MatrixMultiply(transform, matRotateY);
        transform = MatrixMultiply(transform, matTranslate);

        // Draw the leaf
        DrawMesh(leaves->leafMesh, leaves->leafMaterial, transform);
    }

    rlEnableBackfaceCulling();
}

void CleanupLeafSystem(LeafSystem* leaves) {
    if (!leaves->initialized) return;
    UnloadShader(leaves->leafShader);
    UnloadMesh(leaves->leafMesh);
    leaves->initialized = false;
}

// ============================================================================
// Leaf Burst Particle System (for tree chopping in autumn)
// ============================================================================

void InitLeafBurstSystem(LeafBurstSystem* system) {
    // Load leaf shader
    system->leafShader = LoadShader("shaders/leaf.vs", "shaders/leaf.fs");
    system->viewPosLoc = GetShaderLocation(system->leafShader, "viewPos");
    system->fogColorLoc = GetShaderLocation(system->leafShader, "fogColor");
    system->fogDensityLoc = GetShaderLocation(system->leafShader, "fogDensity");

    // Create a simple quad mesh for leaves
    system->leafMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);

    // Setup material with shader
    system->leafMaterial = LoadMaterialDefault();
    system->leafMaterial.shader = system->leafShader;

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

    // Set shader uniforms
    float viewPosArr[3] = { viewPos.x, viewPos.y, viewPos.z };
    float fogColorArr[3] = { fogColor.x, fogColor.y, fogColor.z };
    SetShaderValue(system->leafShader, system->viewPosLoc, viewPosArr, SHADER_UNIFORM_VEC3);
    SetShaderValue(system->leafShader, system->fogColorLoc, fogColorArr, SHADER_UNIFORM_VEC3);
    SetShaderValue(system->leafShader, system->fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);

    // Leaf colors (RGB normalized)
    Color leafColors[] = {
        { 180, 45, 30, 255 },   // Red
        { 210, 120, 40, 255 },  // Orange
        { 200, 170, 50, 255 }   // Yellow/gold
    };

    const float gravity = 3.5f;
    const float drag = 0.5f;

    // Disable backface culling for double-sided leaves
    rlDisableBackfaceCulling();

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

            // Set leaf color with fade
            Color c = leafColors[p->colorType];
            c.a = (unsigned char)(255 * alpha);
            system->leafMaterial.maps[MATERIAL_MAP_DIFFUSE].color = c;

            // Build transform matrix for this leaf
            Matrix matTranslate = MatrixTranslate(p->position.x, p->position.y, p->position.z);
            Matrix matRotateY = MatrixRotateY(p->rotationY * DEG2RAD);
            Matrix matRotateX = MatrixRotateX(p->rotationTumble * DEG2RAD);
            Matrix matScale = MatrixScale(p->size, p->size, p->size);
            // Rotate to make plane vertical (plane is horizontal by default)
            Matrix matRotateToVertical = MatrixRotateX(90.0f * DEG2RAD);

            Matrix transform = MatrixMultiply(matScale, matRotateToVertical);
            transform = MatrixMultiply(transform, matRotateX);
            transform = MatrixMultiply(transform, matRotateY);
            transform = MatrixMultiply(transform, matTranslate);

            // Draw the leaf
            DrawMesh(system->leafMesh, system->leafMaterial, transform);
        }
    }

    rlEnableBackfaceCulling();
}

void CleanupLeafBurstSystem(LeafBurstSystem* system) {
    if (!system->initialized) return;
    UnloadShader(system->leafShader);
    UnloadMesh(system->leafMesh);
    system->initialized = false;
}

// ============================================================================
// Blood Splatter Particle System
// ============================================================================

void InitBloodSplatterSystem(BloodSplatterSystem* system) {
    // Load blood shader
    system->bloodShader = LoadShader("shaders/blood.vs", "shaders/blood.fs");
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
