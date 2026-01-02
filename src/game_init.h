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

    // Entity shader (default for entities)
    Shader entityShader;

    // Monster shader for procedural textures
    Shader monsterShader;
    int monsterMaterialLoc;
    int monsterSeedLoc;

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
    float rotationY;      // Rotation around vertical axis
    float rotationTumble; // Tumbling rotation
    float rotationSpeed;
    float tumbleSpeed;
    float size;           // Leaf size variation
    int colorType;        // 0=red, 1=orange, 2=yellow
};

struct LeafSystem {
    LeafParticle particles[LEAF_PARTICLE_COUNT];
    Shader leafShader;
    Mesh leafMesh;
    Material leafMaterial;
    int viewPosLoc;
    int fogColorLoc;
    int fogDensityLoc;
    bool initialized;
};

// Initialize leaf system
void InitLeafSystem(LeafSystem* leaves, Vector3 centerPos);

// Update and draw falling leaves (needs lighting info for fog)
void UpdateAndDrawLeaves(LeafSystem* leaves, Vector3 centerPos, Vector3 viewPos,
                         Vector3 fogColor, float fogDensity, float deltaTime);

// Cleanup leaf system
void CleanupLeafSystem(LeafSystem* leaves);

// ============================================================================
// Leaf Burst Particle System (for tree chopping in autumn)
// ============================================================================

static const int LEAF_BURST_PARTICLE_COUNT = 250;  // Particles per burst (many leaves!)
static const int MAX_LEAF_BURSTS = 8;              // Max simultaneous bursts
static const float LEAF_BURST_LIFETIME = 4.0f;     // How long burst lasts

struct LeafBurstParticle {
    Vector3 position;
    Vector3 velocity;
    float rotationY;
    float rotationTumble;
    float rotationSpeed;
    float tumbleSpeed;
    float size;
    int colorType;  // 0=red, 1=orange, 2=yellow
    bool active;
};

struct LeafBurst {
    LeafBurstParticle particles[LEAF_BURST_PARTICLE_COUNT];
    float lifetime;
    bool active;
};

struct LeafBurstSystem {
    LeafBurst bursts[MAX_LEAF_BURSTS];
    Shader leafShader;
    Mesh leafMesh;
    Material leafMaterial;
    int viewPosLoc;
    int fogColorLoc;
    int fogDensityLoc;
    bool initialized;
};

// Initialize leaf burst system (shares shader with LeafSystem if already loaded)
void InitLeafBurstSystem(LeafBurstSystem* system);

// Spawn a new leaf burst at position (e.g., tree location)
void SpawnLeafBurst(LeafBurstSystem* system, Vector3 position, float treeHeight);

// Update and draw all active leaf bursts
void UpdateAndDrawLeafBursts(LeafBurstSystem* system, Vector3 viewPos,
                              Vector3 fogColor, float fogDensity, float deltaTime);

// Cleanup
void CleanupLeafBurstSystem(LeafBurstSystem* system);

// ============================================================================
// Blood Splatter Particle System (for combat hits)
// ============================================================================

static const int BLOOD_PARTICLE_COUNT = 20;       // Particles per splatter
static const int MAX_BLOOD_SPLATTERS = 16;        // Max simultaneous splatters
static const float BLOOD_SPLATTER_LIFETIME = 1.5f; // How long splatter lasts

struct BloodParticle {
    Vector3 position;
    Vector3 velocity;
    float size;
    float rotation;      // Y-axis rotation to face velocity direction
    float stretchFactor; // How much to stretch based on speed
    bool active;
};

struct BloodSplatter {
    BloodParticle particles[BLOOD_PARTICLE_COUNT];
    float lifetime;
    bool active;
};

struct BloodSplatterSystem {
    BloodSplatter splatters[MAX_BLOOD_SPLATTERS];
    Shader bloodShader;
    Mesh bloodMesh;
    Material bloodMaterial;
    int viewPosLoc;
    int stretchLoc;
    bool initialized;
};

// Initialize blood splatter system
void InitBloodSplatterSystem(BloodSplatterSystem* system);

// Spawn a new blood splatter at position (e.g., enemy hit location)
void SpawnBloodSplatter(BloodSplatterSystem* system, Vector3 position, Vector3 hitDirection);

// Update and draw all active blood splatters
void UpdateAndDrawBloodSplatters(BloodSplatterSystem* system, Vector3 viewPos, float deltaTime);

// Cleanup blood splatter system
void CleanupBloodSplatterSystem(BloodSplatterSystem* system);

#endif
