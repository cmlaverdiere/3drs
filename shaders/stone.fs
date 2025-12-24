#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 finalColor;

// Lighting uniforms
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform vec3 fogColor;
uniform float fogDensity;
uniform vec3 viewPos;
uniform mat4 lightVP;
uniform sampler2D shadowMap;
uniform int shadowMapResolution;

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

// Calculate shadow factor (0.0 = full shadow, 1.0 = no shadow)
float CalculateShadow(vec3 fragPos, vec3 normal) {
    vec4 fragPosLightSpace = lightVP * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0) {
        return 1.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, -sunDirection)), 0.001);

    float shadow = 0.0;
    vec2 texelSize = vec2(1.0 / float(shadowMapResolution));
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float sampleDepth = texture(shadowMap, projCoords.xy + texelSize * vec2(x, y)).r;
            shadow += (currentDepth - bias > sampleDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    float fadeStart = 0.85;
    float fadeEdge = max(abs(projCoords.x * 2.0 - 1.0), abs(projCoords.y * 2.0 - 1.0));
    shadow *= 1.0 - smoothstep(fadeStart, 1.0, fadeEdge);

    return 1.0 - shadow;
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

    // Calculate lighting
    vec3 normal = normalize(fragNormal);
    float shadow = CalculateShadow(fragWorldPos, normal);

    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    vec3 litColor = stoneColor * (ambientColor + diffuse);

    // Apply fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    finalColor = vec4(litColor, 1.0);
}
