#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float time;

// Sharp hash for jagged edges
float hash(vec2 p) {
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

// Hash for particles
float hash1(float n) {
    return fract(sin(n) * 43758.5453);
}

// Raw noise (no smoothing for sharp edges)
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    // Linear interpolation for sharper look
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Turbulent noise - absolute value creates sharp creases
float turbulence(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++) {
        // abs() creates sharp V-shaped creases
        value += amplitude * abs(noise(p * frequency) * 2.0 - 1.0);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Billowy noise for main flame shape
float billowNoise(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++) {
        float n = noise(p * frequency);
        // Square to create sharper peaks
        n = n * n;
        value += amplitude * n;
        frequency *= 2.2;
        amplitude *= 0.45;
    }
    return value;
}

void main() {
    vec2 uv = fragTexCoord;

    // Center UV horizontally
    float x = (uv.x - 0.5) * 2.0;
    float y = 1.0 - uv.y;  // Flip Y so flame goes upward

    // === MAIN FLAME SHAPE ===
    // Jagged flame edge using turbulence
    float scrollSpeed = 4.0;
    vec2 flameUV = vec2(x * 3.0, y * 2.5 - time * scrollSpeed);

    // Sharp turbulent distortion
    float turb = turbulence(flameUV * 1.5, 5);
    float distortX = turbulence(flameUV + vec2(100.0, 0.0), 4) * 0.3;

    // Distort x position for jagged edges
    float jaggedX = x + (turb - 0.5) * 0.4 * (1.0 - y * 0.5);
    jaggedX += (distortX - 0.15) * (1.0 - y);

    // Base flame width - very tapered
    float baseWidth = 0.6 * pow(1.0 - y, 0.7);
    baseWidth += turb * 0.15 * (1.0 - y);

    // Distance from jagged center
    float dist = abs(jaggedX) / max(baseWidth, 0.001);

    // Sharp edge cutoff
    float edge = 1.0 - smoothstep(0.5, 1.0, dist);

    // Additional high-frequency jaggedness at edges
    float edgeNoise = noise(vec2(x * 20.0, y * 30.0 - time * 8.0));
    edge *= smoothstep(0.3 - edgeNoise * 0.2, 0.5, 1.0 - dist);

    // Vertical fade
    float topFade = 1.0 - smoothstep(0.4, 1.0, y);
    topFade = pow(topFade, 0.8);  // Sharper falloff

    // Core flame
    float coreWidth = 0.25 * (1.0 - y * 0.8);
    float core = 1.0 - smoothstep(0.0, coreWidth, abs(x));
    core *= (1.0 - y * 0.7);

    // Inner bright streaks
    float streakNoise = turbulence(vec2(x * 8.0, y * 6.0 - time * 6.0), 3);
    float streaks = pow(streakNoise, 2.0) * core * 0.8;

    // Combine
    float intensity = edge * topFade;
    intensity = max(intensity, core * 0.9);
    intensity += streaks;

    // === PARTICLES / EMBERS ===
    float particles = 0.0;
    for (int i = 0; i < 12; i++) {
        float fi = float(i);
        // Particle seed
        float seed = hash1(fi * 127.1);
        float seed2 = hash1(fi * 311.7);

        // Particle position - rises over time
        float particleSpeed = 2.0 + seed * 3.0;
        float particleY = fract(seed2 + time * particleSpeed * 0.3);
        float particleX = (seed - 0.5) * 0.8;

        // Drift sideways as it rises
        particleX += sin(time * 2.0 + fi) * 0.1 * particleY;

        // Particle size (smaller as it rises)
        float pSize = 0.03 * (1.0 - particleY * 0.7);

        // Distance to particle
        float pDist = length(vec2(x - particleX, y - particleY));

        // Sharp particle
        float particle = 1.0 - smoothstep(0.0, pSize, pDist);
        particle *= smoothstep(0.0, 0.1, particleY);  // Fade in
        particle *= 1.0 - smoothstep(0.7, 1.0, particleY);  // Fade out

        // Flicker
        particle *= 0.7 + 0.3 * sin(time * 20.0 + fi * 10.0);

        particles += particle * 0.7;
    }

    intensity += particles;

    // === FLICKERING ===
    float flicker = 0.8 + 0.2 * sin(time * 15.0) * sin(time * 23.0) * sin(time * 8.0 + 0.5);
    intensity *= flicker;

    // Discard transparent
    if (intensity < 0.08) discard;

    // === COLORS ===
    vec3 colorCore = vec3(1.0, 1.0, 0.9);      // White-yellow core
    vec3 colorHot = vec3(1.0, 0.85, 0.2);      // Bright yellow
    vec3 colorMid = vec3(1.0, 0.5, 0.05);      // Orange
    vec3 colorCool = vec3(0.95, 0.2, 0.02);    // Red-orange
    vec3 colorEdge = vec3(0.5, 0.05, 0.0);     // Dark red
    vec3 colorParticle = vec3(1.0, 0.6, 0.1);  // Ember orange

    vec3 fireColor;
    if (intensity > 0.85) {
        fireColor = mix(colorHot, colorCore, (intensity - 0.85) / 0.15);
    } else if (intensity > 0.6) {
        fireColor = mix(colorMid, colorHot, (intensity - 0.6) / 0.25);
    } else if (intensity > 0.35) {
        fireColor = mix(colorCool, colorMid, (intensity - 0.35) / 0.25);
    } else if (intensity > 0.15) {
        fireColor = mix(colorEdge, colorCool, (intensity - 0.15) / 0.2);
    } else {
        fireColor = colorEdge * (intensity / 0.15);
    }

    // Particles are more orange/yellow
    fireColor = mix(fireColor, colorParticle, particles * 0.5);

    // Brightness
    fireColor *= 1.5;

    // Sharp alpha
    float alpha = smoothstep(0.08, 0.15, intensity);

    finalColor = vec4(fireColor, alpha);
}
