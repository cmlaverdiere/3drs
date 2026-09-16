#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragWorldPos;
out vec3 fragNormal;
out vec3 fragLocalPos;
#ifdef BEVEL
// Box-space position in metres and the box frame, for rounded-edge normals
out vec3 fragBoxPos;
flat out vec3 fragHalfExtent;
flat out mat3 fragBoxFrame;
#endif

void main() {
    fragLocalPos = vertexPosition;
    fragWorldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;
    fragNormal = normalize(mat3(matNormal) * vertexNormal);
#ifdef BEVEL
    vec3 scale = vec3(length(matModel[0].xyz), length(matModel[1].xyz), length(matModel[2].xyz));
    fragBoxFrame = mat3(matModel[0].xyz / scale.x, matModel[1].xyz / scale.y, matModel[2].xyz / scale.z);
    fragHalfExtent = scale * 0.5;
    fragBoxPos = vertexPosition * scale;
#endif
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
