#version 330

// Campfire flame: a quad that turns about the vertical axis to face the camera
in vec3 vertexPosition;

#include "common/frame.glsl"

uniform vec4 uFire;   // xyz base of the flame, w per-fire seed

out vec2 fragUV;      // x 0..1 across, y 0 at the base .. 1 at the top
out vec3 fragWorldPos;

const vec2 FLAME_SIZE = vec2(1.0, 1.9);

void main() {
    fragUV = vertexPosition.xz + 0.5;
    vec3 toCamera = uCamera.xyz - uFire.xyz;
    vec3 right = normalize(vec3(toCamera.z, 0.0, -toCamera.x) + vec3(1e-4, 0.0, 0.0));
    vec3 world = uFire.xyz + right * (fragUV.x - 0.5) * FLAME_SIZE.x + vec3(0.0, fragUV.y * FLAME_SIZE.y, 0.0);
    // Nudge toward the camera so the base clears the logs
    world += normalize(vec3(toCamera.x, 0.0, toCamera.z) + vec3(1e-4, 0.0, 0.0)) * 0.12;
    fragWorldPos = world;
    gl_Position = uViewProj * vec4(world, 1.0);
}
