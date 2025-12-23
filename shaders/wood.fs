#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 finalColor;

// Hash function for noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float hash(float p) {
    return fract(sin(p * 127.1) * 43758.5453);
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

float noise(float p) {
    float i = floor(p);
    float f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), f);
}

// Wood grain pattern
float woodGrain(vec3 pos) {
    // Create rings based on distance from center of "log"
    float dist = length(pos.xz * 0.5);

    // Add variation to the rings
    float rings = sin(dist * 20.0 + noise(pos.xz * 2.0) * 3.0) * 0.5 + 0.5;

    // Add fine grain along Y axis
    float grain = noise(vec2(pos.y * 15.0, dist * 5.0)) * 0.3;

    return rings * 0.7 + grain;
}

void main() {
    // Wood color palette
    vec3 darkWood = vec3(0.35, 0.2, 0.1);    // Dark brown
    vec3 lightWood = vec3(0.6, 0.4, 0.2);    // Light brown
    vec3 midWood = vec3(0.5, 0.3, 0.15);     // Medium brown

    // Calculate wood pattern based on world position
    float pattern = woodGrain(fragWorldPos);

    // Add some noise variation
    float n = noise(fragWorldPos.xz * 4.0 + fragWorldPos.y * 2.0);
    pattern = pattern * 0.8 + n * 0.2;

    // Blend colors based on pattern
    vec3 woodColor;
    if (pattern < 0.4) {
        woodColor = mix(darkWood, midWood, pattern / 0.4);
    } else {
        woodColor = mix(midWood, lightWood, (pattern - 0.4) / 0.6);
    }

    // Simple lighting based on normal
    float light = max(dot(fragNormal, normalize(vec3(0.3, 1.0, 0.5))), 0.3);
    woodColor *= light;

    finalColor = vec4(woodColor, 1.0);
}
