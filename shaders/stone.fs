#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 finalColor;

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

// Fractal noise
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Voronoi for stone block pattern
float voronoi(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    float minDist = 1.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash(i + neighbor) * vec2(0.8) + vec2(0.1);
            vec2 diff = neighbor + point - f;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

void main() {
    // Stone color palette
    vec3 darkStone = vec3(0.3, 0.3, 0.32);   // Dark gray
    vec3 midStone = vec3(0.5, 0.5, 0.52);    // Medium gray
    vec3 lightStone = vec3(0.65, 0.63, 0.6); // Light gray with slight warmth

    // Use world position for consistent texturing
    vec2 uv = fragWorldPos.xz + fragWorldPos.y * 0.3;

    // Create stone block pattern
    float blocks = voronoi(uv * 1.5);

    // Add rough texture
    float roughness = fbm(uv * 8.0) * 0.4;
    float detail = noise(uv * 20.0) * 0.15;

    // Combine patterns
    float pattern = blocks * 0.5 + roughness + detail;

    // Add cracks in the stone
    float cracks = 1.0 - smoothstep(0.02, 0.05, blocks);

    // Blend colors
    vec3 stoneColor;
    if (pattern < 0.35) {
        stoneColor = mix(darkStone, midStone, pattern / 0.35);
    } else {
        stoneColor = mix(midStone, lightStone, (pattern - 0.35) / 0.65);
    }

    // Darken cracks
    stoneColor = mix(stoneColor, darkStone * 0.5, cracks * 0.7);

    // Simple lighting
    float light = max(dot(fragNormal, normalize(vec3(0.3, 1.0, 0.5))), 0.35);
    stoneColor *= light;

    finalColor = vec4(stoneColor, 1.0);
}
