#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform float time;
uniform vec3 viewPos;

out vec2 fragTexCoord;
out vec3 fragWorldPos;
out vec3 fragNormal;
out float bladeHeight;

void main() {
    // Positions are already in world space (baked mesh)
    // Use texcoord.y to determine if this is a top vertex (1.0) or bottom (0.0)
    float heightFactor = vertexTexCoord.y;

    // Wind animation - subtle for short grass
    float windStrength = 0.03;
    float windSpeed = 2.0;

    // Use world position for wind phase variation
    float phase = vertexPosition.x * 0.5 + vertexPosition.z * 0.4 + time * windSpeed;

    // Wind displacement - only affects top vertices
    float windX = sin(phase) * windStrength * heightFactor;
    float windZ = cos(phase * 0.7 + 1.3) * windStrength * 0.5 * heightFactor;

    vec3 animatedPos = vertexPosition;
    animatedPos.x += windX;
    animatedPos.z += windZ;

    fragWorldPos = animatedPos;
    fragNormal = normalize(vertexNormal);
    fragTexCoord = vertexTexCoord;
    bladeHeight = heightFactor;

    gl_Position = mvp * vec4(animatedPos, 1.0);
}
