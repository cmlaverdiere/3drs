#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;

#include "common/frame.glsl"

out vec3 fragWorldPos;
out vec3 fragBaseWorldPos;
out vec2 fragUV;

void main() {
    vec3 world = (matModel * vec4(vertexPosition, 1.0)).xyz;
    fragBaseWorldPos = world;
    fragUV = vertexTexCoord;
    float t = uTime;
    float h = sin(world.x * 0.35 + t * 1.1) * 0.035 + sin(world.z * 0.27 - t * 0.9) * 0.03 +
              sin((world.x + world.z) * 0.6 + t * 1.7) * 0.015;
    vec3 displaced = vertexPosition;
    displaced.y += h;
    fragWorldPos = (matModel * vec4(displaced, 1.0)).xyz;
    gl_Position = mvp * vec4(displaced, 1.0);
}
