#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragWorldPos;
out vec3 fragNormal;

void main() {
    fragWorldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;
    fragNormal = vertexNormal;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
