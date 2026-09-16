// Standalone: c++ -std=c++17 -Isrc -Ibuild/_deps/raylib-src/src tests/lighting_math_test.cpp -o /tmp/lighting_math_test && /tmp/lighting_math_test
#include "lighting_math.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

static void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

// Raylib 5.0 names its homogeneous Vector4 transform QuaternionTransform.
static Vector4 Transform(Vector4 point, Matrix matrix) {
    return QuaternionTransform(point, matrix);
}

static Vector2 ShadowPixel(Vector3 point, const ShadowMatrices& matrices) {
    Vector4 clip = Transform({point.x, point.y, point.z, 1.0f}, matrices.viewProjection);
    return {(clip.x / clip.w * 0.5f + 0.5f) * SHADOW_MAP_RESOLUTION,
            (clip.y / clip.w * 0.5f + 0.5f) * SHADOW_MAP_RESOLUTION};
}

static void RequireSamePixel(Vector2 first, Vector2 second, const char* message) {
    Require(fabsf(first.x - second.x) < 0.01f && fabsf(first.y - second.y) < 0.01f, message);
}

int main() {
    const Vector3 direction = Vector3Normalize({0.4f, -0.8f, 0.3f});
    const Vector3 right = Vector3Normalize(Vector3CrossProduct(direction, {0, 1, 0}));
    const Vector3 up = Vector3CrossProduct(right, direction);
    const Vector3 worldPoint = {17.0f, 3.0f, -28.0f};
    const float texel = SHADOW_ORTHO_SIZE / SHADOW_MAP_RESOLUTION;
    const auto base = CalculateShadowMatrices({0, 0, 0}, direction);
    const Vector2 basePixel = ShadowPixel(worldPoint, base);

    for (float offset : {-0.24f, 0.24f}) {
        Vector3 center = Vector3Add(Vector3Scale(right, offset * texel), Vector3Scale(up, offset * texel));
        RequireSamePixel(basePixel, ShadowPixel(worldPoint, CalculateShadowMatrices(center, direction)),
                         "subtexel camera motion must preserve fixed-point shadow XY");
    }
    for (Vector3 axis : {right, up}) {
        Vector2 moved = ShadowPixel(worldPoint, CalculateShadowMatrices(Vector3Scale(axis, 0.75f * texel), direction));
        float dx = moved.x - basePixel.x, dy = moved.y - basePixel.y;
        Require(fabsf(dx - roundf(dx)) < 0.01f && fabsf(dy - roundf(dy)) < 0.01f,
                "crossing snap boundary must move projection by integer texels");
        Require(fabsf(fabsf(dx) + fabsf(dy) - 1.0f) < 0.01f,
                "one crossed snap boundary must move projection by exactly one texel");
    }
    RequireSamePixel(basePixel, ShadowPixel(worldPoint, CalculateShadowMatrices(Vector3Scale(direction, 35.0f), direction)),
                     "camera translation along light direction must preserve shadow XY");

    for (Vector3 vertical : {Vector3{0, -1, 0}, Vector3{0, 1, 0}, Vector3{0.000001f, -1, 0.000001f}}) {
        auto matrices = CalculateShadowMatrices({25, 2, -15}, vertical);
        for (Matrix matrix : {matrices.view, matrices.projection, matrices.viewProjection}) {
            auto values = MatrixToFloatV(matrix);
            for (float value : values.v) Require(std::isfinite(value), "vertical sun must produce finite matrices");
        }
        Vector2 projected = ShadowPixel(worldPoint, matrices);
        Require(std::isfinite(projected.x) && std::isfinite(projected.y), "vertical sun projection must be finite");
    }

    Matrix projection = MatrixPerspective(60.0 * DEG2RAD, 16.0 / 9.0, 0.1, 400.0);
    Matrix inverse = MatrixInvert(projection);
    for (Vector4 point : {Vector4{0, 0, -0.2f, 1}, Vector4{1.2f, -0.7f, -5, 1}, Vector4{-12, 8, -100, 1}}) {
        Vector4 clip = Transform(point, projection);
        float u = clip.x / clip.w * 0.5f + 0.5f;
        float v = clip.y / clip.w * 0.5f + 0.5f;
        float depth = clip.z / clip.w * 0.5f + 0.5f;
        Vector4 reconstructed = Transform({u * 2 - 1, v * 2 - 1, depth * 2 - 1, 1}, inverse);
        Vector3 position = {reconstructed.x / reconstructed.w, reconstructed.y / reconstructed.w, reconstructed.z / reconstructed.w};
        float tolerance = 0.0001f * fabsf(point.z);
        Require(fabsf(position.x - point.x) < tolerance && fabsf(position.y - point.y) < tolerance && fabsf(position.z - point.z) < tolerance,
                "inverse projection must reconstruct view position from perspective depth and UV");
    }
    std::puts("PASS: stable shadow XY, integer texel steps, light-axis invariance, vertical sun, perspective depth reconstruction");
}
