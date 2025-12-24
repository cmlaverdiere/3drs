#include "game_init.h"
#include "math_utils.h"
#include "enemy_ai.h"
#include "sound_system.h"
#include "xp_system.h"

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

    InitAudioDevice();
    InitSoundSystem();
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
    state->skillXP[SKILL_HITPOINTS] = XP_TABLE[9];

    for (int i = 0; i < INV_SLOTS; i++) {
        state->inventory[i] = ITEM_NONE;
        state->inventoryCount[i] = 0;
    }

    state->equippedWeapon = ITEM_NONE;
    state->swordPickedUp = false;
    state->maxHP = 10;
    state->currentHP = 10;
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

    // Grass shader and ground model
    res.grassShader = LoadShader("shaders/grass.vs", "shaders/grass.fs");
    Mesh groundMesh = GenHeightmapMesh(512.0f, 512.0f, 256, 256);
    res.groundModel = LoadModelFromMesh(groundMesh);
    res.groundModel.materials[0].shader = res.grassShader;

    // Wall shaders
    res.wallShaders[WALL_WOOD] = LoadShader("shaders/wall.vs", "shaders/wood.fs");
    res.wallShaders[WALL_STONE] = LoadShader("shaders/wall.vs", "shaders/stone.fs");
    res.wallShaders[WALL_BRICK] = LoadShader("shaders/wall.vs", "shaders/brick.fs");

    // Water shader
    res.waterShader = LoadShader("shaders/water.vs", "shaders/water.fs");
    res.waterTimeLoc = GetShaderLocation(res.waterShader, "time");

    // Sand shader
    res.sandShader = LoadShader("shaders/grass.vs", "shaders/sand.fs");

    // Entity shader for lit enemies/trees/items
    res.entityShader = LoadShader("shaders/entity.vs", "shaders/entity.fs");

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

    res.entityModels.initialized = true;

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

    // Create sand models
    res.sandCount = mapData.sandCount;
    for (int i = 0; i < res.sandCount; i++) {
        sandZones[i] = mapData.sandZones[i];
        Mesh sandMesh = GenMeshPlane(sandZones[i].width, sandZones[i].length, 20, 20);
        res.sandModels[i] = LoadModelFromMesh(sandMesh);
        res.sandModels[i].materials[0].shader = res.sandShader;
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
                if (valley.axis == 0) {
                    dist = fabsf(worldX - valley.position);
                } else {
                    dist = fabsf(worldZ - valley.position);
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
        trees[i].health = TREE_MAX_HEALTH;
        trees[i].alive = true;
        trees[i].respawnTimer = 0.0f;
    }
}

void InitItemsFromMap(WorldItem* items, int* itemCount, const MapData& mapData, bool swordPickedUp) {
    *itemCount = mapData.itemCount;
    for (int i = 0; i < *itemCount; i++) {
        items[i].type = mapData.itemTypes[i];
        items[i].position = mapData.itemSpawns[i];
        items[i].pickedUp = false;
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
    UnloadModel(res->groundModel);
    UnloadShader(res->grassShader);

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

    for (int i = 0; i < res->sandCount; i++) {
        UnloadModel(res->sandModels[i]);
    }
    UnloadShader(res->sandShader);

    UnloadShader(res->entityShader);
    UnloadShader(res->depthShader);
    UnloadShader(res->skyShader);
    UnloadModel(res->skyModel);

    // Unload entity primitive models
    if (res->entityModels.initialized) {
        UnloadModel(res->entityModels.cube);
        UnloadModel(res->entityModels.sphere);
        UnloadModel(res->entityModels.cylinder);
    }

    UnloadSoundSystem();
    CloseAudioDevice();
}
