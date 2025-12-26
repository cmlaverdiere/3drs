#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;

out vec2 fragTexCoord;
out vec3 fragWorldPos;

void main() {
    fragTexCoord = vertexTexCoord;
    fragWorldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
