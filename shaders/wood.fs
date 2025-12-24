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

    // Calculate lighting
    vec3 normal = normalize(fragNormal);
    float shadow = CalculateShadow(fragWorldPos, normal);

    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    vec3 litColor = woodColor * (ambientColor + diffuse);

    // Apply fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    finalColor = vec4(litColor, 1.0);
}
