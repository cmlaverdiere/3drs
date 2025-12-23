#include "raylib.h"
#include <cstdio>
#include <ctime>
#include <sys/stat.h>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "3D RuneScape-style Game");

    Camera3D camera = {};
    camera.position = (Vector3){ 0.0f, 1.8f, 0.0f };
    camera.target = (Vector3){ 0.0f, 1.8f, 1.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Shader grassShader = LoadShader("shaders/grass.vs", "shaders/grass.fs");

    Mesh groundMesh = GenMeshPlane(100.0f, 100.0f, 10, 10);
    Model groundModel = LoadModelFromMesh(groundMesh);
    groundModel.materials[0].shader = grassShader;

    mkdir("screenshots", 0755);

    float screenshotMsgTimer = 0.0f;
    char screenshotMsg[128] = "";

    DisableCursor();
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        if (IsKeyPressed(KEY_P)) {
            time_t now = time(nullptr);
            char filename[64];
            strftime(filename, sizeof(filename), "screenshots/%Y%m%d_%H%M%S.png", localtime(&now));
            Image screenshot = LoadImageFromScreen();
            ExportImage(screenshot, filename);
            UnloadImage(screenshot);
            snprintf(screenshotMsg, sizeof(screenshotMsg), "Saved: %s", filename);
            screenshotMsgTimer = 2.0f;
        }

        if (screenshotMsgTimer > 0.0f) screenshotMsgTimer -= dt;

        BeginDrawing();
            ClearBackground(SKYBLUE);

            BeginMode3D(camera);
                DrawModel(groundModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                DrawGrid(100, 1.0f);
            EndMode3D();

            DrawText("WASD to move, Mouse to look", 10, 10, 20, WHITE);
            DrawFPS(screenWidth - 100, 10);

            if (screenshotMsgTimer > 0.0f) {
                DrawText(screenshotMsg, 10, screenHeight - 30, 20, YELLOW);
            }
        EndDrawing();
    }

    UnloadModel(groundModel);
    UnloadShader(grassShader);
    CloseWindow();
    return 0;
}
