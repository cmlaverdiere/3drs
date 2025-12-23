#include "rendering.h"

void DrawSword(Vector3 pos, Color bladeColor, Color handleColor) {
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z}, 0.08f, 0.05f, 0.6f, bladeColor);
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z - 0.35f}, 0.06f, 0.08f, 0.15f, handleColor);
    DrawCube((Vector3){pos.x, pos.y + 0.05f, pos.z - 0.25f}, 0.2f, 0.04f, 0.04f, handleColor);
}

void DrawTroll(Vector3 pos, bool highlighted) {
    Color trollSkin = { 100, 140, 100, 255 };
    Color trollDark = { 70, 100, 70, 255 };
    Color eyeColor = { 200, 50, 50, 255 };
    Color eyeWhite = { 220, 220, 180, 255 };
    Color browColor = { 50, 70, 50, 255 };
    Color mouthColor = { 40, 30, 30, 255 };

    // Body
    DrawCube((Vector3){pos.x, pos.y + 0.8f, pos.z}, 0.6f, 0.8f, 0.4f, trollSkin);
    // Head
    DrawSphere((Vector3){pos.x, pos.y + 1.5f, pos.z}, 0.35f, trollSkin);

    // Face - angry expression
    float headY = pos.y + 1.5f;
    float faceZ = pos.z + 0.30f;

    // Eye whites
    DrawSphere((Vector3){pos.x - 0.10f, headY + 0.05f, faceZ}, 0.07f, eyeWhite);
    DrawSphere((Vector3){pos.x + 0.10f, headY + 0.05f, faceZ}, 0.07f, eyeWhite);

    // Pupils
    DrawSphere((Vector3){pos.x - 0.10f, headY + 0.05f, faceZ + 0.04f}, 0.04f, eyeColor);
    DrawSphere((Vector3){pos.x + 0.10f, headY + 0.05f, faceZ + 0.04f}, 0.04f, eyeColor);

    // Angry eyebrows
    DrawCube((Vector3){pos.x - 0.12f, headY + 0.15f, faceZ}, 0.10f, 0.03f, 0.02f, browColor);
    DrawCube((Vector3){pos.x - 0.06f, headY + 0.12f, faceZ}, 0.06f, 0.03f, 0.02f, browColor);
    DrawCube((Vector3){pos.x + 0.12f, headY + 0.15f, faceZ}, 0.10f, 0.03f, 0.02f, browColor);
    DrawCube((Vector3){pos.x + 0.06f, headY + 0.12f, faceZ}, 0.06f, 0.03f, 0.02f, browColor);

    // Scowling mouth
    DrawCube((Vector3){pos.x, headY - 0.12f, faceZ}, 0.14f, 0.03f, 0.02f, mouthColor);
    DrawCube((Vector3){pos.x - 0.08f, headY - 0.10f, faceZ}, 0.03f, 0.03f, 0.02f, mouthColor);
    DrawCube((Vector3){pos.x + 0.08f, headY - 0.10f, faceZ}, 0.03f, 0.03f, 0.02f, mouthColor);

    // Arms
    DrawCube((Vector3){pos.x - 0.45f, pos.y + 0.8f, pos.z}, 0.2f, 0.6f, 0.2f, trollDark);
    DrawCube((Vector3){pos.x + 0.45f, pos.y + 0.8f, pos.z}, 0.2f, 0.6f, 0.2f, trollDark);
    // Legs
    DrawCube((Vector3){pos.x - 0.15f, pos.y + 0.2f, pos.z}, 0.2f, 0.4f, 0.2f, trollDark);
    DrawCube((Vector3){pos.x + 0.15f, pos.y + 0.2f, pos.z}, 0.2f, 0.4f, 0.2f, trollDark);

    if (highlighted) {
        Color outlineColor = { 255, 255, 0, 255 };
        DrawCubeWires((Vector3){pos.x, pos.y + 0.8f, pos.z}, 0.65f, 0.85f, 0.45f, outlineColor);
        DrawSphereWires((Vector3){pos.x, pos.y + 1.5f, pos.z}, 0.38f, 8, 8, outlineColor);
        DrawCubeWires((Vector3){pos.x - 0.45f, pos.y + 0.8f, pos.z}, 0.25f, 0.65f, 0.25f, outlineColor);
        DrawCubeWires((Vector3){pos.x + 0.45f, pos.y + 0.8f, pos.z}, 0.25f, 0.65f, 0.25f, outlineColor);
        DrawCubeWires((Vector3){pos.x - 0.15f, pos.y + 0.2f, pos.z}, 0.25f, 0.45f, 0.25f, outlineColor);
        DrawCubeWires((Vector3){pos.x + 0.15f, pos.y + 0.2f, pos.z}, 0.25f, 0.45f, 0.25f, outlineColor);
    }
}
