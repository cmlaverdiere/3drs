#version 330

// Instanced grass blade: curved, wind-bent (gusts travel across the field),
// pushed aside by the player, thinned with distance via a per-blade LOD key.
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 instA;   // root xyz, facing angle
in vec4 instB;   // height, width, lod key, curvature
in vec4 instC;   // colour rgb, flower id

uniform mat4 mvp;

#include "common/frame.glsl"

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;
out float vT;
out float vGust;
flat out float vFlower;

const float RADIUS = 50.0;   // matches grass.cpp

void main() {
    vec3 root = instA.xyz;
    float height = instB.x;
    float width = instB.y;
    float lod = instB.z;
    vec2 toCam = root.xz - uCamera.xz;
    float dist = length(toCam);
    float keep = clamp(1.0 - (dist - 12.0) / 42.0, 0.06, 1.0);
    float fade = (1.0 - smoothstep(keep - 0.06, keep, lod)) * (1.0 - smoothstep(RADIUS - 10.0, RADIUS, dist));
    height *= fade;

    float t = vertexTexCoord.y;
    vec2 facing = vec2(cos(instA.w), sin(instA.w));
    vec2 wind = uWind.xy;
    // Travelling gusts: bright waves roll across the meadow
    float gust = fbm(root.xz * 0.05 - wind * uTime * 1.6, 2);
    gust = smoothstep(0.3, 0.8, gust);
    float flutter = sin(uTime * 3.3 + lod * 40.0 + root.x * 0.7) * 0.08 + sin(uTime * 5.1 + lod * 17.0) * 0.04;
    float bend = instB.w + (0.18 + gust * 0.85) * uWind.z + flutter;
    vec2 bendDir = wind;
    // The player parts the grass
    float push = (1.0 - smoothstep(0.25, 1.3, dist)) * 1.1;
    if (dist > 0.01) bendDir = normalize(bendDir * (0.4 + gust) + (toCam / dist) * push * 2.5);
    bend = clamp(bend + push, 0.0, 1.35);

    float angle = bend * t;
    float along = t * height;
    vec3 side = vec3(-facing.y, 0.0, facing.x) * vertexPosition.x * width * (1.0 - t * 0.9);
    vec3 world = root + side + vec3(bendDir.x, 0.0, bendDir.y) * sin(angle) * along * 0.85 +
                 vec3(0.0, cos(angle) * along, 0.0);

    vec3 bladeNormal = normalize(vec3(facing.x, 0.0, facing.y));
    vNormal = normalize(bladeNormal + vec3(bendDir.x, 0.0, bendDir.y) * -0.3 * bend + vec3(0.0, 0.25, 0.0));
    vWorldPos = world;
    vColor = instC.rgb;
    vFlower = instC.w;
    vT = t;
    vGust = gust;
    gl_Position = mvp * vec4(world, 1.0);
}
