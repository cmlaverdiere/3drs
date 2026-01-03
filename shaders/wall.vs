#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;

out vec2 fragTexCoord;
out vec3 fragWorldPos;
out vec3 fragNormal;
out vec4 fragColor;
out vec3 fragLocalPos;

void main() {
    fragTexCoord = vertexTexCoord;
    fragLocalPos = vertexPosition;
    fragWorldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;
    fragNormal = normalize(mat3(matModel) * vertexNormal);
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
