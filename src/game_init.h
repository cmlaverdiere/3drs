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
    Model firePlane;      // Billboard plane for fire shader
    Shader fireShader;    // Procedural fire shader
    int fireTimeLoc;      // Time uniform location
    bool initialized;
};

// Grass blade system (baked mesh = 1 draw call, very efficient)
static const int GRASS_BLADE_COUNT = 20000;  // Dense coverage, single draw call
static const float GRASS_SPAWN_RADIUS = 50.0f;  // Grass spawns within this radius of origin

struct GrassSystem {
    Mesh bladeMesh;
    Material bladeMaterial;
    Shader bladeShader;
    Matrix* transforms;
    int bladeCount;
    int timeLoc;
    bool initialized;
};

// All game resources that need cleanup
struct GameResources {
    // Shaders
    Shader grassShader;  // Also handles sand zones
    Shader wallShaders[WALL_MATERIAL_COUNT];
    Shader waterShader;
    Shader entityShader;  // For lit entities (enemies, trees, items)
    Shader depthShader;   // For shadow map pass
    Shader skyShader;     // For sky rendering
    int waterTimeLoc;

    // Sky
    Model skyModel;

    // Models
    Model groundModel;
    Model wallModels[MAX_WALLS];
    Model waterModels[MAX_WATER];

    // Entity primitive models (for DrawModelEx-based rendering)
    EntityModels entityModels;

    // Grass blade system
    GrassSystem grass;

    // Counts for cleanup
    int wallCount;
    int waterCount;
    int sandCount;  // Still tracked for spatial queries
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

// Initialize rocks from map data
void InitRocksFromMap(Rock* rocks, int* rockCount, const MapData& mapData);

// Initialize world items from map data
void InitItemsFromMap(WorldItem* items, int* itemCount, const MapData& mapData, bool swordPickedUp);

// Populate spatial hash with walls, enemies, trees
void PopulateSpatialHash(WorldSpatialData* spatial, const Wall* walls, int wallCount,
                         const Enemy* enemies, int enemyCount,
                         const Tree* trees, int treeCount);

// Cleanup all game resources
void CleanupGameResources(GameResources* res);

// Initialize grass blade system
void InitGrassSystem(GrassSystem* grass);

// Draw grass blades (call after terrain, before transparent objects)
void DrawGrassBlades(GrassSystem* grass, float time);

// Cleanup grass system
void CleanupGrassSystem(GrassSystem* grass);

// Snow particle system (for winter mode)
static const int SNOW_PARTICLE_COUNT = 2000;
static const float SNOW_SPAWN_RADIUS = 40.0f;
static const float SNOW_HEIGHT = 30.0f;

struct SnowParticle {
    Vector3 position;
    float speed;
    float wobble;  // Side-to-side drift phase
};

struct SnowSystem {
    SnowParticle particles[SNOW_PARTICLE_COUNT];
    bool initialized;
};

// Initialize snow system
void InitSnowSystem(SnowSystem* snow, Vector3 centerPos);

// Update and draw snow particles
void UpdateAndDrawSnow(SnowSystem* snow, Vector3 centerPos, float deltaTime);

// Falling leaves particle system (for autumn mode)
static const int LEAF_PARTICLE_COUNT = 800;
static const float LEAF_SPAWN_RADIUS = 50.0f;
static const float LEAF_HEIGHT = 25.0f;

struct LeafParticle {
    Vector3 position;
    float fallSpeed;
    float driftSpeed;
    float rotation;
    float rotationSpeed;
    int colorType;  // 0=red, 1=orange, 2=yellow
};

struct LeafSystem {
    LeafParticle particles[LEAF_PARTICLE_COUNT];
    bool initialized;
};

// Initialize leaf system
void InitLeafSystem(LeafSystem* leaves, Vector3 centerPos);

// Update and draw falling leaves
void UpdateAndDrawLeaves(LeafSystem* leaves, Vector3 centerPos, float deltaTime);

#endif
