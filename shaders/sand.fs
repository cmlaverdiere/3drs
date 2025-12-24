#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

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

// Calculate shadow factor (0.0 = full shadow, 1.0 = no shadow)
float CalculateShadow(vec3 fragPos, vec3 normal) {
    vec4 fragPosLightSpace = lightVP * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Check if fragment is outside shadow map
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0) {
        return 1.0;
    }

    float currentDepth = projCoords.z;

    // Bias based on surface angle to sun
    float bias = max(0.005 * (1.0 - dot(normal, -sunDirection)), 0.001);

    // PCF (3x3 kernel) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0 / float(shadowMapResolution));
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float sampleDepth = texture(shadowMap, projCoords.xy + texelSize * vec2(x, y)).r;
            shadow += (currentDepth - bias > sampleDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // Fade shadows at edge of shadow map
    float fadeStart = 0.85;
    float fadeEdge = max(abs(projCoords.x * 2.0 - 1.0), abs(projCoords.y * 2.0 - 1.0));
    shadow *= 1.0 - smoothstep(fadeStart, 1.0, fadeEdge);

    return 1.0 - shadow;
}

void main() {
    vec2 worldXZ = fragWorldPos.xz;

    // Multi-scale noise for natural look
    float n1 = fbm(worldXZ * 0.3);
    float n2 = fbm(worldXZ * 1.5 + 50.0);
    float n3 = noise(worldXZ * 6.0);

    float combined = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;

    // Desert sand color palette
    vec3 darkSand = vec3(0.76, 0.60, 0.42);   // Darker sandy brown
    vec3 midSand = vec3(0.87, 0.72, 0.53);    // Medium sand
    vec3 lightSand = vec3(0.95, 0.85, 0.65);  // Light golden sand

    // Blend between colors based on noise
    vec3 sandColor;
    if (combined < 0.4) {
        sandColor = mix(darkSand, midSand, combined / 0.4);
    } else {
        sandColor = mix(midSand, lightSand, (combined - 0.4) / 0.6);
    }

    // Add subtle variation for sand grain texture
    sandColor += (n3 - 0.5) * 0.06;

    // Calculate lighting
    vec3 normal = normalize(fragNormal);
    float shadow = CalculateShadow(fragWorldPos, normal);

    // Diffuse lighting
    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    // Combine ambient and diffuse
    vec3 litColor = sandColor * (ambientColor + diffuse);

    // Apply distance fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    finalColor = vec4(litColor, 1.0);
}
