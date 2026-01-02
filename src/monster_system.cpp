#include "monster_system.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

// Parse a color from "r g b a" format
static Color ParseColor(const char* line) {
    Color c = {0, 0, 0, 255};
    int r, g, b, a;
    if (sscanf(line, "%d %d %d %d", &r, &g, &b, &a) >= 3) {
        c.r = (unsigned char)r;
        c.g = (unsigned char)g;
        c.b = (unsigned char)b;
        c.a = (a > 0) ? (unsigned char)a : 255;
    }
    return c;
}

// Parse a primitive type from string
static PrimitiveType ParsePrimitiveType(const char* typeStr) {
    if (strcmp(typeStr, "sphere") == 0) return PRIM_SPHERE;
    if (strcmp(typeStr, "cylinder") == 0) return PRIM_CYLINDER;
    return PRIM_CUBE;  // Default to cube
}

// Parse a monster material type from string
static MonsterMaterial ParseMaterialType(const char* matStr) {
    if (strcmp(matStr, "scales") == 0) return MAT_SCALES;
    if (strcmp(matStr, "stone") == 0) return MAT_STONE;
    if (strcmp(matStr, "fur") == 0) return MAT_FUR;
    if (strcmp(matStr, "striped") == 0) return MAT_STRIPED;
    if (strcmp(matStr, "spotted") == 0) return MAT_SPOTTED;
    return MAT_FLAT;  // Default to flat
}

// Get material type name for saving
static const char* GetMaterialName(MonsterMaterial mat) {
    switch (mat) {
        case MAT_SCALES: return "scales";
        case MAT_STONE: return "stone";
        case MAT_FUR: return "fur";
        case MAT_STRIPED: return "striped";
        case MAT_SPOTTED: return "spotted";
        default: return "flat";
    }
}

// Parse a visual primitive line
// Format: "type x y z w h d [r g b a]"
// For sphere: w is radius, h and d are ignored
static bool ParsePrimitive(const char* line, MonsterPrimitive* prim) {
    char typeStr[32];
    float x, y, z, w, h = 0, d = 0;
    int r, g, b, a;

    // Try parsing with color override (sphere with radius only)
    int parsed = sscanf(line, "%31s %f %f %f %f %d %d %d %d",
                        typeStr, &x, &y, &z, &w, &r, &g, &b, &a);

    if (parsed >= 9) {
        // Sphere with color override
        prim->type = ParsePrimitiveType(typeStr);
        prim->x = x;
        prim->y = y;
        prim->z = z;
        prim->w = w;
        prim->h = w;  // For spheres, all dimensions are radius
        prim->d = w;
        prim->hasColorOverride = true;
        prim->colorOverride = {(unsigned char)r, (unsigned char)g,
                               (unsigned char)b, (unsigned char)a};
        return true;
    }

    // Try parsing cube/cylinder with all dimensions + color
    parsed = sscanf(line, "%31s %f %f %f %f %f %f %d %d %d %d",
                    typeStr, &x, &y, &z, &w, &h, &d, &r, &g, &b, &a);

    if (parsed >= 11) {
        // Full primitive with color override
        prim->type = ParsePrimitiveType(typeStr);
        prim->x = x;
        prim->y = y;
        prim->z = z;
        prim->w = w;
        prim->h = h;
        prim->d = d;
        prim->hasColorOverride = true;
        prim->colorOverride = {(unsigned char)r, (unsigned char)g,
                               (unsigned char)b, (unsigned char)a};
        return true;
    }

    // Try parsing cube/cylinder with all dimensions, no color
    parsed = sscanf(line, "%31s %f %f %f %f %f %f",
                    typeStr, &x, &y, &z, &w, &h, &d);

    if (parsed >= 7) {
        // Full primitive without color override
        prim->type = ParsePrimitiveType(typeStr);
        prim->x = x;
        prim->y = y;
        prim->z = z;
        prim->w = w;
        prim->h = h;
        prim->d = d;
        prim->hasColorOverride = false;
        return true;
    }

    // Try parsing sphere with just radius, no color
    parsed = sscanf(line, "%31s %f %f %f %f", typeStr, &x, &y, &z, &w);

    if (parsed >= 5) {
        // Sphere (radius only)
        prim->type = ParsePrimitiveType(typeStr);
        prim->x = x;
        prim->y = y;
        prim->z = z;
        prim->w = w;
        prim->h = w;
        prim->d = w;
        prim->hasColorOverride = false;
        return true;
    }

    return false;
}

int LoadCustomMonsters(const char* filepath, CustomMonster* monsters, int maxMonsters) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        printf("INFO: No custom monsters file found at %s (will be created on first save)\n", filepath);
        return 0;
    }

    int monsterCount = 0;
    char line[512];
    CustomMonster* current = nullptr;
    bool inVisualBlock = false;

    while (fgets(line, sizeof(line), file) && monsterCount < maxMonsters) {
        // Trim newline
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        // Skip empty lines and comments
        if (len == 0 || line[0] == '#') continue;

        // Check for visual block end
        if (inVisualBlock && strcmp(line, ".") == 0) {
            inVisualBlock = false;
            continue;
        }

        // Check for monster block end
        if (current && strcmp(line, "end") == 0) {
            current->loaded = true;
            current = nullptr;
            continue;
        }

        // Parse visual primitives inside visual block
        if (inVisualBlock && current) {
            if (current->primitiveCount < MAX_MONSTER_PRIMITIVES) {
                if (ParsePrimitive(line, &current->primitives[current->primitiveCount])) {
                    current->primitiveCount++;
                }
            }
            continue;
        }

        // Parse directives
        char key[64];
        if (sscanf(line, "%63s", key) != 1) continue;

        if (strcmp(key, "monster") == 0) {
            // Start new monster
            current = &monsters[monsterCount++];
            memset(current, 0, sizeof(CustomMonster));
            sscanf(line + 8, "%31s", current->id);
            // Set defaults
            current->level = 1;
            current->maxHealth = 10;
            current->maxDamage = 1;
            current->attackCooldown = 1.0f;
            current->chaseSpeed = 3.0f;
            current->attackRange = 2.0f;
            current->aggressive = false;
            current->bodyColor = {150, 150, 150, 255};
            current->limbColor = {100, 100, 100, 255};
            current->material = MAT_FLAT;
        }
        else if (current) {
            // Parse monster properties
            if (strcmp(key, "name") == 0) {
                const char* value = line + 5;
                while (*value == ' ') value++;
                strncpy(current->name, value, sizeof(current->name) - 1);
            }
            else if (strcmp(key, "description") == 0) {
                const char* value = line + 12;
                while (*value == ' ') value++;
                strncpy(current->description, value, sizeof(current->description) - 1);
            }
            else if (strcmp(key, "level") == 0) {
                sscanf(line, "level %d", &current->level);
            }
            else if (strcmp(key, "health") == 0) {
                sscanf(line, "health %d", &current->maxHealth);
            }
            else if (strcmp(key, "damage") == 0) {
                sscanf(line, "damage %d", &current->maxDamage);
            }
            else if (strcmp(key, "attack_cooldown") == 0) {
                sscanf(line, "attack_cooldown %f", &current->attackCooldown);
            }
            else if (strcmp(key, "chase_speed") == 0) {
                sscanf(line, "chase_speed %f", &current->chaseSpeed);
            }
            else if (strcmp(key, "attack_range") == 0) {
                sscanf(line, "attack_range %f", &current->attackRange);
            }
            else if (strcmp(key, "aggressive") == 0) {
                char boolStr[16];
                if (sscanf(line, "aggressive %15s", boolStr) == 1) {
                    current->aggressive = (strcmp(boolStr, "true") == 0);
                }
            }
            else if (strcmp(key, "body_color") == 0) {
                current->bodyColor = ParseColor(line + 11);
            }
            else if (strcmp(key, "limb_color") == 0) {
                current->limbColor = ParseColor(line + 11);
            }
            else if (strcmp(key, "material") == 0) {
                char matStr[32];
                if (sscanf(line, "material %31s", matStr) == 1) {
                    current->material = ParseMaterialType(matStr);
                }
            }
            else if (strcmp(key, "visual") == 0) {
                inVisualBlock = true;
            }
        }
    }

    fclose(file);
    printf("Loaded %d custom monsters from %s\n", monsterCount, filepath);
    return monsterCount;
}

bool SaveCustomMonsters(const char* filepath, const CustomMonster* monsters, int monsterCount) {
    FILE* file = fopen(filepath, "w");
    if (!file) {
        printf("ERROR: Could not open %s for writing\n", filepath);
        return false;
    }

    fprintf(file, "# Custom Monster Definitions\n");
    fprintf(file, "# Generated by in-game LLM system\n\n");

    for (int i = 0; i < monsterCount; i++) {
        const CustomMonster* m = &monsters[i];
        if (!m->loaded) continue;

        fprintf(file, "monster %s\n", m->id);
        fprintf(file, "name %s\n", m->name);
        if (m->description[0]) {
            fprintf(file, "description %s\n", m->description);
        }
        fprintf(file, "level %d\n", m->level);
        fprintf(file, "health %d\n", m->maxHealth);
        fprintf(file, "damage %d\n", m->maxDamage);
        fprintf(file, "attack_cooldown %.1f\n", m->attackCooldown);
        fprintf(file, "chase_speed %.1f\n", m->chaseSpeed);
        fprintf(file, "attack_range %.1f\n", m->attackRange);
        fprintf(file, "aggressive %s\n", m->aggressive ? "true" : "false");
        fprintf(file, "body_color %d %d %d %d\n",
                m->bodyColor.r, m->bodyColor.g, m->bodyColor.b, m->bodyColor.a);
        fprintf(file, "limb_color %d %d %d %d\n",
                m->limbColor.r, m->limbColor.g, m->limbColor.b, m->limbColor.a);
        if (m->material != MAT_FLAT) {
            fprintf(file, "material %s\n", GetMaterialName(m->material));
        }

        if (m->primitiveCount > 0) {
            fprintf(file, "visual\n");
            for (int p = 0; p < m->primitiveCount; p++) {
                const MonsterPrimitive* prim = &m->primitives[p];
                const char* typeStr = "cube";
                if (prim->type == PRIM_SPHERE) typeStr = "sphere";
                else if (prim->type == PRIM_CYLINDER) typeStr = "cylinder";

                if (prim->type == PRIM_SPHERE) {
                    // Sphere: only output radius
                    if (prim->hasColorOverride) {
                        fprintf(file, "%s %.2f %.2f %.2f %.2f %d %d %d %d\n",
                                typeStr, prim->x, prim->y, prim->z, prim->w,
                                prim->colorOverride.r, prim->colorOverride.g,
                                prim->colorOverride.b, prim->colorOverride.a);
                    } else {
                        fprintf(file, "%s %.2f %.2f %.2f %.2f\n",
                                typeStr, prim->x, prim->y, prim->z, prim->w);
                    }
                } else {
                    // Cube or cylinder: output all dimensions
                    if (prim->hasColorOverride) {
                        fprintf(file, "%s %.2f %.2f %.2f %.2f %.2f %.2f %d %d %d %d\n",
                                typeStr, prim->x, prim->y, prim->z,
                                prim->w, prim->h, prim->d,
                                prim->colorOverride.r, prim->colorOverride.g,
                                prim->colorOverride.b, prim->colorOverride.a);
                    } else {
                        fprintf(file, "%s %.2f %.2f %.2f %.2f %.2f %.2f\n",
                                typeStr, prim->x, prim->y, prim->z,
                                prim->w, prim->h, prim->d);
                    }
                }
            }
            fprintf(file, ".\n");
        }

        fprintf(file, "end\n\n");
    }

    fclose(file);
    printf("Saved %d custom monsters to %s\n", monsterCount, filepath);
    return true;
}

int AddCustomMonster(CustomMonster* monsters, int* monsterCount, int maxMonsters,
                     const CustomMonster* newMonster) {
    if (*monsterCount >= maxMonsters) return -1;

    int index = *monsterCount;
    monsters[index] = *newMonster;
    monsters[index].loaded = true;
    (*monsterCount)++;
    return index;
}

bool RemoveCustomMonster(CustomMonster* monsters, int* monsterCount, int index) {
    if (index < 0 || index >= *monsterCount) return false;

    // Shift remaining monsters down
    for (int i = index; i < *monsterCount - 1; i++) {
        monsters[i] = monsters[i + 1];
    }
    (*monsterCount)--;
    return true;
}

void GenerateMonsterID(char* buffer, int bufferSize, const char* monsterName) {
    // Convert name to lowercase slug: "Forest Fox" -> "forest_fox"
    int j = 0;
    for (int i = 0; monsterName[i] && j < bufferSize - 1; i++) {
        char c = monsterName[i];
        if (c >= 'A' && c <= 'Z') {
            buffer[j++] = c + ('a' - 'A');  // lowercase
        } else if (c >= 'a' && c <= 'z') {
            buffer[j++] = c;
        } else if (c >= '0' && c <= '9') {
            buffer[j++] = c;
        } else if (c == ' ' || c == '-') {
            if (j > 0 && buffer[j-1] != '_') {
                buffer[j++] = '_';  // replace spaces/dashes with underscore
            }
        }
        // Skip other characters
    }
    // Remove trailing underscore if any
    if (j > 0 && buffer[j-1] == '_') j--;
    buffer[j] = '\0';

    // If empty, use fallback
    if (j == 0) {
        snprintf(buffer, bufferSize, "monster");
    }
}

int FindCustomMonsterByID(const CustomMonster* monsters, int monsterCount, const char* id) {
    for (int i = 0; i < monsterCount; i++) {
        if (monsters[i].loaded && strcmp(monsters[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}
