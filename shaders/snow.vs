#version 330

// Camera-facing snowflake quads
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 instA;   // position xyz, radius

uniform mat4 mvp;

#include "common/frame.glsl"

out vec2 vUV;
out vec3 vWorldPos;

void main() {
    vec3 center = instA.xyz;
    vec3 fwd = normalize(center - uCamera.xyz);
    vec3 right = normalize(cross(fwd, vec3(0.0, 1.0, 0.0)));
    vec3 up = cross(right, fwd);
    vec3 world = center + (right * vertexTexCoord.x + up * vertexTexCoord.y) * instA.w;
    vUV = vertexTexCoord;
    vWorldPos = world;
    gl_Position = mvp * vec4(world, 1.0);
}
