#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;

uniform float time;

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

void main() {
    vec2 uv = fragWorldPos.xz;

    // Scrolling water patterns
    float n1 = noise(uv * 0.3 + vec2(time * 0.5, time * 0.3));
    float n2 = noise(uv * 0.5 - vec2(time * 0.4, time * 0.2));
    float n3 = noise(uv * 1.0 + vec2(time * 0.3, -time * 0.5));

    float combined = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;

    // Water colors
    vec3 deepWater = vec3(0.0, 0.2, 0.4);
    vec3 shallowWater = vec3(0.1, 0.4, 0.6);
    vec3 highlight = vec3(0.3, 0.6, 0.8);

    // Blend based on noise
    vec3 waterColor = mix(deepWater, shallowWater, combined);

    // Add sparkle highlights
    float sparkle = noise(uv * 2.0 + vec2(time * 2.0, time * 1.5));
    if (sparkle > 0.85) {
        waterColor = mix(waterColor, highlight, (sparkle - 0.85) * 6.0);
    }

    // Semi-transparent
    float alpha = 0.75 + combined * 0.15;

    finalColor = vec4(waterColor, alpha);
}
