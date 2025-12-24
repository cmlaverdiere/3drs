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

    (void)highlighted;  // Wireframe outline removed

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

    (void)highlighted;  // Wireframe outline removed

    rlPopMatrix();
}

void DrawScorpion(Vector3 pos, float facingAngle, bool highlighted) {
    Color bodyColor = { 139, 90, 43, 255 };       // Dark brown
    Color bodyDark = { 101, 67, 33, 255 };        // Darker brown
    Color clawColor = { 160, 100, 50, 255 };      // Lighter brown for claws
    Color stingerColor = { 80, 50, 30, 255 };     // Very dark for stinger tip

    // Apply rotation around Y axis at position
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(facingAngle * RAD2DEG, 0, 1, 0);

    // Draw scorpion at origin (facing +Z is forward)
    // Body segments (raised slightly for visibility)
    float baseY = 0.25f;  // Base height offset
    DrawCube((Vector3){0, baseY, 0}, 0.4f, 0.15f, 0.5f, bodyColor);  // Main body
    DrawCube((Vector3){0, baseY - 0.03f, 0.3f}, 0.3f, 0.12f, 0.2f, bodyDark); // Head

    // Legs (4 pairs)
    float legY = baseY - 0.07f;
    // Front legs
    DrawCube((Vector3){-0.25f, legY, 0.15f}, 0.15f, 0.05f, 0.06f, bodyDark);
    DrawCube((Vector3){0.25f, legY, 0.15f}, 0.15f, 0.05f, 0.06f, bodyDark);
    // Mid-front legs
    DrawCube((Vector3){-0.28f, legY, 0.0f}, 0.18f, 0.05f, 0.06f, bodyDark);
    DrawCube((Vector3){0.28f, legY, 0.0f}, 0.18f, 0.05f, 0.06f, bodyDark);
    // Mid-back legs
    DrawCube((Vector3){-0.28f, legY, -0.15f}, 0.18f, 0.05f, 0.06f, bodyDark);
    DrawCube((Vector3){0.28f, legY, -0.15f}, 0.18f, 0.05f, 0.06f, bodyDark);
    // Back legs
    DrawCube((Vector3){-0.22f, legY, -0.25f}, 0.12f, 0.05f, 0.06f, bodyDark);
    DrawCube((Vector3){0.22f, legY, -0.25f}, 0.12f, 0.05f, 0.06f, bodyDark);

    // Pincers/claws (front)
    float clawY = baseY - 0.03f;
    // Left claw arm
    DrawCube((Vector3){-0.2f, clawY, 0.45f}, 0.08f, 0.08f, 0.2f, clawColor);
    DrawCube((Vector3){-0.25f, clawY, 0.58f}, 0.12f, 0.06f, 0.08f, clawColor);  // Pincer
    DrawCube((Vector3){-0.18f, clawY, 0.58f}, 0.06f, 0.06f, 0.1f, clawColor);   // Pincer jaw
    // Right claw arm
    DrawCube((Vector3){0.2f, clawY, 0.45f}, 0.08f, 0.08f, 0.2f, clawColor);
    DrawCube((Vector3){0.25f, clawY, 0.58f}, 0.12f, 0.06f, 0.08f, clawColor);   // Pincer
    DrawCube((Vector3){0.18f, clawY, 0.58f}, 0.06f, 0.06f, 0.1f, clawColor);    // Pincer jaw

    // Tail (segmented, curves up and over)
    DrawCube((Vector3){0, baseY + 0.03f, -0.35f}, 0.15f, 0.12f, 0.15f, bodyColor);  // Segment 1
    DrawCube((Vector3){0, baseY + 0.13f, -0.48f}, 0.12f, 0.1f, 0.12f, bodyColor);   // Segment 2
    DrawCube((Vector3){0, baseY + 0.27f, -0.55f}, 0.1f, 0.1f, 0.1f, bodyColor);     // Segment 3
    DrawCube((Vector3){0, baseY + 0.40f, -0.55f}, 0.08f, 0.12f, 0.08f, bodyColor);  // Segment 4
    DrawCube((Vector3){0, baseY + 0.50f, -0.50f}, 0.06f, 0.1f, 0.08f, bodyColor);   // Segment 5

    // Stinger (pointing forward)
    DrawCube((Vector3){0, baseY + 0.55f, -0.42f}, 0.05f, 0.08f, 0.1f, stingerColor);
    DrawSphere((Vector3){0, baseY + 0.57f, -0.36f}, 0.04f, stingerColor);  // Stinger tip

    (void)highlighted;  // Wireframe outline removed

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
        case ENEMY_SCORPION:
            DrawScorpion(enemy.position, enemy.facingAngle, highlighted);
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
        case ITEM_BRONZE_AXE: {
            // Axe on ground
            Color bronzeHead = { 205, 127, 50, 255 };
            Color woodHandle = { 101, 67, 33, 255 };
            // Handle (lying flat)
            DrawCube((Vector3){pos.x, pos.y + 0.04f, pos.z}, 0.5f, 0.06f, 0.06f, woodHandle);
            // Axe head
            DrawCube((Vector3){pos.x + 0.2f, pos.y + 0.08f, pos.z}, 0.15f, 0.12f, 0.25f, bronzeHead);
            break;
        }
        case ITEM_LOGS: {
            // Wooden log on ground
            Color barkColor = { 101, 67, 33, 255 };
            Color woodColor = { 210, 180, 140, 255 };
            DrawCylinder((Vector3){pos.x, pos.y, pos.z}, 0.12f, 0.12f, 0.5f, 8, barkColor);
            // End caps
            DrawCylinderWires((Vector3){pos.x, pos.y, pos.z}, 0.12f, 0.12f, 0.5f, 8, woodColor);
            break;
        }
        case ITEM_CHITIN: {
            // Dark brown chitin shell piece
            Color chitinColor = { 101, 67, 33, 255 };
            Color chitinDark = { 70, 45, 20, 255 };
            // Main shell piece (curved appearance via overlapping shapes)
            DrawCube((Vector3){pos.x, pos.y + 0.04f, pos.z}, 0.25f, 0.06f, 0.35f, chitinColor);
            DrawCube((Vector3){pos.x, pos.y + 0.07f, pos.z}, 0.2f, 0.04f, 0.3f, chitinDark);
            // Ridges on shell
            DrawCube((Vector3){pos.x, pos.y + 0.08f, pos.z - 0.08f}, 0.18f, 0.02f, 0.05f, chitinDark);
            DrawCube((Vector3){pos.x, pos.y + 0.08f, pos.z + 0.08f}, 0.18f, 0.02f, 0.05f, chitinDark);
            break;
        }
        default:
            // Unknown item - draw small red cube
            DrawCube(pos, 0.2f, 0.2f, 0.2f, RED);
            break;
    }
}

void DrawTree(Vector3 pos, bool highlighted) {
    Color trunkColor = { 101, 67, 33, 255 };      // Brown trunk
    Color leavesColor = { 34, 139, 34, 255 };     // Forest green
    Color leavesDark = { 20, 100, 20, 255 };      // Darker green for variety

    // Trunk
    DrawCylinder((Vector3){pos.x, pos.y, pos.z}, 0.3f, 0.4f, 2.5f, 8, trunkColor);

    // Leaves (layered spheres for a round tree appearance)
    DrawSphere((Vector3){pos.x, pos.y + 3.5f, pos.z}, 1.5f, leavesColor);
    DrawSphere((Vector3){pos.x - 0.5f, pos.y + 3.0f, pos.z + 0.5f}, 1.0f, leavesDark);
    DrawSphere((Vector3){pos.x + 0.5f, pos.y + 3.0f, pos.z - 0.5f}, 1.0f, leavesDark);
    DrawSphere((Vector3){pos.x, pos.y + 4.2f, pos.z}, 0.8f, leavesColor);

    if (highlighted) {
        Color outlineColor = { 255, 255, 0, 255 };
        DrawCylinderWires((Vector3){pos.x, pos.y, pos.z}, 0.35f, 0.45f, 2.5f, 8, outlineColor);
        DrawSphereWires((Vector3){pos.x, pos.y + 3.5f, pos.z}, 1.55f, 8, 8, outlineColor);
    }
}
