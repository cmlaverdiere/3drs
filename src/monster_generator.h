#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include "types.h"

// Maximum sizes
constexpr int GEN_INPUT_MAX = 512;
constexpr int GEN_RESPONSE_MAX = 8192;
constexpr int MAX_MONSTER_EGGS = 8;

// Generator UI state machine
enum class GeneratorState {
    CLOSED,           // UI not visible
    TYPING,           // Player typing description
    GENERATING,       // API request in flight (egg spawned)
    ERROR             // Error state, show message
};

// Monster egg (pending generation)
struct MonsterEgg {
    Vector3 position;
    float timer;              // Time since spawn (for wobble animation)
    bool hatching;            // True when generation complete
    float hatchTimer;         // Hatch animation progress (0.0 to 1.0)
    int customMonsterIndex;   // Index to spawn when hatch complete (-1 if pending)
    bool active;
};

// Thread-safe monster generator state
struct MonsterGenerator {
    // UI state
    GeneratorState state;
    char inputBuffer[GEN_INPUT_MAX];
    int inputLength;
    int cursorPos;

    // List navigation
    int listScrollOffset;
    int selectedIndex;       // -1 if none selected

    // Generated result (pending)
    CustomMonster generatedMonster;
    bool generationReady;

    // Error handling
    bool hasError;
    char errorMessage[256];

    // Response storage (thread fills this)
    char response[GEN_RESPONSE_MAX];
    int responseLength;

    // Threading
    std::thread* workerThread;
    std::atomic<bool>* requestInFlight;
    std::mutex* responseMutex;

    // Monster eggs (pending generations in world)
    MonsterEgg eggs[MAX_MONSTER_EGGS];
    int activeEggIndex;      // Which egg is currently generating (-1 if none)
};

// Initialize the generator system
void InitMonsterGenerator(MonsterGenerator* gen);

// Shutdown and cleanup
void ShutdownMonsterGenerator(MonsterGenerator* gen);

// Open the generator UI (call when 'G' pressed)
void OpenMonsterGenerator(MonsterGenerator* gen);

// Close the generator UI
void CloseMonsterGenerator(MonsterGenerator* gen);

// Update generator (call every frame)
// Returns true if a monster finished generating and needs to be spawned
bool UpdateMonsterGenerator(MonsterGenerator* gen, CustomMonster* customMonsters,
                            int* customMonsterCount, int maxMonsters,
                            Enemy* enemies, int* enemyCount, int maxEnemies,
                            float deltaTime);

// Handle mouse input for generator UI (call every frame when UI is open)
// Returns true if UI should close
bool HandleMonsterGeneratorInput(MonsterGenerator* gen,
                                  CustomMonster* customMonsters, int* customMonsterCount, int maxMonsters,
                                  Enemy* enemies, int* enemyCount, int maxEnemies,
                                  const Camera3D* camera,
                                  int screenWidth, int screenHeight);

// Draw generator UI
void DrawMonsterGenerator(const MonsterGenerator* gen,
                          const CustomMonster* customMonsters, int customMonsterCount,
                          int screenWidth, int screenHeight);

// Draw monster eggs in the world
void DrawMonsterEggs(const struct EntityModels* models, const MonsterGenerator* gen);

// Start async monster generation
// Spawns egg at position, starts LLM request
void StartMonsterGeneration(MonsterGenerator* gen, const char* description, Vector3 spawnPos);

// Check if generator UI is open
bool IsGeneratorOpen(const MonsterGenerator* gen);

// Spawn a monster from the list at position
void SpawnMonsterFromList(MonsterGenerator* gen, int monsterIndex,
                          const CustomMonster* customMonsters, int customMonsterCount,
                          Enemy* enemies, int* enemyCount, int maxEnemies,
                          Vector3 position);
