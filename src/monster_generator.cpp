#include "monster_generator.h"
#include "monster_system.h"
#include "rendering.h"
#include "game_init.h"
#include "math_utils.h"
#include "sound_system.h"
#include "rlgl.h"
#include "raymath.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>

// Egg animation constants
constexpr float EGG_WOBBLE_SPEED = 3.0f;
constexpr float EGG_WOBBLE_ANGLE = 5.0f;  // degrees
constexpr float HATCH_DURATION = 1.0f;

// UI constants
constexpr int UI_WIDTH = 400;
constexpr int UI_HEIGHT = 450;
constexpr int INPUT_HEIGHT = 30;
constexpr int BUTTON_HEIGHT = 30;
constexpr int LIST_ITEM_HEIGHT = 25;
constexpr int LIST_VISIBLE_ITEMS = 8;

// Parchment colors (matching other menus)
static const Color PARCHMENT_BG = { 222, 198, 158, 240 };
static const Color PARCHMENT_BORDER = { 139, 90, 43, 255 };
static const Color PARCHMENT_DARK = { 180, 150, 100, 255 };
static const Color PARCHMENT_TEXT = { 60, 40, 20, 255 };
static const Color PARCHMENT_INPUT_BG = { 200, 180, 140, 255 };
static const Color PARCHMENT_BUTTON = { 160, 130, 90, 255 };
static const Color PARCHMENT_BUTTON_HOVER = { 180, 150, 110, 255 };
static const Color PARCHMENT_SELECTED = { 180, 160, 120, 255 };
static const Color EGG_COLOR = { 245, 235, 220, 255 };
static const Color EGG_SPOT_COLOR = { 200, 180, 160, 255 };

// Curl write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    char* buffer = (char*)userp;
    size_t currentLen = strlen(buffer);
    if (currentLen + realsize < GEN_RESPONSE_MAX - 1) {
        memcpy(buffer + currentLen, contents, realsize);
        buffer[currentLen + realsize] = '\0';
    }
    return realsize;
}

// Parse LLM response into CustomMonster
static bool ParseMonsterResponse(const char* response, CustomMonster* monster, char* errorDetail, size_t errorSize) {
    // Debug: print raw response
    printf("[MonsterGen] Raw API response (first 1000 chars):\n%.1000s\n", response);

    // Check for API error message
    const char* errorKey = "\"error\":";
    if (strstr(response, errorKey)) {
        const char* msgKey = "\"message\":";
        const char* msgStart = strstr(response, msgKey);
        if (msgStart) {
            msgStart = strchr(msgStart, '"');
            if (msgStart) {
                msgStart++;
                const char* msgEnd = strchr(msgStart, '"');
                if (msgEnd && errorDetail) {
                    int len = (msgEnd - msgStart < (int)errorSize - 1) ? (msgEnd - msgStart) : (int)errorSize - 1;
                    strncpy(errorDetail, msgStart, len);
                    errorDetail[len] = '\0';
                }
            }
        }
        printf("[MonsterGen] API returned error in response body\n");
        return false;
    }

    // Find the "text" field in JSON response (the one with actual content, not "type":"text")
    // We need to find "text":" pattern where the value is a string starting with content
    const char* textKey = "\"text\":\"";
    const char* textStart = strstr(response, textKey);
    if (!textStart) {
        printf("[MonsterGen] ERROR: Could not find \"text\":\" key in response\n");
        if (errorDetail) snprintf(errorDetail, errorSize, "No 'text' field in API response");
        return false;
    }

    textStart += strlen(textKey);  // Skip past "text":" to the actual content

    // Extract text content (unescape JSON)
    char content[4096];
    int ci = 0;
    while (*textStart && ci < (int)sizeof(content) - 1) {
        if (*textStart == '"' && *(textStart - 1) != '\\') break;
        if (*textStart == '\\' && *(textStart + 1) == 'n') {
            content[ci++] = '\n';
            textStart += 2;
        } else if (*textStart == '\\' && *(textStart + 1) == '"') {
            content[ci++] = '"';
            textStart += 2;
        } else if (*textStart == '\\' && *(textStart + 1) == '\\') {
            content[ci++] = '\\';
            textStart += 2;
        } else {
            content[ci++] = *textStart++;
        }
    }
    content[ci] = '\0';

    // Debug: print extracted text content
    printf("[MonsterGen] Extracted content from 'text' field:\n%s\n", content);
    printf("[MonsterGen] ---END CONTENT---\n");

    // Parse content line by line
    memset(monster, 0, sizeof(CustomMonster));
    monster->level = 5;
    monster->maxHealth = 20;
    monster->maxDamage = 3;
    monster->attackCooldown = 1.0f;
    monster->chaseSpeed = 3.0f;
    monster->attackRange = 2.0f;
    monster->bodyColor = {150, 150, 150, 255};
    monster->limbColor = {100, 100, 100, 255};

    bool inVisual = false;
    char* line = strtok(content, "\n");
    while (line) {
        // Skip whitespace
        while (*line == ' ' || *line == '\t') line++;

        if (inVisual) {
            if (line[0] == '.') {
                inVisual = false;
            } else if (monster->primitiveCount < MAX_MONSTER_PRIMITIVES) {
                MonsterPrimitive* prim = &monster->primitives[monster->primitiveCount];
                char typeStr[32];
                float x, y, z, w, h = 0, d = 0;
                int r, g, b, a;

                // Try sphere with color
                if (sscanf(line, "%31s %f %f %f %f %d %d %d %d",
                          typeStr, &x, &y, &z, &w, &r, &g, &b, &a) >= 9 &&
                    strcmp(typeStr, "sphere") == 0) {
                    prim->type = PRIM_SPHERE;
                    prim->x = x; prim->y = y; prim->z = z;
                    prim->w = w; prim->h = w; prim->d = w;
                    prim->hasColorOverride = true;
                    prim->colorOverride = {(unsigned char)r, (unsigned char)g,
                                          (unsigned char)b, (unsigned char)a};
                    monster->primitiveCount++;
                }
                // Try cube/cylinder with color
                else if (sscanf(line, "%31s %f %f %f %f %f %f %d %d %d %d",
                               typeStr, &x, &y, &z, &w, &h, &d, &r, &g, &b, &a) >= 11) {
                    prim->type = (strcmp(typeStr, "sphere") == 0) ? PRIM_SPHERE :
                                 (strcmp(typeStr, "cylinder") == 0) ? PRIM_CYLINDER : PRIM_CUBE;
                    prim->x = x; prim->y = y; prim->z = z;
                    prim->w = w; prim->h = h; prim->d = d;
                    prim->hasColorOverride = true;
                    prim->colorOverride = {(unsigned char)r, (unsigned char)g,
                                          (unsigned char)b, (unsigned char)a};
                    monster->primitiveCount++;
                }
                // Try cube/cylinder without color
                else if (sscanf(line, "%31s %f %f %f %f %f %f",
                               typeStr, &x, &y, &z, &w, &h, &d) >= 7) {
                    prim->type = (strcmp(typeStr, "sphere") == 0) ? PRIM_SPHERE :
                                 (strcmp(typeStr, "cylinder") == 0) ? PRIM_CYLINDER : PRIM_CUBE;
                    prim->x = x; prim->y = y; prim->z = z;
                    prim->w = w; prim->h = h; prim->d = d;
                    prim->hasColorOverride = false;
                    monster->primitiveCount++;
                }
                // Try sphere without color
                else if (sscanf(line, "%31s %f %f %f %f", typeStr, &x, &y, &z, &w) >= 5 &&
                         strcmp(typeStr, "sphere") == 0) {
                    prim->type = PRIM_SPHERE;
                    prim->x = x; prim->y = y; prim->z = z;
                    prim->w = w; prim->h = w; prim->d = w;
                    prim->hasColorOverride = false;
                    monster->primitiveCount++;
                }
            }
        } else {
            // Parse properties
            if (strncmp(line, "name ", 5) == 0) {
                strncpy(monster->name, line + 5, sizeof(monster->name) - 1);
            } else if (strncmp(line, "level ", 6) == 0) {
                sscanf(line, "level %d", &monster->level);
            } else if (strncmp(line, "health ", 7) == 0) {
                sscanf(line, "health %d", &monster->maxHealth);
            } else if (strncmp(line, "damage ", 7) == 0) {
                sscanf(line, "damage %d", &monster->maxDamage);
            } else if (strncmp(line, "attack_cooldown ", 16) == 0) {
                sscanf(line, "attack_cooldown %f", &monster->attackCooldown);
            } else if (strncmp(line, "chase_speed ", 12) == 0) {
                sscanf(line, "chase_speed %f", &monster->chaseSpeed);
            } else if (strncmp(line, "attack_range ", 13) == 0) {
                sscanf(line, "attack_range %f", &monster->attackRange);
            } else if (strncmp(line, "aggressive ", 11) == 0) {
                char boolStr[16];
                if (sscanf(line, "aggressive %15s", boolStr) == 1) {
                    monster->aggressive = (strcmp(boolStr, "true") == 0);
                }
            } else if (strncmp(line, "body_color ", 11) == 0) {
                int r, g, b, a;
                if (sscanf(line, "body_color %d %d %d %d", &r, &g, &b, &a) >= 3) {
                    monster->bodyColor = {(unsigned char)r, (unsigned char)g,
                                         (unsigned char)b, (unsigned char)(a > 0 ? a : 255)};
                }
            } else if (strncmp(line, "limb_color ", 11) == 0) {
                int r, g, b, a;
                if (sscanf(line, "limb_color %d %d %d %d", &r, &g, &b, &a) >= 3) {
                    monster->limbColor = {(unsigned char)r, (unsigned char)g,
                                         (unsigned char)b, (unsigned char)(a > 0 ? a : 255)};
                }
            } else if (strcmp(line, "visual") == 0) {
                inVisual = true;
            }
        }

        line = strtok(nullptr, "\n");
    }

    // Debug: print what was parsed
    printf("[MonsterGen] Parsed monster: name='%s', level=%d, health=%d, primitiveCount=%d\n",
           monster->name, monster->level, monster->maxHealth, monster->primitiveCount);

    if (monster->name[0] == '\0') {
        printf("[MonsterGen] ERROR: No 'name' field found in monster definition\n");
        if (errorDetail) snprintf(errorDetail, errorSize, "Missing 'name' in monster definition");
        return false;
    }
    if (monster->primitiveCount == 0) {
        printf("[MonsterGen] ERROR: No primitives found in 'visual' section\n");
        if (errorDetail) snprintf(errorDetail, errorSize, "Missing 'visual' section or no valid primitives");
        return false;
    }

    printf("[MonsterGen] Successfully parsed monster: %s\n", monster->name);
    return true;
}

// Worker thread function for LLM request
static void GenerationWorker(MonsterGenerator* gen) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        gen->responseMutex->lock();
        gen->hasError = true;
        snprintf(gen->errorMessage, sizeof(gen->errorMessage), "Failed to init curl");
        gen->responseMutex->unlock();
        gen->requestInFlight->store(false);
        return;
    }

    const char* apiKey = getenv("MY_ANTHROPIC_API_KEY");
    if (!apiKey) {
        gen->responseMutex->lock();
        gen->hasError = true;
        snprintf(gen->errorMessage, sizeof(gen->errorMessage),
                "Set MY_ANTHROPIC_API_KEY environment variable");
        gen->responseMutex->unlock();
        curl_easy_cleanup(curl);
        gen->requestInFlight->store(false);
        return;
    }

    // Build prompt
    const char* systemPrompt =
        "You are designing monsters for a 3D game. Generate a monster definition based on the player's description.\\n\\n"
        "OUTPUT FORMAT (copy exactly):\\n"
        "name <display name>\\n"
        "level <1-50>\\n"
        "health <5-150>\\n"
        "damage <1-15>\\n"
        "attack_cooldown <0.8-2.5>\\n"
        "chase_speed <1.5-5.0>\\n"
        "attack_range <1.5-4.0>\\n"
        "aggressive <true/false>\\n"
        "body_color <r> <g> <b> 255\\n"
        "limb_color <r> <g> <b> 255\\n"
        "visual\\n"
        "<primitives>\\n"
        ".\\n\\n"
        "PRIMITIVES (one per line):\\n"
        "- cube x y z width height depth [r g b a]\\n"
        "- sphere x y z radius [r g b a]\\n"
        "- cylinder x y z radius height [r g b a]\\n\\n"
        "Y is up. Origin at monster's feet. Human height ~1.7 units.\\n"
        "Optional [r g b a] overrides body_color.\\n\\n"
        "EXAMPLE - Goblin (humanoid, level 3):\\n"
        "name Goblin\\n"
        "level 3\\n"
        "health 15\\n"
        "damage 3\\n"
        "attack_cooldown 1.0\\n"
        "chase_speed 3.5\\n"
        "attack_range 2.0\\n"
        "aggressive false\\n"
        "body_color 100 140 100 255\\n"
        "limb_color 70 100 70 255\\n"
        "visual\\n"
        "cube 0 0.8 0 0.5 0.7 0.35\\n"
        "sphere 0 1.45 0 0.3\\n"
        "cube -0.4 0.8 0 0.15 0.5 0.15\\n"
        "cube 0.4 0.8 0 0.15 0.5 0.15\\n"
        "cube -0.12 0.25 0 0.15 0.5 0.15\\n"
        "cube 0.12 0.25 0 0.15 0.5 0.15\\n"
        ".\\n\\n"
        "Match stats to description (weak=low level, fierce=aggressive/high damage). Be creative with shapes.";

    // Escape user input for JSON
    char escapedInput[1024];
    int ei = 0;
    for (int i = 0; gen->inputBuffer[i] && ei < (int)sizeof(escapedInput) - 2; i++) {
        if (gen->inputBuffer[i] == '"' || gen->inputBuffer[i] == '\\') {
            escapedInput[ei++] = '\\';
        }
        escapedInput[ei++] = gen->inputBuffer[i];
    }
    escapedInput[ei] = '\0';

    // Build JSON body
    char jsonBody[16384];
    snprintf(jsonBody, sizeof(jsonBody),
        "{"
        "\"model\": \"claude-opus-4-5\","
        "\"max_tokens\": 1024,"
        "\"system\": \"%s\","
        "\"messages\": [{\"role\": \"user\", \"content\": \"Create a monster: %s\"}]"
        "}",
        systemPrompt, escapedInput);

    // Set up headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    char authHeader[256];
    snprintf(authHeader, sizeof(authHeader), "x-api-key: %s", apiKey);
    headers = curl_slist_append(headers, authHeader);
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    // Set up curl
    gen->response[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, gen->response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    // Perform request
    CURLcode res = curl_easy_perform(curl);

    gen->responseMutex->lock();
    if (res != CURLE_OK) {
        gen->hasError = true;
        snprintf(gen->errorMessage, sizeof(gen->errorMessage),
                "Curl error: %s", curl_easy_strerror(res));
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode != 200) {
            gen->hasError = true;
            // Try to extract error message from response body
            char errorDetail[128] = {0};
            ParseMonsterResponse(gen->response, &gen->generatedMonster, errorDetail, sizeof(errorDetail));
            if (errorDetail[0]) {
                snprintf(gen->errorMessage, sizeof(gen->errorMessage),
                        "API error (HTTP %ld): %s", httpCode, errorDetail);
            } else {
                snprintf(gen->errorMessage, sizeof(gen->errorMessage),
                        "API error (HTTP %ld)", httpCode);
            }
            printf("[MonsterGen] HTTP error %ld, response: %.500s\n", httpCode, gen->response);
        } else {
            // Parse response
            char errorDetail[128] = {0};
            if (ParseMonsterResponse(gen->response, &gen->generatedMonster, errorDetail, sizeof(errorDetail))) {
                gen->generationReady = true;
            } else {
                gen->hasError = true;
                if (errorDetail[0]) {
                    snprintf(gen->errorMessage, sizeof(gen->errorMessage),
                            "Parse error: %s", errorDetail);
                } else {
                    snprintf(gen->errorMessage, sizeof(gen->errorMessage),
                            "Failed to parse monster from response");
                }
            }
        }
    }
    gen->responseMutex->unlock();

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    gen->requestInFlight->store(false);
}

void InitMonsterGenerator(MonsterGenerator* gen) {
    memset(gen, 0, sizeof(MonsterGenerator));
    gen->state = GeneratorState::CLOSED;
    gen->selectedIndex = -1;
    gen->activeEggIndex = -1;

    gen->workerThread = nullptr;
    gen->requestInFlight = new std::atomic<bool>(false);
    gen->responseMutex = new std::mutex();
}

void ShutdownMonsterGenerator(MonsterGenerator* gen) {
    // Wait for any pending request
    if (gen->workerThread && gen->workerThread->joinable()) {
        gen->workerThread->join();
        delete gen->workerThread;
    }

    delete gen->requestInFlight;
    delete gen->responseMutex;
}

void OpenMonsterGenerator(MonsterGenerator* gen) {
    gen->state = GeneratorState::TYPING;
    gen->inputBuffer[0] = '\0';
    gen->inputLength = 0;
    gen->cursorPos = 0;
    gen->hasError = false;
    gen->errorMessage[0] = '\0';
}

void CloseMonsterGenerator(MonsterGenerator* gen) {
    gen->state = GeneratorState::CLOSED;
}

bool IsGeneratorOpen(const MonsterGenerator* gen) {
    return gen->state != GeneratorState::CLOSED;
}

void StartMonsterGeneration(MonsterGenerator* gen, const char* description, Vector3 spawnPos) {
    printf("[MonsterGen] StartMonsterGeneration called, description='%s'\n", description);

    // Find inactive egg slot
    int eggIdx = -1;
    for (int i = 0; i < MAX_MONSTER_EGGS; i++) {
        if (!gen->eggs[i].active) {
            eggIdx = i;
            break;
        }
    }
    if (eggIdx < 0) return;  // No slots

    // Initialize egg
    MonsterEgg* egg = &gen->eggs[eggIdx];
    egg->position = spawnPos;
    egg->timer = 0.0f;
    egg->hatching = false;
    egg->hatchTimer = 0.0f;
    egg->customMonsterIndex = -1;
    egg->active = true;
    gen->activeEggIndex = eggIdx;

    // Copy description
    strncpy(gen->inputBuffer, description, GEN_INPUT_MAX - 1);
    gen->inputLength = (int)strlen(gen->inputBuffer);

    // Start worker thread
    gen->generationReady = false;
    gen->hasError = false;
    gen->requestInFlight->store(true);

    // Clean up old thread if any
    if (gen->workerThread && gen->workerThread->joinable()) {
        gen->workerThread->join();
        delete gen->workerThread;
    }
    gen->workerThread = new std::thread(GenerationWorker, gen);

    gen->state = GeneratorState::GENERATING;
}

bool UpdateMonsterGenerator(MonsterGenerator* gen, CustomMonster* customMonsters,
                            int* customMonsterCount, int maxMonsters,
                            Enemy* enemies, int* enemyCount, int maxEnemies,
                            float deltaTime) {
    bool monsterSpawned = false;

    // Debug: print state every few seconds
    static float debugTimer = 0;
    debugTimer += deltaTime;
    if (debugTimer > 2.0f && gen->activeEggIndex >= 0) {
        debugTimer = 0;
        printf("[MonsterGen] Update: state=%d, activeEggIndex=%d, requestInFlight=%d, generationReady=%d, hasError=%d\n",
               (int)gen->state, gen->activeEggIndex,
               gen->requestInFlight->load(), gen->generationReady, gen->hasError);
    }

    // Update eggs
    for (int i = 0; i < MAX_MONSTER_EGGS; i++) {
        MonsterEgg* egg = &gen->eggs[i];
        if (!egg->active) continue;

        egg->timer += deltaTime;

        if (egg->hatching) {
            egg->hatchTimer += deltaTime / HATCH_DURATION;
            if (egg->hatchTimer >= 1.0f) {
                // Hatch complete - spawn monster
                if (egg->customMonsterIndex >= 0 && *enemyCount < maxEnemies) {
                    Enemy* enemy = &enemies[*enemyCount];
                    memset(enemy, 0, sizeof(Enemy));
                    enemy->type = ENEMY_TYPE_COUNT;  // Marker for custom monster
                    enemy->customMonsterIndex = egg->customMonsterIndex;
                    enemy->position = egg->position;
                    enemy->spawnPoint = egg->position;

                    const CustomMonster* m = &customMonsters[egg->customMonsterIndex];
                    enemy->health = m->maxHealth;
                    enemy->alive = true;
                    enemy->hostile = false;
                    enemy->facingAngle = RandomFloat(0.0f, 2.0f * PI);

                    (*enemyCount)++;
                    monsterSpawned = true;
                    PlaySoundEffect(SFX_LEVEL_UP);  // Hatch sound
                }

                egg->active = false;
                if (gen->activeEggIndex == i) {
                    gen->activeEggIndex = -1;
                }
            }
        }
    }

    // Check for generation completion (even if menu was closed)
    bool inFlight = gen->requestInFlight->load();
    if (!inFlight && gen->activeEggIndex >= 0) {
        printf("[MonsterGen] Generation complete! Checking results...\n");
        gen->responseMutex->lock();
        if (gen->hasError) {
            // Only show error if menu is still open
            if (gen->state != GeneratorState::CLOSED) {
                gen->state = GeneratorState::ERROR;
            }
            // Deactivate egg on error
            if (gen->activeEggIndex >= 0) {
                gen->eggs[gen->activeEggIndex].active = false;
                gen->activeEggIndex = -1;
            }
        } else if (gen->generationReady) {
            // Save monster (only once)
            gen->generationReady = false;  // Clear flag immediately to prevent re-processing

            GenerateMonsterID(gen->generatedMonster.id, sizeof(gen->generatedMonster.id));
            strncpy(gen->generatedMonster.description, gen->inputBuffer,
                   sizeof(gen->generatedMonster.description) - 1);
            int newIdx = AddCustomMonster(customMonsters, customMonsterCount, maxMonsters,
                                         &gen->generatedMonster);
            if (newIdx >= 0) {
                SaveCustomMonsters("monsters/custom.monster", customMonsters, *customMonsterCount);
                printf("[MonsterGen] Saved monster '%s' at index %d\n", gen->generatedMonster.name, newIdx);

                // Trigger egg hatch
                if (gen->activeEggIndex >= 0) {
                    gen->eggs[gen->activeEggIndex].customMonsterIndex = newIdx;
                    gen->eggs[gen->activeEggIndex].hatching = true;
                }
            }

            // Clear activeEggIndex tracking (egg will handle its own lifecycle)
            // Note: don't set to -1 yet, let the egg hatch first

            // Only reset UI state if menu is still open
            if (gen->state != GeneratorState::CLOSED) {
                gen->state = GeneratorState::TYPING;
                gen->inputBuffer[0] = '\0';
                gen->inputLength = 0;
                gen->cursorPos = 0;
            }
        }
        gen->responseMutex->unlock();
    }

    // Handle input in TYPING state
    if (gen->state == GeneratorState::TYPING) {
        // Character input
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126 && gen->inputLength < GEN_INPUT_MAX - 1) {
                // Insert at cursor
                for (int i = gen->inputLength; i > gen->cursorPos; i--) {
                    gen->inputBuffer[i] = gen->inputBuffer[i - 1];
                }
                gen->inputBuffer[gen->cursorPos] = (char)key;
                gen->cursorPos++;
                gen->inputLength++;
                gen->inputBuffer[gen->inputLength] = '\0';
            }
            key = GetCharPressed();
        }

        // Backspace
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (gen->cursorPos > 0) {
                for (int i = gen->cursorPos - 1; i < gen->inputLength; i++) {
                    gen->inputBuffer[i] = gen->inputBuffer[i + 1];
                }
                gen->cursorPos--;
                gen->inputLength--;
            }
        }

        // Cursor movement
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            if (gen->cursorPos > 0) gen->cursorPos--;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            if (gen->cursorPos < gen->inputLength) gen->cursorPos++;
        }

        // List navigation
        if (IsKeyPressed(KEY_UP)) {
            if (gen->selectedIndex > 0) gen->selectedIndex--;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            gen->selectedIndex++;
        }
    }

    // Handle error state
    if (gen->state == GeneratorState::ERROR) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            gen->state = GeneratorState::TYPING;
            gen->hasError = false;
        }
    }

    return monsterSpawned;
}

// Helper: Get spawn position in front of player
static Vector3 GetSpawnPositionInFrontOfPlayer(const Camera3D* camera) {
    Vector3 forward = Vector3Normalize({
        camera->target.x - camera->position.x,
        0,
        camera->target.z - camera->position.z
    });
    return Vector3Add(camera->position, Vector3Scale(forward, 5.0f));
}

bool HandleMonsterGeneratorInput(MonsterGenerator* gen,
                                  CustomMonster* customMonsters, int* customMonsterCount, int maxMonsters,
                                  Enemy* enemies, int* enemyCount, int maxEnemies,
                                  const Camera3D* camera,
                                  int screenWidth, int screenHeight) {
    if (gen->state == GeneratorState::CLOSED) return false;

    int x = (screenWidth - UI_WIDTH) / 2;
    int y = (screenHeight - UI_HEIGHT) / 2;

    // Calculate positions matching draw function
    int contentY = y + 55 + 22;  // After "Describe your monster:" label
    int inputBoxY = contentY;
    contentY += INPUT_HEIGHT + 10;

    // Generate button position
    Rectangle genBtn = { (float)(x + 15), (float)contentY, 100, BUTTON_HEIGHT };
    contentY += BUTTON_HEIGHT + 15 + 12 + 22;  // +divider +header

    // List area
    int listY = contentY;
    Rectangle listBox = { (float)(x + 15), (float)listY,
                         (float)(UI_WIDTH - 30), (float)(LIST_ITEM_HEIGHT * LIST_VISIBLE_ITEMS) };
    contentY += LIST_ITEM_HEIGHT * LIST_VISIBLE_ITEMS + 10;

    // Spawn and Delete buttons
    Rectangle spawnBtn = { (float)(x + 15), (float)contentY, 80, BUTTON_HEIGHT };
    Rectangle deleteBtn = { (float)(x + 105), (float)contentY, 80, BUTTON_HEIGHT };

    // Handle mouse clicks
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();

        // Generate button
        if (gen->state == GeneratorState::TYPING && gen->inputLength > 0) {
            if (CheckCollisionPointRec(mousePos, genBtn)) {
                Vector3 spawnPos = GetSpawnPositionInFrontOfPlayer(camera);
                StartMonsterGeneration(gen, gen->inputBuffer, spawnPos);
                CloseMonsterGenerator(gen);  // Close to see the egg
                return true;  // Signal menu closed
            }
        }

        // List item clicks
        if (CheckCollisionPointRec(mousePos, listBox)) {
            int visibleStart = gen->listScrollOffset;
            for (int i = visibleStart; i < visibleStart + LIST_VISIBLE_ITEMS && i < *customMonsterCount; i++) {
                int itemY = listY + (i - visibleStart) * LIST_ITEM_HEIGHT;
                Rectangle itemRect = { (float)(x + 16), (float)itemY, (float)(UI_WIDTH - 32), LIST_ITEM_HEIGHT };
                if (CheckCollisionPointRec(mousePos, itemRect)) {
                    gen->selectedIndex = i;
                    break;
                }
            }
        }

        // Spawn button
        bool canSpawn = gen->selectedIndex >= 0 && gen->selectedIndex < *customMonsterCount;
        if (canSpawn && CheckCollisionPointRec(mousePos, spawnBtn)) {
            Vector3 spawnPos = GetSpawnPositionInFrontOfPlayer(camera);
            SpawnMonsterFromList(gen, gen->selectedIndex, customMonsters, *customMonsterCount,
                                 enemies, enemyCount, maxEnemies, spawnPos);
            CloseMonsterGenerator(gen);  // Close after spawning
            return true;  // Signal menu closed
        }

        // Delete button
        if (canSpawn && CheckCollisionPointRec(mousePos, deleteBtn)) {
            // Remove from array
            for (int i = gen->selectedIndex; i < *customMonsterCount - 1; i++) {
                customMonsters[i] = customMonsters[i + 1];
            }
            (*customMonsterCount)--;

            // Update selection
            if (gen->selectedIndex >= *customMonsterCount) {
                gen->selectedIndex = *customMonsterCount - 1;
            }

            // Save changes
            SaveCustomMonsters("monsters/custom.monster", customMonsters, *customMonsterCount);
            return false;
        }
    }

    // Mouse wheel for list scrolling
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && CheckCollisionPointRec(GetMousePosition(), listBox)) {
        gen->listScrollOffset -= (int)wheel;
        if (gen->listScrollOffset < 0) gen->listScrollOffset = 0;
        int maxScroll = *customMonsterCount - LIST_VISIBLE_ITEMS;
        if (maxScroll < 0) maxScroll = 0;
        if (gen->listScrollOffset > maxScroll) gen->listScrollOffset = maxScroll;
    }

    return false;
}

void DrawMonsterGenerator(const MonsterGenerator* gen,
                          const CustomMonster* customMonsters, int customMonsterCount,
                          int screenWidth, int screenHeight) {
    if (gen->state == GeneratorState::CLOSED) return;

    int x = (screenWidth - UI_WIDTH) / 2;
    int y = (screenHeight - UI_HEIGHT) / 2;

    // Parchment background with border (matching other menus)
    DrawRectangle(x - 4, y - 4, UI_WIDTH + 8, UI_HEIGHT + 8, PARCHMENT_BORDER);
    DrawRectangle(x, y, UI_WIDTH, UI_HEIGHT, PARCHMENT_BG);
    DrawRectangleLines(x + 6, y + 6, UI_WIDTH - 12, UI_HEIGHT - 12, PARCHMENT_DARK);

    // Corner decorations (like other parchment menus)
    DrawTriangle((Vector2){(float)x, (float)y},
                 (Vector2){(float)(x + 20), (float)y},
                 (Vector2){(float)x, (float)(y + 20)}, PARCHMENT_BORDER);
    DrawTriangle((Vector2){(float)(x + UI_WIDTH), (float)y},
                 (Vector2){(float)(x + UI_WIDTH - 20), (float)y},
                 (Vector2){(float)(x + UI_WIDTH), (float)(y + 20)}, PARCHMENT_BORDER);
    DrawTriangle((Vector2){(float)x, (float)(y + UI_HEIGHT)},
                 (Vector2){(float)(x + 20), (float)(y + UI_HEIGHT)},
                 (Vector2){(float)x, (float)(y + UI_HEIGHT - 20)}, PARCHMENT_BORDER);
    DrawTriangle((Vector2){(float)(x + UI_WIDTH), (float)(y + UI_HEIGHT)},
                 (Vector2){(float)(x + UI_WIDTH - 20), (float)(y + UI_HEIGHT)},
                 (Vector2){(float)(x + UI_WIDTH), (float)(y + UI_HEIGHT - 20)}, PARCHMENT_BORDER);

    // Title
    const char* title = "Monster Generator";
    int titleWidth = MeasureText(title, 24);
    DrawText(title, x + (UI_WIDTH - titleWidth) / 2, y + 12, 24, PARCHMENT_BORDER);

    // Title separator
    DrawRectangle(x + 15, y + 45, UI_WIDTH - 30, 2, PARCHMENT_BORDER);

    int contentY = y + 55;

    // Input section
    DrawText("Describe your monster:", x + 15, contentY, 16, PARCHMENT_TEXT);
    contentY += 22;

    Rectangle inputBox = { (float)(x + 15), (float)contentY, (float)(UI_WIDTH - 30), INPUT_HEIGHT };
    DrawRectangleRec(inputBox, PARCHMENT_INPUT_BG);
    DrawRectangleLinesEx(inputBox, 1, PARCHMENT_BORDER);

    // Draw input text with cursor
    if (gen->state == GeneratorState::TYPING) {
        const char* text = gen->inputBuffer;
        DrawText(text, x + 20, contentY + 7, 14, PARCHMENT_TEXT);

        // Cursor
        if ((int)(GetTime() * 2) % 2 == 0) {
            int cursorX = x + 20 + MeasureText(gen->inputBuffer, 14);
            if (gen->cursorPos < gen->inputLength) {
                char temp[GEN_INPUT_MAX];
                strncpy(temp, gen->inputBuffer, gen->cursorPos);
                temp[gen->cursorPos] = '\0';
                cursorX = x + 20 + MeasureText(temp, 14);
            }
            DrawLine(cursorX, contentY + 5, cursorX, contentY + INPUT_HEIGHT - 5, PARCHMENT_TEXT);
        }
    } else if (gen->state == GeneratorState::GENERATING) {
        const char* dots = "...";
        int dotCount = ((int)(GetTime() * 3)) % 4;
        char loadingText[32];
        snprintf(loadingText, sizeof(loadingText), "Generating%.*s", dotCount, dots);
        DrawText(loadingText, x + 20, contentY + 7, 14, PARCHMENT_TEXT);
    }
    contentY += INPUT_HEIGHT + 10;

    // Generate button
    Rectangle genBtn = { (float)(x + 15), (float)contentY, 100, BUTTON_HEIGHT };
    bool canGenerate = gen->state == GeneratorState::TYPING && gen->inputLength > 0;
    bool hoverGen = canGenerate && CheckCollisionPointRec(GetMousePosition(), genBtn);
    Color genBtnColor = canGenerate ? (hoverGen ? PARCHMENT_BUTTON_HOVER : PARCHMENT_BUTTON) : PARCHMENT_DARK;
    DrawRectangleRec(genBtn, genBtnColor);
    DrawRectangleLinesEx(genBtn, 1, PARCHMENT_BORDER);
    int genTextW = MeasureText("Generate", 14);
    DrawText("Generate", x + 15 + (100 - genTextW) / 2, contentY + 8, 14, PARCHMENT_TEXT);
    contentY += BUTTON_HEIGHT + 15;

    // Divider
    DrawRectangle(x + 15, contentY, UI_WIDTH - 30, 2, PARCHMENT_BORDER);
    contentY += 12;

    // Monster list header
    DrawText("Your Monsters:", x + 15, contentY, 16, PARCHMENT_TEXT);
    contentY += 22;

    // Monster list
    Rectangle listBox = { (float)(x + 15), (float)contentY,
                         (float)(UI_WIDTH - 30), (float)(LIST_ITEM_HEIGHT * LIST_VISIBLE_ITEMS) };
    DrawRectangleRec(listBox, PARCHMENT_INPUT_BG);
    DrawRectangleLinesEx(listBox, 1, PARCHMENT_BORDER);

    int visibleStart = gen->listScrollOffset;
    int visibleEnd = visibleStart + LIST_VISIBLE_ITEMS;
    if (visibleEnd > customMonsterCount) visibleEnd = customMonsterCount;

    for (int i = visibleStart; i < visibleEnd; i++) {
        int itemY = contentY + (i - visibleStart) * LIST_ITEM_HEIGHT;
        Rectangle itemRect = { (float)(x + 16), (float)itemY, (float)(UI_WIDTH - 32), LIST_ITEM_HEIGHT };

        bool isSelected = (i == gen->selectedIndex);
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), itemRect);

        if (isSelected) {
            DrawRectangleRec(itemRect, PARCHMENT_SELECTED);
        } else if (isHovered) {
            DrawRectangleRec(itemRect, (Color){190, 170, 130, 255});
        }

        char itemText[128];
        snprintf(itemText, sizeof(itemText), "%s%s (Lv.%d)",
                isSelected ? "> " : "  ",
                customMonsters[i].name, customMonsters[i].level);
        DrawText(itemText, x + 20, itemY + 5, 14, PARCHMENT_TEXT);
    }

    contentY += LIST_ITEM_HEIGHT * LIST_VISIBLE_ITEMS + 10;

    // Spawn and Delete buttons
    Rectangle spawnBtn = { (float)(x + 15), (float)contentY, 80, BUTTON_HEIGHT };
    Rectangle deleteBtn = { (float)(x + 105), (float)contentY, 80, BUTTON_HEIGHT };

    bool canSpawn = gen->selectedIndex >= 0 && gen->selectedIndex < customMonsterCount;
    bool hoverSpawn = canSpawn && CheckCollisionPointRec(GetMousePosition(), spawnBtn);
    bool hoverDelete = canSpawn && CheckCollisionPointRec(GetMousePosition(), deleteBtn);

    Color spawnBtnColor = canSpawn ? (hoverSpawn ? PARCHMENT_BUTTON_HOVER : PARCHMENT_BUTTON) : PARCHMENT_DARK;
    DrawRectangleRec(spawnBtn, spawnBtnColor);
    DrawRectangleLinesEx(spawnBtn, 1, PARCHMENT_BORDER);
    int spawnTextW = MeasureText("Spawn", 14);
    DrawText("Spawn", x + 15 + (80 - spawnTextW) / 2, contentY + 8, 14, PARCHMENT_TEXT);

    Color deleteBtnColor = canSpawn ? (hoverDelete ? (Color){180, 120, 100, 255} : (Color){160, 100, 80, 255})
                                     : PARCHMENT_DARK;
    DrawRectangleRec(deleteBtn, deleteBtnColor);
    DrawRectangleLinesEx(deleteBtn, 1, PARCHMENT_BORDER);
    int deleteTextW = MeasureText("Delete", 14);
    DrawText("Delete", x + 105 + (80 - deleteTextW) / 2, contentY + 8, 14, PARCHMENT_TEXT);

    // Error message
    if (gen->state == GeneratorState::ERROR) {
        DrawRectangle(x + 30, y + UI_HEIGHT / 2 - 40, UI_WIDTH - 60, 80, (Color){180, 140, 120, 250});
        DrawRectangleLines(x + 30, y + UI_HEIGHT / 2 - 40, UI_WIDTH - 60, 80, PARCHMENT_BORDER);
        DrawText("Error:", x + 40, y + UI_HEIGHT / 2 - 30, 16, PARCHMENT_TEXT);
        DrawText(gen->errorMessage, x + 40, y + UI_HEIGHT / 2 - 10, 12, PARCHMENT_TEXT);
        DrawText("Press ENTER to continue", x + 40, y + UI_HEIGHT / 2 + 20, 12, PARCHMENT_BORDER);
    }
}

void DrawMonsterEggs(const EntityModels* models, const MonsterGenerator* gen) {
    for (int i = 0; i < MAX_MONSTER_EGGS; i++) {
        const MonsterEgg* egg = &gen->eggs[i];
        if (!egg->active) continue;

        float wobble = sinf(egg->timer * EGG_WOBBLE_SPEED) * EGG_WOBBLE_ANGLE;

        rlPushMatrix();
        rlTranslatef(egg->position.x, egg->position.y, egg->position.z);
        rlRotatef(wobble, 0, 0, 1);

        if (egg->hatching) {
            // Hatching animation - egg gets smaller, cracks appear
            float scale = 1.0f - egg->hatchTimer * 0.5f;
            rlScalef(scale, scale, scale);

            // Draw cracking egg
            DrawModelSphere(models, (Vector3){0, 0.3f, 0}, 0.25f, EGG_COLOR);

            // Crack lines
            if (egg->hatchTimer > 0.3f) {
                Color crackColor = {50, 40, 30, 255};
                DrawModelCube(models, (Vector3){0.05f, 0.35f, 0.12f}, 0.02f, 0.15f, 0.02f, crackColor);
                DrawModelCube(models, (Vector3){-0.08f, 0.25f, 0.1f}, 0.02f, 0.1f, 0.02f, crackColor);
            }
        } else {
            // Normal egg with spots
            // Main egg shape (stretched sphere)
            rlPushMatrix();
            rlScalef(0.8f, 1.0f, 0.8f);
            DrawModelSphere(models, (Vector3){0, 0.3f, 0}, 0.3f, EGG_COLOR);
            rlPopMatrix();

            // Spots
            DrawModelSphere(models, (Vector3){0.1f, 0.35f, 0.15f}, 0.05f, EGG_SPOT_COLOR);
            DrawModelSphere(models, (Vector3){-0.08f, 0.25f, 0.12f}, 0.04f, EGG_SPOT_COLOR);
            DrawModelSphere(models, (Vector3){0.05f, 0.45f, 0.08f}, 0.03f, EGG_SPOT_COLOR);
        }

        rlPopMatrix();
    }
}

void SpawnMonsterFromList(MonsterGenerator* gen, int monsterIndex,
                          const CustomMonster* customMonsters, int customMonsterCount,
                          Enemy* enemies, int* enemyCount, int maxEnemies,
                          Vector3 position) {
    if (monsterIndex < 0 || monsterIndex >= customMonsterCount) return;
    if (*enemyCount >= maxEnemies) return;

    const CustomMonster* m = &customMonsters[monsterIndex];
    Enemy* enemy = &enemies[*enemyCount];

    memset(enemy, 0, sizeof(Enemy));
    enemy->type = ENEMY_TYPE_COUNT;  // Marker for custom monster
    enemy->customMonsterIndex = monsterIndex;
    enemy->position = position;
    enemy->spawnPoint = position;
    enemy->health = m->maxHealth;
    enemy->alive = true;
    enemy->hostile = false;
    enemy->facingAngle = RandomFloat(0.0f, 2.0f * PI);

    (*enemyCount)++;
}
