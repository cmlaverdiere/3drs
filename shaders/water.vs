#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;
uniform float time;

out vec2 fragTexCoord;
out vec3 fragWorldPos;

void main() {
    vec3 worldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;

    // Gentle wave animation
    float wave1 = sin(worldPos.x * 0.5 + time * 2.0) * 0.1;
    float wave2 = sin(worldPos.z * 0.3 + time * 1.5) * 0.08;
    float wave3 = sin((worldPos.x + worldPos.z) * 0.4 + time * 2.5) * 0.05;

    vec3 displacedPos = vertexPosition;
    displacedPos.y += wave1 + wave2 + wave3;

    fragTexCoord = vertexTexCoord;
    fragWorldPos = (matModel * vec4(displacedPos, 1.0)).xyz;
    gl_Position = mvp * vec4(displacedPos, 1.0);
}
