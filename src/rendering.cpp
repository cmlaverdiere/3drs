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

void DrawScimitar(const EntityModels* models, Vector3 pos, Color bladeColor, Color handleColor) {
    // Curved blade - multiple segments to approximate curve
    DrawModelCube(models, (Vector3){pos.x, pos.y + 0.05f, pos.z + 0.1f}, 0.06f, 0.04f, 0.25f, bladeColor);
    DrawModelCube(models, (Vector3){pos.x - 0.03f, pos.y + 0.05f, pos.z + 0.32f}, 0.05f, 0.04f, 0.2f, bladeColor);
    DrawModelCube(models, (Vector3){pos.x - 0.06f, pos.y + 0.05f, pos.z + 0.48f}, 0.04f, 0.04f, 0.12f, bladeColor);
    // Handle
    DrawModelCube(models, (Vector3){pos.x, pos.y + 0.05f, pos.z - 0.1f}, 0.05f, 0.06f, 0.15f, handleColor);
    // Guard
    DrawModelCube(models, (Vector3){pos.x, pos.y + 0.05f, pos.z - 0.02f}, 0.12f, 0.03f, 0.03f, handleColor);
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

void DrawBandit(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted) {
    // Human bandit - hooded figure with dark clothes
    Color skinColor = { 220, 180, 150, 255 };     // Skin
    Color hoodColor = { 50, 40, 35, 255 };        // Dark hood/cloak
    Color shirtColor = { 80, 60, 50, 255 };       // Dark brown shirt
    Color pantsColor = { 40, 35, 30, 255 };       // Very dark pants
    Color beltColor = { 100, 70, 40, 255 };       // Leather belt
    Color eyeWhite = { 255, 255, 255, 255 };
    Color eyeBlack = { 20, 20, 20, 255 };

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    // Legs
    float legHeight = 0.7f;
    float legWidth = 0.14f;
    DrawModelCube(models, (Vector3){-0.1f, legHeight * 0.5f, 0}, legWidth, legHeight, legWidth, pantsColor);
    DrawModelCube(models, (Vector3){0.1f, legHeight * 0.5f, 0}, legWidth, legHeight, legWidth, pantsColor);

    // Torso
    float torsoBottom = legHeight;
    float torsoHeight = 0.55f;
    DrawModelCube(models, (Vector3){0, torsoBottom + torsoHeight * 0.5f, 0}, 0.4f, torsoHeight, 0.22f, shirtColor);

    // Belt
    DrawModelCube(models, (Vector3){0, torsoBottom + 0.05f, 0}, 0.42f, 0.08f, 0.24f, beltColor);

    // Arms
    float armHeight = 0.5f;
    float shoulderY = torsoBottom + torsoHeight * 0.85f;
    DrawModelCube(models, (Vector3){-0.28f, shoulderY - armHeight * 0.5f, 0}, 0.1f, armHeight, 0.1f, shirtColor);
    DrawModelCube(models, (Vector3){0.28f, shoulderY - armHeight * 0.5f, 0}, 0.1f, armHeight, 0.1f, shirtColor);

    // Hands
    DrawModelSphere(models, (Vector3){-0.28f, shoulderY - armHeight - 0.03f, 0}, 0.06f, skinColor);
    DrawModelSphere(models, (Vector3){0.28f, shoulderY - armHeight - 0.03f, 0}, 0.06f, skinColor);

    // Head
    float headY = torsoBottom + torsoHeight + 0.18f;
    DrawModelSphere(models, (Vector3){0, headY, 0}, 0.18f, skinColor);

    // Hood (cube around head)
    DrawModelCube(models, (Vector3){0, headY + 0.05f, -0.05f}, 0.4f, 0.3f, 0.25f, hoodColor);
    DrawModelCube(models, (Vector3){0, headY + 0.15f, 0}, 0.38f, 0.15f, 0.35f, hoodColor);

    // Eyes (visible under hood)
    float faceZ = 0.16f;
    DrawModelSphere(models, (Vector3){-0.05f, headY + 0.02f, faceZ}, 0.025f, eyeWhite);
    DrawModelSphere(models, (Vector3){0.05f, headY + 0.02f, faceZ}, 0.025f, eyeWhite);
    DrawModelSphere(models, (Vector3){-0.05f, headY + 0.02f, faceZ + 0.01f}, 0.012f, eyeBlack);
    DrawModelSphere(models, (Vector3){0.05f, headY + 0.02f, faceZ + 0.01f}, 0.012f, eyeBlack);

    // Dagger at hip
    Color daggerBlade = { 180, 180, 190, 255 };
    DrawModelCube(models, (Vector3){0.22f, torsoBottom, 0.1f}, 0.03f, 0.03f, 0.2f, daggerBlade);

    (void)highlighted;
    rlPopMatrix();
}

void DrawSandGolem(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted) {
    // Large sand-colored elemental creature
    Color sandLight = { 210, 180, 140, 255 };     // Light sand
    Color sandDark = { 180, 150, 110, 255 };      // Darker sand
    Color sandDeep = { 150, 120, 80, 255 };       // Deep sand
    Color eyeGlow = { 255, 200, 100, 255 };       // Glowing amber eyes

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    // Large legs (thick pillars)
    float legHeight = 0.9f;
    DrawModelCube(models, (Vector3){-0.25f, legHeight * 0.5f, 0}, 0.35f, legHeight, 0.35f, sandDark);
    DrawModelCube(models, (Vector3){0.25f, legHeight * 0.5f, 0}, 0.35f, legHeight, 0.35f, sandDark);

    // Massive torso
    float torsoBottom = legHeight * 0.8f;
    float torsoHeight = 1.0f;
    DrawModelCube(models, (Vector3){0, torsoBottom + torsoHeight * 0.5f, 0}, 0.9f, torsoHeight, 0.6f, sandLight);

    // Chest detail
    DrawModelCube(models, (Vector3){0, torsoBottom + torsoHeight * 0.6f, 0.25f}, 0.5f, 0.4f, 0.15f, sandDeep);

    // Massive arms
    float shoulderY = torsoBottom + torsoHeight * 0.9f;
    float armHeight = 0.9f;
    DrawModelCube(models, (Vector3){-0.6f, shoulderY - armHeight * 0.4f, 0}, 0.25f, armHeight, 0.25f, sandDark);
    DrawModelCube(models, (Vector3){0.6f, shoulderY - armHeight * 0.4f, 0}, 0.25f, armHeight, 0.25f, sandDark);

    // Fists
    DrawModelSphere(models, (Vector3){-0.6f, shoulderY - armHeight - 0.1f, 0}, 0.2f, sandDeep);
    DrawModelSphere(models, (Vector3){0.6f, shoulderY - armHeight - 0.1f, 0}, 0.2f, sandDeep);

    // Head (rough rocky shape)
    float headY = torsoBottom + torsoHeight + 0.35f;
    DrawModelCube(models, (Vector3){0, headY, 0}, 0.5f, 0.45f, 0.4f, sandLight);
    DrawModelCube(models, (Vector3){0, headY + 0.1f, 0}, 0.4f, 0.25f, 0.35f, sandDark);

    // Glowing eyes
    float faceZ = 0.2f;
    DrawModelSphere(models, (Vector3){-0.12f, headY + 0.05f, faceZ}, 0.07f, eyeGlow);
    DrawModelSphere(models, (Vector3){0.12f, headY + 0.05f, faceZ}, 0.07f, eyeGlow);

    // Rough surface details
    DrawModelCube(models, (Vector3){0.3f, torsoBottom + 0.3f, 0.2f}, 0.15f, 0.15f, 0.1f, sandDeep);
    DrawModelCube(models, (Vector3){-0.25f, torsoBottom + 0.5f, 0.22f}, 0.12f, 0.12f, 0.08f, sandDeep);

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
        case ENEMY_BANDIT:
            DrawBandit(models, enemy.position, enemy.facingAngle, highlighted);
            break;
        case ENEMY_SAND_GOLEM:
            DrawSandGolem(models, enemy.position, enemy.facingAngle, highlighted);
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
        case ITEM_BANDIT_ORDERS: {
            // Scroll/paper with wax seal
            Color parchment = { 240, 230, 200, 255 };
            Color parchmentDark = { 200, 190, 160, 255 };
            Color waxSeal = { 150, 40, 40, 255 };
            // Rolled parchment
            DrawModelCylinder(models, (Vector3){pos.x, pos.y + 0.03f, pos.z}, 0.08f, 0.08f, 0.35f, parchment);
            // Darker bands at ends
            DrawModelCylinder(models, (Vector3){pos.x, pos.y + 0.03f, pos.z + 0.15f}, 0.09f, 0.09f, 0.04f, parchmentDark);
            DrawModelCylinder(models, (Vector3){pos.x, pos.y + 0.03f, pos.z - 0.15f}, 0.09f, 0.09f, 0.04f, parchmentDark);
            // Wax seal
            DrawModelCylinder(models, (Vector3){pos.x, pos.y + 0.1f, pos.z}, 0.06f, 0.06f, 0.02f, waxSeal);
            break;
        }
        case ITEM_DESERT_ARTIFACT: {
            // Glowing golden artifact
            Color gold = { 255, 200, 50, 255 };
            Color goldDark = { 200, 150, 30, 255 };
            Color glow = { 255, 230, 150, 255 };
            // Base pyramid shape (using cubes)
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.25f, 0.1f, 0.25f, goldDark);
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.12f, pos.z}, 0.18f, 0.08f, 0.18f, gold);
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.18f, pos.z}, 0.1f, 0.06f, 0.1f, gold);
            // Glowing orb on top
            DrawModelSphere(models, (Vector3){pos.x, pos.y + 0.25f, pos.z}, 0.06f, glow);
            break;
        }
        case ITEM_SILK: {
            // Rolled silk fabric
            Color silkColor = { 200, 50, 80, 255 };  // Rich red silk
            Color silkHighlight = { 230, 100, 120, 255 };
            DrawModelCylinder(models, (Vector3){pos.x, pos.y + 0.04f, pos.z}, 0.1f, 0.1f, 0.4f, silkColor);
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.08f, pos.z}, 0.08f, 0.02f, 0.35f, silkHighlight);
            break;
        }
        case ITEM_SPICE: {
            // Spice pouch/bag
            Color bagColor = { 160, 120, 80, 255 };
            Color spiceColor = { 200, 100, 30, 255 };  // Orange spice
            // Bag
            DrawModelSphere(models, (Vector3){pos.x, pos.y + 0.1f, pos.z}, 0.12f, bagColor);
            // Tied top
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.2f, pos.z}, 0.04f, 0.06f, 0.04f, bagColor);
            // Spice visible at opening
            DrawModelSphere(models, (Vector3){pos.x, pos.y + 0.18f, pos.z}, 0.04f, spiceColor);
            break;
        }
        case ITEM_STEEL_SCIMITAR: {
            Color steelBlade = { 180, 180, 190, 255 };
            Color steelHandle = { 100, 80, 60, 255 };
            DrawScimitar(models, pos, steelBlade, steelHandle);
            break;
        }
        case ITEM_MITHRIL_SCIMITAR: {
            Color mithrilBlade = { 100, 140, 180, 255 };  // Bluish tint
            Color mithrilHandle = { 80, 100, 120, 255 };
            DrawScimitar(models, pos, mithrilBlade, mithrilHandle);
            break;
        }
        case ITEM_ADAMANT_SCIMITAR: {
            Color adamantBlade = { 80, 160, 80, 255 };    // Green tint
            Color adamantHandle = { 60, 100, 60, 255 };
            DrawScimitar(models, pos, adamantBlade, adamantHandle);
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

void DrawHumanoid(const EntityModels* models, Vector3 pos, float facingAngle,
                  Color skinColor, Color shirtColor, Color pantsColor, float heightScale) {
    // Humanoid proportions (scaled by heightScale) - total height ~1.7 units
    const float HEAD_RADIUS = 0.2f * heightScale;
    const float TORSO_WIDTH = 0.45f * heightScale;
    const float TORSO_HEIGHT = 0.6f * heightScale;
    const float TORSO_DEPTH = 0.25f * heightScale;
    const float ARM_WIDTH = 0.12f * heightScale;
    const float ARM_HEIGHT = 0.55f * heightScale;
    const float LEG_WIDTH = 0.15f * heightScale;
    const float LEG_HEIGHT = 0.75f * heightScale;
    const float HAND_RADIUS = 0.07f * heightScale;

    // Apply rotation around Y axis at position
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    // Calculate Y positions (standing on ground)
    float legTopY = LEG_HEIGHT;
    float torsoBottomY = legTopY;
    float torsoMidY = torsoBottomY + TORSO_HEIGHT * 0.5f;
    float shoulderY = torsoBottomY + TORSO_HEIGHT * 0.85f;
    float headY = torsoBottomY + TORSO_HEIGHT + HEAD_RADIUS * 0.8f;

    // Head (sphere) - skin color
    DrawModelSphere(models, (Vector3){0, headY, 0}, HEAD_RADIUS, skinColor);

    // Face - simple eyes
    float faceZ = HEAD_RADIUS * 0.85f;
    Color eyeWhite = { 255, 255, 255, 255 };
    Color eyeBlack = { 30, 30, 30, 255 };
    DrawModelSphere(models, (Vector3){-0.04f * heightScale, headY + 0.02f * heightScale, faceZ}, 0.025f * heightScale, eyeWhite);
    DrawModelSphere(models, (Vector3){0.04f * heightScale, headY + 0.02f * heightScale, faceZ}, 0.025f * heightScale, eyeWhite);
    DrawModelSphere(models, (Vector3){-0.04f * heightScale, headY + 0.02f * heightScale, faceZ + 0.01f * heightScale}, 0.012f * heightScale, eyeBlack);
    DrawModelSphere(models, (Vector3){0.04f * heightScale, headY + 0.02f * heightScale, faceZ + 0.01f * heightScale}, 0.012f * heightScale, eyeBlack);

    // Torso (cube) - shirt color
    DrawModelCube(models, (Vector3){0, torsoMidY, 0}, TORSO_WIDTH, TORSO_HEIGHT, TORSO_DEPTH, shirtColor);

    // Arms (cubes hanging at sides) - shirt color
    float armOffsetX = TORSO_WIDTH * 0.5f + ARM_WIDTH * 0.5f;
    float armMidY = shoulderY - ARM_HEIGHT * 0.5f;

    // Left arm
    DrawModelCube(models, (Vector3){-armOffsetX, armMidY, 0}, ARM_WIDTH, ARM_HEIGHT, ARM_WIDTH, shirtColor);
    // Left hand
    DrawModelSphere(models, (Vector3){-armOffsetX, armMidY - ARM_HEIGHT * 0.5f - HAND_RADIUS * 0.5f, 0}, HAND_RADIUS, skinColor);

    // Right arm
    DrawModelCube(models, (Vector3){armOffsetX, armMidY, 0}, ARM_WIDTH, ARM_HEIGHT, ARM_WIDTH, shirtColor);
    // Right hand
    DrawModelSphere(models, (Vector3){armOffsetX, armMidY - ARM_HEIGHT * 0.5f - HAND_RADIUS * 0.5f, 0}, HAND_RADIUS, skinColor);

    // Legs (cubes) - pants color
    float legOffsetX = LEG_WIDTH * 0.7f;
    float legMidY = LEG_HEIGHT * 0.5f;

    // Left leg
    DrawModelCube(models, (Vector3){-legOffsetX, legMidY, 0}, LEG_WIDTH, LEG_HEIGHT, LEG_WIDTH, pantsColor);

    // Right leg
    DrawModelCube(models, (Vector3){legOffsetX, legMidY, 0}, LEG_WIDTH, LEG_HEIGHT, LEG_WIDTH, pantsColor);

    // Shoes (darker)
    Color shoeColor = { 40, 30, 25, 255 };
    float shoeHeight = 0.08f * heightScale;
    DrawModelCube(models, (Vector3){-legOffsetX, shoeHeight * 0.5f, 0.03f * heightScale}, LEG_WIDTH * 1.1f, shoeHeight, LEG_WIDTH * 1.3f, shoeColor);
    DrawModelCube(models, (Vector3){legOffsetX, shoeHeight * 0.5f, 0.03f * heightScale}, LEG_WIDTH * 1.1f, shoeHeight, LEG_WIDTH * 1.3f, shoeColor);

    rlPopMatrix();
}

void DrawNPC(const EntityModels* models, const NPC& npc) {
    if (!npc.active) return;

    const NPCConfig& config = NPC_CONFIGS[npc.type];
    DrawHumanoid(models, npc.position, npc.facingAngle,
                 config.skinColor, config.shirtColor, config.pantsColor, config.height);
}

void DrawNPCs(const EntityModels* models, const NPC* npcs, int npcCount) {
    for (int i = 0; i < npcCount; i++) {
        DrawNPC(models, npcs[i]);
    }
}
