#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float time;

out vec2 fragTexCoord;
out vec3 fragWorldPos;
out vec3 fragNormal;

void main() {
    vec3 worldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;

    // Wave parameters
    float freq1 = 0.5, amp1 = 0.1, speed1 = 2.0;
    float freq2 = 0.3, amp2 = 0.08, speed2 = 1.5;
    float freq3 = 0.4, amp3 = 0.05, speed3 = 2.5;

    // Wave heights
    float wave1 = sin(worldPos.x * freq1 + time * speed1) * amp1;
    float wave2 = sin(worldPos.z * freq2 + time * speed2) * amp2;
    float wave3 = sin((worldPos.x + worldPos.z) * freq3 + time * speed3) * amp3;

    // Compute normal from wave derivatives
    // dh/dx for each wave
    float dx1 = cos(worldPos.x * freq1 + time * speed1) * freq1 * amp1;
    float dx3 = cos((worldPos.x + worldPos.z) * freq3 + time * speed3) * freq3 * amp3;
    float dhdx = dx1 + dx3;

    // dh/dz for each wave
    float dz2 = cos(worldPos.z * freq2 + time * speed2) * freq2 * amp2;
    float dz3 = cos((worldPos.x + worldPos.z) * freq3 + time * speed3) * freq3 * amp3;
    float dhdz = dz2 + dz3;

    // Normal from cross product of tangent vectors
    // T_x = (1, dh/dx, 0), T_z = (0, dh/dz, 1)
    // N = T_x cross T_z = (dh/dx, 1, dh/dz) - then normalize
    vec3 normal = normalize(vec3(-dhdx, 1.0, -dhdz));

    vec3 displacedPos = vertexPosition;
    displacedPos.y += wave1 + wave2 + wave3;

    fragTexCoord = vertexTexCoord;
    fragWorldPos = (matModel * vec4(displacedPos, 1.0)).xyz;
    fragNormal = normalize(mat3(matNormal) * normal);
    gl_Position = mvp * vec4(displacedPos, 1.0);
}
