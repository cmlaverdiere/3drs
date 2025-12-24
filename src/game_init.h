#ifndef GAME_INIT_H
#define GAME_INIT_H

#include "raylib.h"
#include "types.h"
#include "spatial_hash.h"

// Primitive models for entity rendering (with proper normals)
struct EntityModels {
    Model cube;
    Model sphere;
    Model cylinder;
    bool initialized;
};

// All game resources that need cleanup
struct GameResources {
    // Shaders
    Shader grassShader;
    Shader wallShaders[WALL_MATERIAL_COUNT];
    Shader waterShader;
    Shader sandShader;
    Shader entityShader;  // For lit entities (enemies, trees, items)
    Shader depthShader;   // For shadow map pass
    int waterTimeLoc;

    // Models
    Model groundModel;
    Model wallModels[MAX_WALLS];
    Model waterModels[MAX_WATER];
    Model sandModels[MAX_SAND];

    // Entity primitive models (for DrawModelEx-based rendering)
    EntityModels entityModels;

    // Counts for cleanup
    int wallCount;
    int waterCount;
    int sandCount;
};

// Initialize window and audio
void InitGameWindow(int* screenWidth, int* screenHeight);

// Initialize player state with defaults
void InitPlayerState(PlayerState* state);

// Initialize camera
void InitCamera(Camera3D* camera, const PlayerState* state);

// Load and initialize all game resources (shaders, models)
GameResources LoadGameResources(const MapData& mapData, Wall* walls, Water* waterBodies, Sand* sandZones);

// Initialize heightmap with valleys from map data
void InitializeHeightmap(const MapData& mapData);

// Generate ground mesh from heightmap
Mesh GenHeightmapMesh(float sizeX, float sizeZ, int resX, int resZ);

// Initialize enemies from map data
void InitEnemiesFromMap(Enemy* enemies, int* enemyCount, const MapData& mapData);

// Initialize trees from map data
void InitTreesFromMap(Tree* trees, int* treeCount, const MapData& mapData);

// Initialize world items from map data
void InitItemsFromMap(WorldItem* items, int* itemCount, const MapData& mapData, bool swordPickedUp);

// Populate spatial hash with walls, enemies, trees
void PopulateSpatialHash(WorldSpatialData* spatial, const Wall* walls, int wallCount,
                         const Enemy* enemies, int enemyCount,
                         const Tree* trees, int treeCount);

// Cleanup all game resources
void CleanupGameResources(GameResources* res);

#endif
