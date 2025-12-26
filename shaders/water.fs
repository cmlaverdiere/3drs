#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

uniform float time;

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

// Calculate shadow factor with reduced intensity for water
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
    float bias = max(0.002 * (1.0 - dot(normal, -sunDirection)), 0.0005);

    float shadow = 0.0;
    vec2 texelSize = vec2(1.0 / float(shadowMapResolution));
    // Use larger 5x5 kernel for softer water shadows
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            float sampleDepth = texture(shadowMap, projCoords.xy + texelSize * vec2(x, y)).r;
            shadow += (currentDepth - bias > sampleDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;

    // Reduce shadow intensity for water (it's reflective)
    shadow *= 0.5;

    float fadeStart = 0.85;
    float fadeEdge = max(abs(projCoords.x * 2.0 - 1.0), abs(projCoords.y * 2.0 - 1.0));
    shadow *= 1.0 - smoothstep(fadeStart, 1.0, fadeEdge);

    return 1.0 - shadow;
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

    // Calculate lighting
    vec3 normal = normalize(fragNormal);
    float shadow = CalculateShadow(fragWorldPos, normal);

    // Diffuse lighting
    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    // Simple specular for water shininess
    vec3 viewDir = normalize(viewPos - fragWorldPos);
    vec3 reflectDir = reflect(sunDirection, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = sunColor * spec * 0.5 * shadow;

    // Combine lighting
    vec3 litColor = waterColor * (ambientColor + diffuse) + specular;

    // Apply fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    // Semi-transparent
    float alpha = 0.75 + combined * 0.15;

    finalColor = vec4(litColor, alpha);
}
