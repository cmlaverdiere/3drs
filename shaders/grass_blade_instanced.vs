#version 330

// Vertex attributes
in vec3 vertexPosition;      // Unit blade local position
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in mat4 instanceTransform;   // Per-instance transform (position, rotation, scale)

// Uniforms
uniform mat4 mvp;
uniform float time;
uniform vec3 viewPos;

// Outputs
out vec2 fragTexCoord;
out vec3 fragWorldPos;
out vec3 fragNormal;
out float bladeHeight;

void main() {
    // Apply instance transform to get world position
    vec4 worldPos = instanceTransform * vec4(vertexPosition, 1.0);

    // Height factor for wind (use texcoord.y: 0=base, 1=tip)
    float heightFactor = vertexTexCoord.y;

    // Wind animation based on world position
    float windStrength = 0.03;
    float windSpeed = 2.0;
    float phase = worldPos.x * 0.5 + worldPos.z * 0.4 + time * windSpeed;

    // Wind displacement - only affects top vertices
    float windX = sin(phase) * windStrength * heightFactor;
    float windZ = cos(phase * 0.7 + 1.3) * windStrength * 0.5 * heightFactor;

    worldPos.x += windX;
    worldPos.z += windZ;

    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(instanceTransform) * vertexNormal);
    fragTexCoord = vertexTexCoord;
    bladeHeight = heightFactor;

    gl_Position = mvp * worldPos;
}
