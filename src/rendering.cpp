#include "rendering.h"
#include "game_init.h"
#include "math_utils.h"
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

void DrawDemon(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted) {
    // Fire demon - red-skinned humanoid with horns and bat-like wings
    Color demonSkin = { 140, 40, 40, 255 };      // Dark red skin
    Color demonDark = { 80, 20, 20, 255 };       // Darker red
    Color hornColor = { 40, 30, 30, 255 };       // Black horns
    Color eyeGlow = { 255, 150, 50, 255 };       // Fiery orange eyes
    Color wingMembrane = { 100, 30, 30, 200 };   // Semi-transparent wings

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    float scale = 1.3f;  // Larger than humanoid

    // Legs
    DrawModelCube(models, (Vector3){-0.12f * scale, 0.4f * scale, 0}, 0.18f * scale, 0.8f * scale, 0.18f * scale, demonDark);
    DrawModelCube(models, (Vector3){0.12f * scale, 0.4f * scale, 0}, 0.18f * scale, 0.8f * scale, 0.18f * scale, demonDark);

    // Muscular torso
    DrawModelCube(models, (Vector3){0, 1.1f * scale, 0}, 0.5f * scale, 0.7f * scale, 0.3f * scale, demonSkin);

    // Arms
    DrawModelCube(models, (Vector3){-0.35f * scale, 1.0f * scale, 0}, 0.14f * scale, 0.6f * scale, 0.14f * scale, demonSkin);
    DrawModelCube(models, (Vector3){0.35f * scale, 1.0f * scale, 0}, 0.14f * scale, 0.6f * scale, 0.14f * scale, demonSkin);

    // Head
    DrawModelSphere(models, (Vector3){0, 1.6f * scale, 0}, 0.22f * scale, demonSkin);

    // Horns (curved)
    DrawModelCube(models, (Vector3){-0.12f * scale, 1.75f * scale, 0}, 0.05f * scale, 0.2f * scale, 0.05f * scale, hornColor);
    DrawModelCube(models, (Vector3){-0.15f * scale, 1.9f * scale, -0.05f * scale}, 0.04f * scale, 0.15f * scale, 0.04f * scale, hornColor);
    DrawModelCube(models, (Vector3){0.12f * scale, 1.75f * scale, 0}, 0.05f * scale, 0.2f * scale, 0.05f * scale, hornColor);
    DrawModelCube(models, (Vector3){0.15f * scale, 1.9f * scale, -0.05f * scale}, 0.04f * scale, 0.15f * scale, 0.04f * scale, hornColor);

    // Glowing eyes
    float faceZ = 0.2f * scale;
    DrawModelSphere(models, (Vector3){-0.06f * scale, 1.62f * scale, faceZ}, 0.04f * scale, eyeGlow);
    DrawModelSphere(models, (Vector3){0.06f * scale, 1.62f * scale, faceZ}, 0.04f * scale, eyeGlow);

    // Wings (bat-like, behind body)
    // Wing bones
    DrawModelCube(models, (Vector3){-0.4f * scale, 1.3f * scale, -0.15f * scale}, 0.5f * scale, 0.05f * scale, 0.05f * scale, hornColor);
    DrawModelCube(models, (Vector3){-0.7f * scale, 1.1f * scale, -0.2f * scale}, 0.05f * scale, 0.4f * scale, 0.05f * scale, hornColor);
    DrawModelCube(models, (Vector3){0.4f * scale, 1.3f * scale, -0.15f * scale}, 0.5f * scale, 0.05f * scale, 0.05f * scale, hornColor);
    DrawModelCube(models, (Vector3){0.7f * scale, 1.1f * scale, -0.2f * scale}, 0.05f * scale, 0.4f * scale, 0.05f * scale, hornColor);

    // Wing membrane
    DrawModelCube(models, (Vector3){-0.5f * scale, 1.1f * scale, -0.18f * scale}, 0.4f * scale, 0.5f * scale, 0.02f * scale, wingMembrane);
    DrawModelCube(models, (Vector3){0.5f * scale, 1.1f * scale, -0.18f * scale}, 0.4f * scale, 0.5f * scale, 0.02f * scale, wingMembrane);

    // Tail
    DrawModelCube(models, (Vector3){0, 0.5f * scale, -0.4f * scale}, 0.06f * scale, 0.06f * scale, 0.4f * scale, demonDark);

    (void)highlighted;
    rlPopMatrix();
}

void DrawDragon(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted) {
    // Large quadruped dragon with wings
    Color dragonScales = { 40, 80, 40, 255 };    // Dark green scales
    Color dragonBelly = { 180, 160, 100, 255 };  // Yellowish underbelly
    Color hornColor = { 60, 50, 40, 255 };       // Bone-colored horns
    Color eyeColor = { 255, 200, 50, 255 };      // Amber eyes
    Color wingMembrane = { 60, 100, 60, 180 };   // Greenish wing membrane
    Color clawColor = { 40, 35, 30, 255 };       // Dark claws

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    float scale = 2.0f;  // Large creature

    // Body (horizontal quadruped)
    DrawModelCube(models, (Vector3){0, 1.0f * scale, 0}, 0.8f * scale, 0.6f * scale, 1.5f * scale, dragonScales);
    // Underbelly
    DrawModelCube(models, (Vector3){0, 0.8f * scale, 0}, 0.6f * scale, 0.2f * scale, 1.3f * scale, dragonBelly);

    // Four legs
    float legY = 0.4f * scale;
    float legH = 0.8f * scale;
    // Front legs
    DrawModelCube(models, (Vector3){-0.35f * scale, legY, 0.5f * scale}, 0.2f * scale, legH, 0.2f * scale, dragonScales);
    DrawModelCube(models, (Vector3){0.35f * scale, legY, 0.5f * scale}, 0.2f * scale, legH, 0.2f * scale, dragonScales);
    // Back legs
    DrawModelCube(models, (Vector3){-0.35f * scale, legY, -0.5f * scale}, 0.2f * scale, legH, 0.2f * scale, dragonScales);
    DrawModelCube(models, (Vector3){0.35f * scale, legY, -0.5f * scale}, 0.2f * scale, legH, 0.2f * scale, dragonScales);

    // Claws
    DrawModelCube(models, (Vector3){-0.35f * scale, 0.05f * scale, 0.6f * scale}, 0.15f * scale, 0.1f * scale, 0.15f * scale, clawColor);
    DrawModelCube(models, (Vector3){0.35f * scale, 0.05f * scale, 0.6f * scale}, 0.15f * scale, 0.1f * scale, 0.15f * scale, clawColor);
    DrawModelCube(models, (Vector3){-0.35f * scale, 0.05f * scale, -0.4f * scale}, 0.15f * scale, 0.1f * scale, 0.15f * scale, clawColor);
    DrawModelCube(models, (Vector3){0.35f * scale, 0.05f * scale, -0.4f * scale}, 0.15f * scale, 0.1f * scale, 0.15f * scale, clawColor);

    // Neck (curved upward)
    DrawModelCube(models, (Vector3){0, 1.3f * scale, 0.9f * scale}, 0.3f * scale, 0.4f * scale, 0.4f * scale, dragonScales);
    DrawModelCube(models, (Vector3){0, 1.6f * scale, 1.1f * scale}, 0.25f * scale, 0.35f * scale, 0.35f * scale, dragonScales);

    // Head
    DrawModelCube(models, (Vector3){0, 1.8f * scale, 1.4f * scale}, 0.4f * scale, 0.35f * scale, 0.5f * scale, dragonScales);
    // Snout
    DrawModelCube(models, (Vector3){0, 1.75f * scale, 1.7f * scale}, 0.25f * scale, 0.2f * scale, 0.3f * scale, dragonScales);

    // Eyes
    DrawModelSphere(models, (Vector3){-0.15f * scale, 1.9f * scale, 1.5f * scale}, 0.08f * scale, eyeColor);
    DrawModelSphere(models, (Vector3){0.15f * scale, 1.9f * scale, 1.5f * scale}, 0.08f * scale, eyeColor);

    // Horns (back of head)
    DrawModelCube(models, (Vector3){-0.15f * scale, 2.05f * scale, 1.2f * scale}, 0.05f * scale, 0.25f * scale, 0.08f * scale, hornColor);
    DrawModelCube(models, (Vector3){0.15f * scale, 2.05f * scale, 1.2f * scale}, 0.05f * scale, 0.25f * scale, 0.08f * scale, hornColor);

    // Wings
    // Wing bones
    DrawModelCube(models, (Vector3){-0.6f * scale, 1.3f * scale, 0}, 0.8f * scale, 0.08f * scale, 0.08f * scale, hornColor);
    DrawModelCube(models, (Vector3){-1.1f * scale, 1.0f * scale, 0}, 0.08f * scale, 0.7f * scale, 0.08f * scale, hornColor);
    DrawModelCube(models, (Vector3){0.6f * scale, 1.3f * scale, 0}, 0.8f * scale, 0.08f * scale, 0.08f * scale, hornColor);
    DrawModelCube(models, (Vector3){1.1f * scale, 1.0f * scale, 0}, 0.08f * scale, 0.7f * scale, 0.08f * scale, hornColor);

    // Wing membrane
    DrawModelCube(models, (Vector3){-0.8f * scale, 1.0f * scale, -0.1f * scale}, 0.7f * scale, 0.8f * scale, 0.03f * scale, wingMembrane);
    DrawModelCube(models, (Vector3){0.8f * scale, 1.0f * scale, -0.1f * scale}, 0.7f * scale, 0.8f * scale, 0.03f * scale, wingMembrane);

    // Tail (long, segmented)
    DrawModelCube(models, (Vector3){0, 0.9f * scale, -1.0f * scale}, 0.2f * scale, 0.2f * scale, 0.6f * scale, dragonScales);
    DrawModelCube(models, (Vector3){0, 0.85f * scale, -1.5f * scale}, 0.15f * scale, 0.15f * scale, 0.5f * scale, dragonScales);
    DrawModelCube(models, (Vector3){0, 0.8f * scale, -1.9f * scale}, 0.1f * scale, 0.1f * scale, 0.4f * scale, dragonScales);
    // Tail spike
    DrawModelCube(models, (Vector3){0, 0.8f * scale, -2.2f * scale}, 0.06f * scale, 0.15f * scale, 0.15f * scale, hornColor);

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
        case ENEMY_DEMON:
            DrawDemon(models, enemy.position, enemy.facingAngle, highlighted);
            break;
        case ENEMY_DRAGON:
            DrawDragon(models, enemy.position, enemy.facingAngle, highlighted);
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
        case ITEM_OAK_LOGS: {
            // Oak logs - darker, larger than normal logs
            Color oakBark = { 80, 50, 25, 255 };
            Color oakRings = { 150, 110, 60, 255 };
            DrawModelCylinder(models, (Vector3){pos.x, pos.y, pos.z}, 0.16f, 0.16f, 0.6f, oakBark);
            // Visible rings on cut end
            DrawModelCylinder(models, (Vector3){pos.x, pos.y + 0.01f, pos.z + 0.28f}, 0.12f, 0.12f, 0.02f, oakRings);
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
        case ITEM_BRONZE_PICKAXE: {
            Color bronzeHead = { 205, 127, 50, 255 };
            Color bronzeTip = { 180, 110, 45, 255 };
            Color woodHandle = { 101, 67, 33, 255 };
            // Diagonal handle (from ground up to head)
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.15f, pos.z}, 0.08f, 0.35f, 0.08f, woodHandle);
            // Pickaxe head - horizontal bar
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.35f, pos.z}, 0.45f, 0.08f, 0.06f, bronzeHead);
            // Left point (tapered)
            DrawModelCube(models, (Vector3){pos.x - 0.25f, pos.y + 0.33f, pos.z}, 0.1f, 0.06f, 0.05f, bronzeTip);
            // Right point (tapered)
            DrawModelCube(models, (Vector3){pos.x + 0.25f, pos.y + 0.33f, pos.z}, 0.1f, 0.06f, 0.05f, bronzeTip);
            break;
        }
        case ITEM_COPPER_ORE: {
            Color copperColor = { 180, 100, 50, 255 };
            Color stoneColor = { 100, 90, 80, 255 };
            // Small ore chunk
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.1f, pos.z}, 0.22f, 0.18f, 0.2f, stoneColor);
            DrawModelSphere(models, (Vector3){pos.x + 0.05f, pos.y + 0.12f, pos.z + 0.05f}, 0.08f, copperColor);
            DrawModelSphere(models, (Vector3){pos.x - 0.04f, pos.y + 0.1f, pos.z - 0.03f}, 0.06f, copperColor);
            break;
        }
        case ITEM_TIN_ORE: {
            Color tinColor = { 150, 150, 140, 255 };
            Color stoneColor = { 90, 85, 80, 255 };
            // Small ore chunk
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.1f, pos.z}, 0.22f, 0.18f, 0.2f, stoneColor);
            DrawModelSphere(models, (Vector3){pos.x + 0.05f, pos.y + 0.12f, pos.z + 0.05f}, 0.08f, tinColor);
            DrawModelSphere(models, (Vector3){pos.x - 0.04f, pos.y + 0.1f, pos.z - 0.03f}, 0.06f, tinColor);
            break;
        }
        case ITEM_BOW: {
            Color woodColor = { 139, 90, 43, 255 };     // Brown wood
            Color stringColor = { 200, 200, 180, 255 }; // Off-white string
            // Bow stave (curved shape using angled pieces)
            // Lower limb
            DrawModelCube(models, (Vector3){pos.x - 0.08f, pos.y + 0.15f, pos.z}, 0.06f, 0.25f, 0.05f, woodColor);
            // Upper limb
            DrawModelCube(models, (Vector3){pos.x + 0.08f, pos.y + 0.45f, pos.z}, 0.06f, 0.25f, 0.05f, woodColor);
            // Center grip
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.3f, pos.z}, 0.08f, 0.12f, 0.06f, woodColor);
            // Bowstring
            DrawModelCube(models, (Vector3){pos.x - 0.12f, pos.y + 0.3f, pos.z}, 0.02f, 0.5f, 0.02f, stringColor);
            break;
        }
        case ITEM_ARROW: {
            Color shaftColor = { 160, 140, 100, 255 };  // Light wood
            Color tipColor = { 100, 100, 110, 255 };    // Iron tip
            Color fletchColor = { 200, 50, 50, 255 };   // Red feathers
            // Arrow shaft (lying flat)
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.02f, pos.z}, 0.03f, 0.03f, 0.6f, shaftColor);
            // Arrowhead
            DrawModelCube(models, (Vector3){pos.x, pos.y + 0.02f, pos.z + 0.32f}, 0.06f, 0.02f, 0.08f, tipColor);
            // Fletching (red feathers)
            DrawModelCube(models, (Vector3){pos.x + 0.03f, pos.y + 0.03f, pos.z - 0.25f}, 0.04f, 0.02f, 0.1f, fletchColor);
            DrawModelCube(models, (Vector3){pos.x - 0.03f, pos.y + 0.03f, pos.z - 0.25f}, 0.04f, 0.02f, 0.1f, fletchColor);
            break;
        }
        default:
            DrawModelCube(models, pos, 0.2f, 0.2f, 0.2f, RED);
            break;
    }
}

// Draw an evergreen/pine tree (for winter mode)
void DrawEvergreenTree(const EntityModels* models, Vector3 pos, TreeType type, bool highlighted) {
    // Darker trunk, snow-dusted pine needles
    Color trunkColor = { 60, 40, 25, 255 };
    Color pineGreen = { 20, 60, 35, 255 };
    Color pineSnow = { 180, 200, 210, 255 };  // Snow on branches

    float scale = (type == TREE_OAK) ? 1.3f : 1.0f;

    // Trunk - taller and thinner for evergreen
    float trunkHeight = 2.0f * scale;
    DrawModelCylinder(models, (Vector3){pos.x, pos.y, pos.z}, 0.2f * scale, 0.25f * scale, trunkHeight, trunkColor);

    // Conical layers of branches (bottom to top)
    float baseY = pos.y + trunkHeight * 0.5f;

    // Bottom layer - widest
    DrawModelCylinder(models, (Vector3){pos.x, baseY + 0.5f * scale, pos.z}, 1.8f * scale, 0.0f, 1.2f * scale, pineGreen);
    // Snow on bottom branches
    DrawModelCylinder(models, (Vector3){pos.x, baseY + 0.85f * scale, pos.z}, 1.4f * scale, 0.0f, 0.15f * scale, pineSnow);

    // Middle layer
    DrawModelCylinder(models, (Vector3){pos.x, baseY + 1.5f * scale, pos.z}, 1.4f * scale, 0.0f, 1.0f * scale, pineGreen);
    // Snow on middle branches
    DrawModelCylinder(models, (Vector3){pos.x, baseY + 1.8f * scale, pos.z}, 1.1f * scale, 0.0f, 0.12f * scale, pineSnow);

    // Upper layer
    DrawModelCylinder(models, (Vector3){pos.x, baseY + 2.3f * scale, pos.z}, 1.0f * scale, 0.0f, 0.9f * scale, pineGreen);
    // Snow on upper branches
    DrawModelCylinder(models, (Vector3){pos.x, baseY + 2.55f * scale, pos.z}, 0.75f * scale, 0.0f, 0.1f * scale, pineSnow);

    // Top layer - pointed
    DrawModelCylinder(models, (Vector3){pos.x, baseY + 3.0f * scale, pos.z}, 0.6f * scale, 0.0f, 0.8f * scale, pineGreen);
    // Snow cap
    DrawModelCylinder(models, (Vector3){pos.x, baseY + 3.25f * scale, pos.z}, 0.4f * scale, 0.0f, 0.08f * scale, pineSnow);

    // Snow accumulation at base
    DrawModelCylinder(models, (Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.8f * scale, 0.8f * scale, 0.1f, pineSnow);

    (void)highlighted;  // Unused - wireframe removed
}

void DrawTree(const EntityModels* models, Vector3 pos, TreeType type, bool highlighted) {
    // Winter mode - draw evergreen trees instead
    if (IsWinterMode()) {
        DrawEvergreenTree(models, pos, type, highlighted);
        return;
    }

    // Determine leaf colors based on season
    Color leavesColor, leavesDark;
    Color oakLeaves, oakLeavesDark;

    // Use position-based variation for autumn colors
    float treeHash = fmodf(fabsf(pos.x * 12.9898f + pos.z * 78.233f), 1.0f);

    if (g_currentSeason == SEASON_SPRING) {
        // Spring - fresh bright green with some yellow-green
        leavesColor = { 60, 180, 60, 255 };
        leavesDark = { 45, 150, 45, 255 };
        oakLeaves = { 50, 160, 50, 255 };
        oakLeavesDark = { 35, 130, 35, 255 };
    } else if (g_currentSeason == SEASON_AUTUMN) {
        // Autumn - varied fall colors based on tree position
        if (treeHash > 0.7f) {
            // Red/crimson tree
            leavesColor = { 180, 45, 30, 255 };
            leavesDark = { 140, 30, 20, 255 };
            oakLeaves = { 160, 40, 25, 255 };
            oakLeavesDark = { 120, 25, 15, 255 };
        } else if (treeHash > 0.4f) {
            // Orange tree
            leavesColor = { 210, 120, 40, 255 };
            leavesDark = { 180, 90, 30, 255 };
            oakLeaves = { 200, 110, 35, 255 };
            oakLeavesDark = { 170, 80, 25, 255 };
        } else {
            // Golden/yellow tree
            leavesColor = { 200, 170, 50, 255 };
            leavesDark = { 170, 140, 40, 255 };
            oakLeaves = { 190, 160, 45, 255 };
            oakLeavesDark = { 160, 130, 35, 255 };
        }
    } else {
        // Summer - deep vibrant green (default)
        leavesColor = { 34, 139, 34, 255 };
        leavesDark = { 20, 100, 20, 255 };
        oakLeaves = { 25, 100, 25, 255 };
        oakLeavesDark = { 15, 75, 15, 255 };
    }

    if (type == TREE_OAK) {
        // Oak tree - larger and darker
        Color oakTrunk = { 70, 45, 20, 255 };

        // Thicker trunk
        DrawModelCylinder(models, (Vector3){pos.x, pos.y, pos.z}, 0.5f, 0.6f, 3.5f, oakTrunk);

        // Larger, more layered canopy
        DrawModelSphere(models, (Vector3){pos.x, pos.y + 5.0f, pos.z}, 2.2f, oakLeaves);
        DrawModelSphere(models, (Vector3){pos.x - 1.0f, pos.y + 4.2f, pos.z + 0.8f}, 1.6f, oakLeavesDark);
        DrawModelSphere(models, (Vector3){pos.x + 1.0f, pos.y + 4.2f, pos.z - 0.8f}, 1.6f, oakLeavesDark);
        DrawModelSphere(models, (Vector3){pos.x + 0.5f, pos.y + 4.5f, pos.z + 1.0f}, 1.3f, oakLeaves);
        DrawModelSphere(models, (Vector3){pos.x - 0.5f, pos.y + 4.5f, pos.z - 1.0f}, 1.3f, oakLeaves);
        DrawModelSphere(models, (Vector3){pos.x, pos.y + 6.0f, pos.z}, 1.2f, oakLeaves);
    } else {
        // Normal tree
        Color trunkColor = { 101, 67, 33, 255 };

        // Trunk
        DrawModelCylinder(models, (Vector3){pos.x, pos.y, pos.z}, 0.3f, 0.4f, 2.5f, trunkColor);

        // Leaves (layered spheres)
        DrawModelSphere(models, (Vector3){pos.x, pos.y + 3.5f, pos.z}, 1.5f, leavesColor);
        DrawModelSphere(models, (Vector3){pos.x - 0.5f, pos.y + 3.0f, pos.z + 0.5f}, 1.0f, leavesDark);
        DrawModelSphere(models, (Vector3){pos.x + 0.5f, pos.y + 3.0f, pos.z - 0.5f}, 1.0f, leavesDark);
        DrawModelSphere(models, (Vector3){pos.x, pos.y + 4.2f, pos.z}, 0.8f, leavesColor);
    }

    (void)highlighted;  // Unused - wireframe removed
}

void DrawRock(const EntityModels* models, Vector3 pos, RockType type, bool highlighted) {
    // Colors vary by ore type
    Color baseColor, oreColor;
    if (type == ROCK_COPPER) {
        baseColor = (Color){ 100, 90, 80, 255 };     // Gray stone
        oreColor = (Color){ 180, 100, 50, 255 };     // Copper orange/brown
    } else {  // ROCK_TIN
        baseColor = (Color){ 90, 85, 80, 255 };      // Slightly different gray
        oreColor = (Color){ 150, 150, 140, 255 };    // Tin silver/gray
    }
    Color shadowColor = { (unsigned char)(baseColor.r - 30), (unsigned char)(baseColor.g - 30), (unsigned char)(baseColor.b - 30), 255 };

    // Main rock body (irregular boulder shape using multiple cubes)
    DrawModelCube(models, (Vector3){pos.x, pos.y + 0.5f, pos.z}, 1.2f, 0.9f, 1.0f, baseColor);
    DrawModelCube(models, (Vector3){pos.x + 0.3f, pos.y + 0.35f, pos.z - 0.2f}, 0.7f, 0.7f, 0.8f, shadowColor);
    DrawModelCube(models, (Vector3){pos.x - 0.25f, pos.y + 0.4f, pos.z + 0.3f}, 0.6f, 0.6f, 0.7f, baseColor);

    // Top boulder
    DrawModelSphere(models, (Vector3){pos.x, pos.y + 1.0f, pos.z}, 0.5f, shadowColor);
    DrawModelCube(models, (Vector3){pos.x + 0.2f, pos.y + 0.9f, pos.z - 0.1f}, 0.4f, 0.3f, 0.4f, baseColor);

    // Ore veins (visible ore spots on the rock)
    DrawModelSphere(models, (Vector3){pos.x + 0.4f, pos.y + 0.6f, pos.z + 0.35f}, 0.18f, oreColor);
    DrawModelSphere(models, (Vector3){pos.x - 0.3f, pos.y + 0.5f, pos.z - 0.3f}, 0.15f, oreColor);
    DrawModelSphere(models, (Vector3){pos.x + 0.1f, pos.y + 0.9f, pos.z + 0.2f}, 0.12f, oreColor);
    DrawModelCube(models, (Vector3){pos.x - 0.35f, pos.y + 0.7f, pos.z + 0.1f}, 0.15f, 0.2f, 0.1f, oreColor);

    (void)highlighted;  // Unused - no wireframe highlight for rocks
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

void DrawLamp(const EntityModels* models, Vector3 pos, bool lit) {
    // Metal post colors
    Color metalDark = { 50, 50, 55, 255 };
    Color metalMid = { 70, 70, 75, 255 };

    // Glass color depends on whether lamp is lit
    Color glassColor = lit ? (Color){ 255, 220, 150, 220 } : (Color){ 150, 150, 150, 180 };
    Color flameColor = { 255, 200, 100, 255 };  // Warm flame glow

    // Post base (wider at bottom)
    DrawModelCube(models, (Vector3){pos.x, pos.y + 0.1f, pos.z}, 0.25f, 0.2f, 0.25f, metalDark);

    // Main post
    DrawModelCylinder(models, (Vector3){pos.x, pos.y + 0.2f, pos.z}, 0.06f, 0.06f, 2.0f, metalMid);

    // Lamp housing frame (top)
    float lampY = pos.y + 2.2f;
    DrawModelCube(models, (Vector3){pos.x, lampY + 0.25f, pos.z}, 0.35f, 0.08f, 0.35f, metalDark);  // Top cap
    DrawModelCube(models, (Vector3){pos.x, lampY - 0.05f, pos.z}, 0.30f, 0.06f, 0.30f, metalDark);  // Bottom rim

    // Glass housing (4 panels)
    float glassY = lampY + 0.1f;
    DrawModelCube(models, (Vector3){pos.x + 0.13f, glassY, pos.z}, 0.02f, 0.25f, 0.24f, glassColor);
    DrawModelCube(models, (Vector3){pos.x - 0.13f, glassY, pos.z}, 0.02f, 0.25f, 0.24f, glassColor);
    DrawModelCube(models, (Vector3){pos.x, glassY, pos.z + 0.13f}, 0.24f, 0.25f, 0.02f, glassColor);
    DrawModelCube(models, (Vector3){pos.x, glassY, pos.z - 0.13f}, 0.24f, 0.25f, 0.02f, glassColor);

    // Flame inside (only when lit)
    if (lit) {
        DrawModelSphere(models, (Vector3){pos.x, glassY, pos.z}, 0.08f, flameColor);
    }
}

void DrawCampfire(const EntityModels* models, Vector3 pos) {
    // Stone ring colors
    Color stoneColor = { 100, 90, 80, 255 };
    Color stoneDark = { 70, 65, 60, 255 };

    // Wood colors
    Color woodColor = { 101, 67, 33, 255 };
    Color woodDark = { 60, 40, 25, 255 };
    Color charColor = { 30, 25, 20, 255 };

    // Stone ring (8 stones in a circle)
    float ringRadius = 0.5f;
    for (int i = 0; i < 8; i++) {
        float angle = (float)i * (PI / 4.0f);
        float sx = pos.x + cosf(angle) * ringRadius;
        float sz = pos.z + sinf(angle) * ringRadius;
        Color stoneC = (i % 2 == 0) ? stoneColor : stoneDark;
        DrawModelCube(models, (Vector3){sx, pos.y + 0.08f, sz}, 0.18f, 0.16f, 0.18f, stoneC);
    }

    // Charred ground
    DrawModelCylinder(models, (Vector3){pos.x, pos.y, pos.z}, 0.35f, 0.35f, 0.02f, charColor);

    // Logs arranged in a teepee/cross pattern
    // Log 1 - angled
    rlPushMatrix();
    rlTranslatef(pos.x + 0.1f, pos.y + 0.15f, pos.z);
    rlRotatef(25.0f, 0, 0, 1);
    rlRotatef(15.0f, 0, 1, 0);
    DrawModelCylinder(models, (Vector3){0, 0, 0}, 0.06f, 0.04f, 0.45f, woodColor);
    rlPopMatrix();

    // Log 2 - opposite angle
    rlPushMatrix();
    rlTranslatef(pos.x - 0.1f, pos.y + 0.15f, pos.z);
    rlRotatef(-25.0f, 0, 0, 1);
    rlRotatef(-20.0f, 0, 1, 0);
    DrawModelCylinder(models, (Vector3){0, 0, 0}, 0.06f, 0.04f, 0.45f, woodDark);
    rlPopMatrix();

    // Log 3 - third angle
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y + 0.15f, pos.z + 0.1f);
    rlRotatef(20.0f, 1, 0, 0);
    rlRotatef(30.0f, 0, 1, 0);
    DrawModelCylinder(models, (Vector3){0, 0, 0}, 0.05f, 0.03f, 0.4f, woodColor);
    rlPopMatrix();

    // Draw fire using shader - two crossed planes for visibility from all angles
    float gameTime = (float)GetTime();
    SetShaderValue(models->fireShader, models->fireTimeLoc, &gameTime, SHADER_UNIFORM_FLOAT);

    float fireHeight = 1.0f;
    float fireWidth = 0.7f;

    // Disable backface culling so fire is visible from both sides
    rlDisableBackfaceCulling();

    // Fire plane 1 - facing Z axis (rotated to be vertical)
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y + fireHeight * 0.5f + 0.1f, pos.z);
    rlRotatef(90.0f, 1, 0, 0);  // Rotate to be vertical
    rlScalef(fireWidth, 1.0f, fireHeight);
    DrawModel(models->firePlane, (Vector3){0, 0, 0}, 1.0f, WHITE);
    rlPopMatrix();

    // Fire plane 2 - rotated 90 degrees (cross pattern)
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y + fireHeight * 0.5f + 0.1f, pos.z);
    rlRotatef(90.0f, 1, 0, 0);  // Rotate to be vertical
    rlRotatef(90.0f, 0, 0, 1);  // Rotate around up axis
    rlScalef(fireWidth, 1.0f, fireHeight);
    DrawModel(models->firePlane, (Vector3){0, 0, 0}, 1.0f, WHITE);
    rlPopMatrix();

    // Restore backface culling
    rlEnableBackfaceCulling();
}

void DrawLightSource(const EntityModels* models, const LightSource& light, bool lampsOn) {
    switch (light.type) {
        case LIGHT_LAMP:
            DrawLamp(models, light.position, lampsOn);
            break;
        case LIGHT_CAMPFIRE:
            DrawCampfire(models, light.position);
            break;
        default:
            break;
    }
}

void DrawLightSources(const EntityModels* models, const LightSource* lights, int lightCount, bool lampsOn) {
    for (int i = 0; i < lightCount; i++) {
        DrawLightSource(models, lights[i], lampsOn);
    }
}

void DrawLadder(const EntityModels* models, const Ladder& ladder, bool highlighted) {
    Color woodColor = highlighted ? (Color){180, 140, 90, 255} : (Color){139, 90, 43, 255};
    Color woodDark = highlighted ? (Color){140, 100, 60, 255} : (Color){100, 65, 30, 255};

    float groundY = GetTerrainHeight(ladder.position.x, ladder.position.z);
    Vector3 basePos = { ladder.position.x, groundY, ladder.position.z };

    float angle = ladder.facingAngle * DEG2RAD;
    float cosA = cosf(angle);
    float sinA = sinf(angle);

    // Ladder dimensions
    float ladderWidth = 0.6f;   // Width between rails
    float railThickness = 0.08f;
    float rungThickness = 0.06f;
    float rungSpacing = 0.5f;   // Vertical distance between rungs

    // Side rails (two vertical poles)
    float halfWidth = ladderWidth / 2.0f;
    float offsetX1 = -halfWidth * cosA;
    float offsetZ1 = -halfWidth * sinA;
    float offsetX2 = halfWidth * cosA;
    float offsetZ2 = halfWidth * sinA;

    // Left rail
    Vector3 rail1Pos = { basePos.x + offsetX1, basePos.y + ladder.height / 2.0f, basePos.z + offsetZ1 };
    DrawModelCube(models, rail1Pos, railThickness, ladder.height, railThickness, woodColor);

    // Right rail
    Vector3 rail2Pos = { basePos.x + offsetX2, basePos.y + ladder.height / 2.0f, basePos.z + offsetZ2 };
    DrawModelCube(models, rail2Pos, railThickness, ladder.height, railThickness, woodColor);

    // Rungs (horizontal bars)
    int numRungs = (int)(ladder.height / rungSpacing);
    for (int i = 1; i <= numRungs; i++) {
        float rungY = basePos.y + i * rungSpacing - rungSpacing / 2.0f;
        Vector3 rungPos = { basePos.x, rungY, basePos.z };

        // Rung rotated to connect the rails
        float rungWidth = ladderWidth;
        float rungDepth = rungThickness;

        // Draw rung as rotated cube
        rlPushMatrix();
        rlTranslatef(rungPos.x, rungPos.y, rungPos.z);
        rlRotatef(ladder.facingAngle, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0}, rungWidth, rungThickness, rungDepth, woodDark);
        rlPopMatrix();
    }
}

void DrawLadders(const EntityModels* models, const Ladder* ladders, int ladderCount, const Ladder* highlightedLadder) {
    for (int i = 0; i < ladderCount; i++) {
        bool highlighted = (highlightedLadder == &ladders[i]);
        DrawLadder(models, ladders[i], highlighted);
    }
}
