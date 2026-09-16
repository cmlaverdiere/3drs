#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragWorldPos;
out vec3 fragNormal;
out vec3 fragObjectNormal;  // Object-space normal for stable tri-planar mapping
out vec4 fragColor;
out vec3 fragObjectPos;  // Object-space position for texture mapping

void main() {
    // Store object-space position and normal for procedural texturing
    // These stay stable relative to the mesh regardless of world transform
    fragObjectPos = vertexPosition;
    fragObjectNormal = vertexNormal;

    // Compute world position and normal for lighting
    fragWorldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;
    fragNormal = normalize(mat3(matNormal) * vertexNormal);

    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
