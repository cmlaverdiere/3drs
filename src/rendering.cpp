#include "rendering.h"
#include "game_init.h"
#include "rlgl.h"
#include <cmath>

// Helper: Draw a cube using the model (proper normals)
void DrawModelCube(const EntityModels* models, Vector3 pos, float width, float height, float depth, Color color) {
    DrawModelEx(models->cube, pos, (Vector3){0, 1, 0}, 0.0f, (Vector3){width, height, depth}, color);
}

// Helper: Draw a sphere using the model (proper normals)
void DrawModelSphere(const EntityModels* models, Vector3 pos, float radius, Color color) {
    DrawModelEx(models->sphere, pos, (Vector3){0, 1, 0}, 0.0f, (Vector3){radius, radius, radius}, color);
}

// Helper: Draw a cylinder using the model (proper normals)
// Note: GenMeshCylinder creates unit radius, unit height cylinder
void DrawModelCylinder(const EntityModels* models, Vector3 pos, float radiusBottom, float radiusTop, float height, Color color) {
    // Average radius for scaling (GenMeshCylinder doesn't support different top/bottom)
    float radius = (radiusBottom + radiusTop) * 0.5f;
    DrawModelEx(models->cylinder, pos, (Vector3){0, 1, 0}, 0.0f, (Vector3){radius, height, radius}, color);
}

void DrawSword(const EntityModels* models, Vector3 pos, Color bladeColor, Color handleColor) {
    DrawModelCube(models, (Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.08f, 0.05f, 0.6f, bladeColor);
    DrawModelCube(models, (Vector3){pos.x, pos.y + 0.05f, pos.z - 0.35f}, 0.06f, 0.08f, 0.15f, handleColor);
    DrawModelCube(models, (Vector3){pos.x, pos.y + 0.05f, pos.z - 0.25f}, 0.2f, 0.04f, 0.04f, handleColor);
}

void DrawTroll(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted) {
    Color trollSkin = { 100, 140, 100, 255 };
    Color trollDark = { 70, 100, 70, 255 };
    Color eyeColor = { 200, 50, 50, 255 };
    Color eyeWhite = { 220, 220, 180, 255 };
    Color browColor = { 50, 70, 50, 255 };
    Color mouthColor = { 40, 30, 30, 255 };

    // Apply rotation around Y axis at position
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    // Draw troll at origin (will be transformed by matrix)
    Vector3 origin = {0, 0, 0};

    // Body
    DrawModelCube(models, (Vector3){0, 0.8f, 0}, 0.6f, 0.8f, 0.4f, trollSkin);
    // Head
    DrawModelSphere(models, (Vector3){0, 1.5f, 0}, 0.35f, trollSkin);

    // Face - angry expression
    float headY = 1.5f;
    float faceZ = 0.30f;

    // Eye whites
    DrawModelSphere(models, (Vector3){-0.10f, headY + 0.05f, faceZ}, 0.07f, eyeWhite);
    DrawModelSphere(models, (Vector3){0.10f, headY + 0.05f, faceZ}, 0.07f, eyeWhite);

    // Pupils
    DrawModelSphere(models, (Vector3){-0.10f, headY + 0.05f, faceZ + 0.04f}, 0.04f, eyeColor);
    DrawModelSphere(models, (Vector3){0.10f, headY + 0.05f, faceZ + 0.04f}, 0.04f, eyeColor);

    // Angry eyebrows
    DrawModelCube(models, (Vector3){-0.12f, headY + 0.15f, faceZ}, 0.10f, 0.03f, 0.02f, browColor);
    DrawModelCube(models, (Vector3){-0.06f, headY + 0.12f, faceZ}, 0.06f, 0.03f, 0.02f, browColor);
    DrawModelCube(models, (Vector3){0.12f, headY + 0.15f, faceZ}, 0.10f, 0.03f, 0.02f, browColor);
    DrawModelCube(models, (Vector3){0.06f, headY + 0.12f, faceZ}, 0.06f, 0.03f, 0.02f, browColor);

    // Scowling mouth
    DrawModelCube(models, (Vector3){0, headY - 0.12f, faceZ}, 0.14f, 0.03f, 0.02f, mouthColor);
    DrawModelCube(models, (Vector3){-0.08f, headY - 0.10f, faceZ}, 0.03f, 0.03f, 0.02f, mouthColor);
    DrawModelCube(models, (Vector3){0.08f, headY - 0.10f, faceZ}, 0.03f, 0.03f, 0.02f, mouthColor);

    // Arms
    DrawModelCube(models, (Vector3){-0.45f, 0.8f, 0}, 0.2f, 0.6f, 0.2f, trollDark);
    DrawModelCube(models, (Vector3){0.45f, 0.8f, 0}, 0.2f, 0.6f, 0.2f, trollDark);
    // Legs
    DrawModelCube(models, (Vector3){-0.15f, 0.2f, 0}, 0.2f, 0.4f, 0.2f, trollDark);
    DrawModelCube(models, (Vector3){0.15f, 0.2f, 0}, 0.2f, 0.4f, 0.2f, trollDark);

    (void)highlighted;  // Wireframe outline removed

    rlPopMatrix();
}

void DrawCow(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted) {
    Color cowBody = { 240, 240, 240, 255 };      // White body
    Color cowSpots = { 40, 30, 30, 255 };        // Black spots
    Color cowPink = { 255, 180, 180, 255 };      // Pink udder/nose
    Color cowHooves = { 60, 50, 40, 255 };       // Dark hooves
    Color eyeWhite = { 255, 255, 255, 255 };
    Color eyeBlack = { 20, 20, 20, 255 };

    // Apply rotation around Y axis at position
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    // Body (horizontal, cow-like)
    DrawModelCube(models, (Vector3){0, 0.6f, 0}, 0.6f, 0.5f, 1.0f, cowBody);

    // Spots on body
    DrawModelCube(models, (Vector3){0.2f, 0.7f, 0.1f}, 0.2f, 0.2f, 0.3f, cowSpots);
    DrawModelCube(models, (Vector3){-0.15f, 0.65f, -0.2f}, 0.15f, 0.15f, 0.25f, cowSpots);

    // Head
    DrawModelCube(models, (Vector3){0, 0.7f, 0.65f}, 0.35f, 0.35f, 0.3f, cowBody);

    // Snout/nose
    DrawModelCube(models, (Vector3){0, 0.6f, 0.85f}, 0.2f, 0.15f, 0.1f, cowPink);

    // Eyes
    DrawModelSphere(models, (Vector3){-0.12f, 0.8f, 0.75f}, 0.05f, eyeWhite);
    DrawModelSphere(models, (Vector3){0.12f, 0.8f, 0.75f}, 0.05f, eyeWhite);
    DrawModelSphere(models, (Vector3){-0.12f, 0.8f, 0.78f}, 0.025f, eyeBlack);
    DrawModelSphere(models, (Vector3){0.12f, 0.8f, 0.78f}, 0.025f, eyeBlack);

    // Ears
    DrawModelCube(models, (Vector3){-0.2f, 0.85f, 0.55f}, 0.1f, 0.05f, 0.1f, cowBody);
    DrawModelCube(models, (Vector3){0.2f, 0.85f, 0.55f}, 0.1f, 0.05f, 0.1f, cowBody);

    // Legs (4 legs)
    float legHeight = 0.35f;
    DrawModelCube(models, (Vector3){-0.2f, legHeight/2, 0.35f}, 0.1f, legHeight, 0.1f, cowBody);
    DrawModelCube(models, (Vector3){0.2f, legHeight/2, 0.35f}, 0.1f, legHeight, 0.1f, cowBody);
    DrawModelCube(models, (Vector3){-0.2f, legHeight/2, -0.35f}, 0.1f, legHeight, 0.1f, cowBody);
    DrawModelCube(models, (Vector3){0.2f, legHeight/2, -0.35f}, 0.1f, legHeight, 0.1f, cowBody);

    // Hooves
    DrawModelCube(models, (Vector3){-0.2f, 0.03f, 0.35f}, 0.1f, 0.06f, 0.1f, cowHooves);
    DrawModelCube(models, (Vector3){0.2f, 0.03f, 0.35f}, 0.1f, 0.06f, 0.1f, cowHooves);
    DrawModelCube(models, (Vector3){-0.2f, 0.03f, -0.35f}, 0.1f, 0.06f, 0.1f, cowHooves);
    DrawModelCube(models, (Vector3){0.2f, 0.03f, -0.35f}, 0.1f, 0.06f, 0.1f, cowHooves);

    // Udder
    DrawModelSphere(models, (Vector3){0, 0.35f, -0.1f}, 0.12f, cowPink);

    // Tail
    DrawModelCube(models, (Vector3){0, 0.7f, -0.6f}, 0.04f, 0.04f, 0.2f, cowBody);
    DrawModelCube(models, (Vector3){0, 0.5f, -0.7f}, 0.06f, 0.15f, 0.04f, cowSpots);

    (void)highlighted;

    rlPopMatrix();
}

void DrawScorpion(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted) {
    Color bodyColor = { 139, 90, 43, 255 };       // Dark brown
    Color bodyDark = { 101, 67, 33, 255 };        // Darker brown
    Color clawColor = { 160, 100, 50, 255 };      // Lighter brown for claws
    Color stingerColor = { 80, 50, 30, 255 };     // Very dark for stinger tip

    // Apply rotation around Y axis at position
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    // Body segments (raised slightly for visibility)
    float baseY = 0.25f;
    DrawModelCube(models, (Vector3){0, baseY, 0}, 0.4f, 0.15f, 0.5f, bodyColor);
    DrawModelCube(models, (Vector3){0, baseY - 0.03f, 0.3f}, 0.3f, 0.12f, 0.2f, bodyDark);

    // Legs (4 pairs)
    float legY = baseY - 0.07f;
    DrawModelCube(models, (Vector3){-0.25f, legY, 0.15f}, 0.15f, 0.05f, 0.06f, bodyDark);
    DrawModelCube(models, (Vector3){0.25f, legY, 0.15f}, 0.15f, 0.05f, 0.06f, bodyDark);
    DrawModelCube(models, (Vector3){-0.28f, legY, 0.0f}, 0.18f, 0.05f, 0.06f, bodyDark);
    DrawModelCube(models, (Vector3){0.28f, legY, 0.0f}, 0.18f, 0.05f, 0.06f, bodyDark);
    DrawModelCube(models, (Vector3){-0.28f, legY, -0.15f}, 0.18f, 0.05f, 0.06f, bodyDark);
    DrawModelCube(models, (Vector3){0.28f, legY, -0.15f}, 0.18f, 0.05f, 0.06f, bodyDark);
    DrawModelCube(models, (Vector3){-0.22f, legY, -0.25f}, 0.12f, 0.05f, 0.06f, bodyDark);
    DrawModelCube(models, (Vector3){0.22f, legY, -0.25f}, 0.12f, 0.05f, 0.06f, bodyDark);

    // Pincers/claws
    float clawY = baseY - 0.03f;
    DrawModelCube(models, (Vector3){-0.2f, clawY, 0.45f}, 0.08f, 0.08f, 0.2f, clawColor);
    DrawModelCube(models, (Vector3){-0.25f, clawY, 0.58f}, 0.12f, 0.06f, 0.08f, clawColor);
    DrawModelCube(models, (Vector3){-0.18f, clawY, 0.58f}, 0.06f, 0.06f, 0.1f, clawColor);
    DrawModelCube(models, (Vector3){0.2f, clawY, 0.45f}, 0.08f, 0.08f, 0.2f, clawColor);
    DrawModelCube(models, (Vector3){0.25f, clawY, 0.58f}, 0.12f, 0.06f, 0.08f, clawColor);
    DrawModelCube(models, (Vector3){0.18f, clawY, 0.58f}, 0.06f, 0.06f, 0.1f, clawColor);

    // Tail
    DrawModelCube(models, (Vector3){0, baseY + 0.03f, -0.35f}, 0.15f, 0.12f, 0.15f, bodyColor);
    DrawModelCube(models, (Vector3){0, baseY + 0.13f, -0.48f}, 0.12f, 0.1f, 0.12f, bodyColor);
    DrawModelCube(models, (Vector3){0, baseY + 0.27f, -0.55f}, 0.1f, 0.1f, 0.1f, bodyColor);
    DrawModelCube(models, (Vector3){0, baseY + 0.40f, -0.55f}, 0.08f, 0.12f, 0.08f, bodyColor);
    DrawModelCube(models, (Vector3){0, baseY + 0.50f, -0.50f}, 0.06f, 0.1f, 0.08f, bodyColor);

    // Stinger
    DrawModelCube(models, (Vector3){0, baseY + 0.55f, -0.42f}, 0.05f, 0.08f, 0.1f, stingerColor);
    DrawModelSphere(models, (Vector3){0, baseY + 0.57f, -0.36f}, 0.04f, stingerColor);

    (void)highlighted;

    rlPopMatrix();
}

void DrawEnemy(const EntityModels* models, const Enemy& enemy, bool highlighted) {
    switch (enemy.type) {
        case ENEMY_TROLL:
            DrawTroll(models, enemy.position, enemy.facingAngle, highlighted);
            break;
        case ENEMY_COW:
            DrawCow(models, enemy.position, enemy.facingAngle, highlighted);
            break;
        case ENEMY_SCORPION:
            DrawScorpion(models, enemy.position, enemy.facingAngle, highlighted);
            break;
        default:
            DrawModelCube(models, enemy.position, 0.5f, 1.0f, 0.5f, RED);
            break;
    }
}

void DrawWorldItem(const EntityModels* models, ItemType type, Vector3 pos) {
    switch (type) {
        case ITEM_BRONZE_SHORTSWORD: {
            Color bronzeBlade = { 205, 127, 50, 255 };
            Color bronzeHandle = { 139, 90, 43, 255 };
            DrawSword(models, pos, bronzeBlade, bronzeHandle);
            break;
        }
        case ITEM_COW_HIDE: {
            Color hideColor = { 139, 90, 43, 255 };
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.02f, pos.z}, 0.4f, 0.04f, 0.5f, hideColor);
            Color spotColor = { 80, 50, 30, 255 };
            DrawModelCube(models, (Vector3){pos.x + 0.1f, pos.y + 0.03f, pos.z + 0.1f}, 0.1f, 0.02f, 0.1f, spotColor);
            DrawModelCube(models, (Vector3){pos.x - 0.1f, pos.y + 0.03f, pos.z - 0.1f}, 0.08f, 0.02f, 0.12f, spotColor);
            break;
        }
        case ITEM_BONES: {
            Color boneColor = { 230, 220, 200, 255 };
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.08f, 0.08f, 0.4f, boneColor);
            DrawModelSphere(models, (Vector3){pos.x, pos.y + 0.05f, pos.z + 0.2f}, 0.06f, boneColor);
            DrawModelSphere(models, (Vector3){pos.x, pos.y + 0.05f, pos.z - 0.2f}, 0.06f, boneColor);
            break;
        }
        case ITEM_GIL: {
            Color goldColor = { 255, 215, 0, 255 };
            Color goldDark = { 200, 160, 0, 255 };
            DrawModelCylinder(models, (Vector3){pos.x, pos.y, pos.z}, 0.15f, 0.15f, 0.05f, goldColor);
            DrawModelCylinder(models, (Vector3){pos.x + 0.05f, pos.y + 0.03f, pos.z + 0.05f}, 0.1f, 0.1f, 0.04f, goldDark);
            DrawModelCylinder(models, (Vector3){pos.x - 0.03f, pos.y + 0.05f, pos.z - 0.03f}, 0.08f, 0.08f, 0.03f, goldColor);
            break;
        }
        case ITEM_BRONZE_AXE: {
            Color bronzeHead = { 205, 127, 50, 255 };
            Color woodHandle = { 101, 67, 33, 255 };
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.04f, pos.z}, 0.5f, 0.06f, 0.06f, woodHandle);
            DrawModelCube(models, (Vector3){pos.x + 0.2f, pos.y + 0.08f, pos.z}, 0.15f, 0.12f, 0.25f, bronzeHead);
            break;
        }
        case ITEM_LOGS: {
            Color barkColor = { 101, 67, 33, 255 };
            DrawModelCylinder(models, (Vector3){pos.x, pos.y, pos.z}, 0.12f, 0.12f, 0.5f, barkColor);
            break;
        }
        case ITEM_CHITIN: {
            Color chitinColor = { 101, 67, 33, 255 };
            Color chitinDark = { 70, 45, 20, 255 };
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.04f, pos.z}, 0.25f, 0.06f, 0.35f, chitinColor);
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.07f, pos.z}, 0.2f, 0.04f, 0.3f, chitinDark);
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.08f, pos.z - 0.08f}, 0.18f, 0.02f, 0.05f, chitinDark);
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.08f, pos.z + 0.08f}, 0.18f, 0.02f, 0.05f, chitinDark);
            break;
        }
        case ITEM_IRON_2H_SWORD: {
            // Iron 2H sword - larger blade, steel coloring
            Color ironBlade = { 180, 180, 190, 255 };
            Color ironDark = { 120, 120, 130, 255 };
            Color leatherGrip = { 80, 50, 30, 255 };
            // Long blade
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.06f, pos.z + 0.1f}, 0.12f, 0.06f, 0.9f, ironBlade);
            // Fuller (groove in blade)
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.07f, pos.z + 0.15f}, 0.04f, 0.02f, 0.7f, ironDark);
            // Crossguard
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.06f, pos.z - 0.38f}, 0.35f, 0.05f, 0.08f, ironDark);
            // Long grip (two-handed)
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.06f, pos.z - 0.55f}, 0.07f, 0.07f, 0.28f, leatherGrip);
            // Pommel
            DrawModelSphere(models, (Vector3){pos.x, pos.y + 0.06f, pos.z - 0.72f}, 0.06f, ironDark);
            break;
        }
        default:
            DrawModelCube(models, pos, 0.2f, 0.2f, 0.2f, RED);
            break;
    }
}

void DrawTree(const EntityModels* models, Vector3 pos, bool highlighted) {
    Color trunkColor = { 101, 67, 33, 255 };
    Color leavesColor = { 34, 139, 34, 255 };
    Color leavesDark = { 20, 100, 20, 255 };

    // Trunk
    DrawModelCylinder(models, (Vector3){pos.x, pos.y, pos.z}, 0.3f, 0.4f, 2.5f, trunkColor);

    // Leaves (layered spheres)
    DrawModelSphere(models, (Vector3){pos.x, pos.y + 3.5f, pos.z}, 1.5f, leavesColor);
    DrawModelSphere(models, (Vector3){pos.x - 0.5f, pos.y + 3.0f, pos.z + 0.5f}, 1.0f, leavesDark);
    DrawModelSphere(models, (Vector3){pos.x + 0.5f, pos.y + 3.0f, pos.z - 0.5f}, 1.0f, leavesDark);
    DrawModelSphere(models, (Vector3){pos.x, pos.y + 4.2f, pos.z}, 0.8f, leavesColor);

    if (highlighted) {
        Color outlineColor = { 255, 255, 0, 255 };
        DrawCylinderWires((Vector3){pos.x, pos.y, pos.z}, 0.35f, 0.45f, 2.5f, 8, outlineColor);
        DrawSphereWires((Vector3){pos.x, pos.y + 3.5f, pos.z}, 1.55f, 8, 8, outlineColor);
    }
}
