#include "raylib.h"

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

    DisableCursor();
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        BeginDrawing();
            ClearBackground(SKYBLUE);

            BeginMode3D(camera);
                DrawModel(groundModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                DrawGrid(100, 1.0f);
            EndMode3D();

            DrawText("WASD to move, Mouse to look", 10, 10, 20, WHITE);
            DrawFPS(screenWidth - 100, 10);
        EndDrawing();
    }

    UnloadModel(groundModel);
    UnloadShader(grassShader);
    CloseWindow();
    return 0;
}
