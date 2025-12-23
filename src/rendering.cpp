#include "rendering.h"
#include "rlgl.h"
#include <cmath>

void DrawSword(Vector3 pos, Color bladeColor, Color handleColor) {
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.08f, 0.05f, 0.6f, bladeColor);
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z - 0.35f}, 0.06f, 0.08f, 0.15f, handleColor);
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z - 0.25f}, 0.2f, 0.04f, 0.04f, handleColor);
}

void DrawTroll(Vector3 pos, float facingAngle, bool highlighted) {
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
    // Body
    DrawCube((Vector3){0, 0.8f, 0}, 0.6f, 0.8f, 0.4f, trollSkin);
    // Head
    DrawSphere((Vector3){0, 1.5f, 0}, 0.35f, trollSkin);

    // Face - angry expression
    float headY = 1.5f;
    float faceZ = 0.30f;

    // Eye whites
    DrawSphere((Vector3){-0.10f, headY + 0.05f, faceZ}, 0.07f, eyeWhite);
    DrawSphere((Vector3){0.10f, headY + 0.05f, faceZ}, 0.07f, eyeWhite);

    // Pupils
    DrawSphere((Vector3){-0.10f, headY + 0.05f, faceZ + 0.04f}, 0.04f, eyeColor);
    DrawSphere((Vector3){0.10f, headY + 0.05f, faceZ + 0.04f}, 0.04f, eyeColor);

    // Angry eyebrows
    DrawCube((Vector3){-0.12f, headY + 0.15f, faceZ}, 0.10f, 0.03f, 0.02f, browColor);
    DrawCube((Vector3){-0.06f, headY + 0.12f, faceZ}, 0.06f, 0.03f, 0.02f, browColor);
    DrawCube((Vector3){0.12f, headY + 0.15f, faceZ}, 0.10f, 0.03f, 0.02f, browColor);
    DrawCube((Vector3){0.06f, headY + 0.12f, faceZ}, 0.06f, 0.03f, 0.02f, browColor);

    // Scowling mouth
    DrawCube((Vector3){0, headY - 0.12f, faceZ}, 0.14f, 0.03f, 0.02f, mouthColor);
    DrawCube((Vector3){-0.08f, headY - 0.10f, faceZ}, 0.03f, 0.03f, 0.02f, mouthColor);
    DrawCube((Vector3){0.08f, headY - 0.10f, faceZ}, 0.03f, 0.03f, 0.02f, mouthColor);

    // Arms
    DrawCube((Vector3){-0.45f, 0.8f, 0}, 0.2f, 0.6f, 0.2f, trollDark);
    DrawCube((Vector3){0.45f, 0.8f, 0}, 0.2f, 0.6f, 0.2f, trollDark);
    // Legs
    DrawCube((Vector3){-0.15f, 0.2f, 0}, 0.2f, 0.4f, 0.2f, trollDark);
    DrawCube((Vector3){0.15f, 0.2f, 0}, 0.2f, 0.4f, 0.2f, trollDark);

    if (highlighted) {
        Color outlineColor = { 255, 255, 0, 255 };
        DrawCubeWires((Vector3){0, 0.8f, 0}, 0.65f, 0.85f, 0.45f, outlineColor);
        DrawSphereWires((Vector3){0, 1.5f, 0}, 0.38f, 8, 8, outlineColor);
        DrawCubeWires((Vector3){-0.45f, 0.8f, 0}, 0.25f, 0.65f, 0.25f, outlineColor);
        DrawCubeWires((Vector3){0.45f, 0.8f, 0}, 0.25f, 0.65f, 0.25f, outlineColor);
        DrawCubeWires((Vector3){-0.15f, 0.2f, 0}, 0.25f, 0.45f, 0.25f, outlineColor);
        DrawCubeWires((Vector3){0.15f, 0.2f, 0}, 0.25f, 0.45f, 0.25f, outlineColor);
    }

    rlPopMatrix();
}

void DrawCow(Vector3 pos, float facingAngle, bool highlighted) {
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

    // Draw cow at origin (will be transformed by matrix)
    // Body (horizontal, cow-like)
    DrawCube((Vector3){0, 0.6f, 0}, 0.6f, 0.5f, 1.0f, cowBody);

    // Spots on body
    DrawCube((Vector3){0.2f, 0.7f, 0.1f}, 0.2f, 0.2f, 0.3f, cowSpots);
    DrawCube((Vector3){-0.15f, 0.65f, -0.2f}, 0.15f, 0.15f, 0.25f, cowSpots);

    // Head
    DrawCube((Vector3){0, 0.7f, 0.65f}, 0.35f, 0.35f, 0.3f, cowBody);

    // Snout/nose
    DrawCube((Vector3){0, 0.6f, 0.85f}, 0.2f, 0.15f, 0.1f, cowPink);

    // Eyes
    DrawSphere((Vector3){-0.12f, 0.8f, 0.75f}, 0.05f, eyeWhite);
    DrawSphere((Vector3){0.12f, 0.8f, 0.75f}, 0.05f, eyeWhite);
    DrawSphere((Vector3){-0.12f, 0.8f, 0.78f}, 0.025f, eyeBlack);
    DrawSphere((Vector3){0.12f, 0.8f, 0.78f}, 0.025f, eyeBlack);

    // Ears
    DrawCube((Vector3){-0.2f, 0.85f, 0.55f}, 0.1f, 0.05f, 0.1f, cowBody);
    DrawCube((Vector3){0.2f, 0.85f, 0.55f}, 0.1f, 0.05f, 0.1f, cowBody);

    // Legs (4 legs)
    float legHeight = 0.35f;
    DrawCube((Vector3){-0.2f, legHeight/2, 0.35f}, 0.1f, legHeight, 0.1f, cowBody);
    DrawCube((Vector3){0.2f, legHeight/2, 0.35f}, 0.1f, legHeight, 0.1f, cowBody);
    DrawCube((Vector3){-0.2f, legHeight/2, -0.35f}, 0.1f, legHeight, 0.1f, cowBody);
    DrawCube((Vector3){0.2f, legHeight/2, -0.35f}, 0.1f, legHeight, 0.1f, cowBody);

    // Hooves
    DrawCube((Vector3){-0.2f, 0.03f, 0.35f}, 0.1f, 0.06f, 0.1f, cowHooves);
    DrawCube((Vector3){0.2f, 0.03f, 0.35f}, 0.1f, 0.06f, 0.1f, cowHooves);
    DrawCube((Vector3){-0.2f, 0.03f, -0.35f}, 0.1f, 0.06f, 0.1f, cowHooves);
    DrawCube((Vector3){0.2f, 0.03f, -0.35f}, 0.1f, 0.06f, 0.1f, cowHooves);

    // Udder
    DrawSphere((Vector3){0, 0.35f, -0.1f}, 0.12f, cowPink);

    // Tail
    DrawCube((Vector3){0, 0.7f, -0.6f}, 0.04f, 0.04f, 0.2f, cowBody);
    DrawCube((Vector3){0, 0.5f, -0.7f}, 0.06f, 0.15f, 0.04f, cowSpots);

    if (highlighted) {
        Color outlineColor = { 255, 255, 0, 255 };
        DrawCubeWires((Vector3){0, 0.6f, 0}, 0.65f, 0.55f, 1.05f, outlineColor);
        DrawCubeWires((Vector3){0, 0.7f, 0.65f}, 0.4f, 0.4f, 0.35f, outlineColor);
    }

    rlPopMatrix();
}

void DrawEnemy(const Enemy& enemy, bool highlighted) {
    switch (enemy.type) {
        case ENEMY_TROLL:
            DrawTroll(enemy.position, enemy.facingAngle, highlighted);
            break;
        case ENEMY_COW:
            DrawCow(enemy.position, enemy.facingAngle, highlighted);
            break;
        default:
            // Unknown enemy type - draw a red cube
            DrawCube(enemy.position, 0.5f, 1.0f, 0.5f, RED);
            break;
    }
}

void DrawWorldItem(ItemType type, Vector3 pos) {
    switch (type) {
        case ITEM_BRONZE_SHORTSWORD: {
            Color bronzeBlade = { 205, 127, 50, 255 };
            Color bronzeHandle = { 139, 90, 43, 255 };
            DrawSword(pos, bronzeBlade, bronzeHandle);
            break;
        }
        case ITEM_COW_HIDE: {
            // Flat brown hide on ground
            Color hideColor = { 139, 90, 43, 255 };
            DrawCube((Vector3){pos.x, pos.y + 0.02f, pos.z}, 0.4f, 0.04f, 0.5f, hideColor);
            // Spots
            Color spotColor = { 80, 50, 30, 255 };
            DrawCube((Vector3){pos.x + 0.1f, pos.y + 0.03f, pos.z + 0.1f}, 0.1f, 0.02f, 0.1f, spotColor);
            DrawCube((Vector3){pos.x - 0.1f, pos.y + 0.03f, pos.z - 0.1f}, 0.08f, 0.02f, 0.12f, spotColor);
            break;
        }
        case ITEM_BONES: {
            // White bone on ground
            Color boneColor = { 230, 220, 200, 255 };
            DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.08f, 0.08f, 0.4f, boneColor);
            // Bone ends
            DrawSphere((Vector3){pos.x, pos.y + 0.05f, pos.z + 0.2f}, 0.06f, boneColor);
            DrawSphere((Vector3){pos.x, pos.y + 0.05f, pos.z - 0.2f}, 0.06f, boneColor);
            break;
        }
        case ITEM_GIL: {
            // Gold coin pile
            Color goldColor = { 255, 215, 0, 255 };
            Color goldDark = { 200, 160, 0, 255 };
            DrawCylinder((Vector3){pos.x, pos.y, pos.z}, 0.15f, 0.15f, 0.05f, 8, goldColor);
            DrawCylinder((Vector3){pos.x + 0.05f, pos.y + 0.03f, pos.z + 0.05f}, 0.1f, 0.1f, 0.04f, 8, goldDark);
            DrawCylinder((Vector3){pos.x - 0.03f, pos.y + 0.05f, pos.z - 0.03f}, 0.08f, 0.08f, 0.03f, 8, goldColor);
            break;
        }
        default:
            // Unknown item - draw small red cube
            DrawCube(pos, 0.2f, 0.2f, 0.2f, RED);
            break;
    }
}
