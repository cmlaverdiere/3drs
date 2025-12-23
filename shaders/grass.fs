#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

// Simple hash function for noise
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

// Fractal noise (multiple octaves)
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec2 worldXZ = fragWorldPos.xz;

    // Multi-scale noise for natural look
    float n1 = fbm(worldXZ * 0.5);
    float n2 = fbm(worldXZ * 2.0 + 100.0);
    float n3 = noise(worldXZ * 8.0);

    float combined = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;

    // Grass color palette
    vec3 darkGrass = vec3(0.1, 0.35, 0.1);
    vec3 midGrass = vec3(0.2, 0.5, 0.15);
    vec3 lightGrass = vec3(0.3, 0.6, 0.2);

    // Blend between colors based on noise
    vec3 grassColor;
    if (combined < 0.4) {
        grassColor = mix(darkGrass, midGrass, combined / 0.4);
    } else {
        grassColor = mix(midGrass, lightGrass, (combined - 0.4) / 0.6);
    }

    // Add subtle variation
    grassColor += (n3 - 0.5) * 0.08;

    finalColor = vec4(grassColor, 1.0);
}
