#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;

out vec2 fragTexCoord;
out vec3 fragWorldPos;

// Hash function for noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Value noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal noise for terrain
float terrainNoise(vec2 p) {
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;

    // Large rolling hills
    value += noise(p * 0.02) * 2.5;

    // Medium bumps
    value += noise(p * 0.08) * 0.8;

    // Small details
    value += noise(p * 0.2) * 0.3;

    return value;
}

void main() {
    // Calculate world position for noise sampling
    vec3 worldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;

    // Apply terrain height displacement
    float height = terrainNoise(worldPos.xz);
    vec3 displacedPos = vertexPosition;
    displacedPos.y += height;

    fragTexCoord = vertexTexCoord;
    fragWorldPos = (matModel * vec4(displacedPos, 1.0)).xyz;
    gl_Position = mvp * vec4(displacedPos, 1.0);
}
