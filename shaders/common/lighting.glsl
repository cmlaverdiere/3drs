// Common lighting functions and uniforms
// Include this in fragment shaders with: #include "common/lighting.glsl"

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
uniform int shadowEnabled;
uniform float shadowDepthRange;

// Point lights
#define MAX_POINT_LIGHTS 16
uniform vec3 pointLightPositions[MAX_POINT_LIGHTS];
uniform vec3 pointLightColors[MAX_POINT_LIGHTS];
uniform int pointLightCount;

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
float fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Poisson disk samples for soft shadow sampling
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
);

// Calculate point light contribution
vec3 calcPointLight(vec3 lightPos, vec3 lightColor, vec3 fragPos, vec3 normal) {
    vec3 lightDir = lightPos - fragPos;
    float distance = length(lightDir);
    lightDir /= max(distance, 0.0001);

    // Soft point light settings
    float radius = 12.0;
    float intensity = 1.2;

    float attenuation = intensity / (1.0 + 0.15 * distance + 0.03 * distance * distance);
    attenuation *= (1.0 - smoothstep(radius * 0.1, radius, distance));

    float diff = max(dot(normal, lightDir), 0.0);
    return diff * lightColor * attenuation;
}

// Calculate all point lights
vec3 calcAllPointLights(vec3 fragPos, vec3 normal) {
    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < pointLightCount && i < MAX_POINT_LIGHTS; i++) {
        pointLighting += calcPointLight(pointLightPositions[i], pointLightColors[i], fragPos, normal);
    }
    return pointLighting;
}

// Calculate shadow factor with Poisson disk PCF (0.0 = full shadow, 1.0 = no shadow)
float calcShadow(vec3 fragPos, vec3 normal) {
    if (shadowEnabled == 0) return 1.0;
    vec4 fragPosLightSpace = lightVP * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 1.0;
    }

    float currentDepth = projCoords.z;
    // Bias in world units, converted to the orthographic depth interval.
    float slope = 1.0 - max(dot(normalize(normal), -sunDirection), 0.0);
    float bias = (0.025 + 0.08 * slope) / shadowDepthRange;

    // Compare each PCF tap against the receiver plane at that tap, rather than
    // against the centre depth. Otherwise even a flat sloping surface shadows
    // itself as the filter steps across neighbouring texels.
    vec3 dx = dFdx(projCoords);
    vec3 dy = dFdy(projCoords);
    float determinant = dx.x * dy.y - dx.y * dy.x;
    vec2 depthGradient = vec2(0.0);
    if (abs(determinant) > 1e-12) {
        depthGradient = vec2(dy.y * dx.z - dx.y * dy.z,
                             dx.x * dy.z - dy.x * dx.z) / determinant;
    }

    // PCF with Poisson disk sampling for softer shadows
    float shadow = 0.0;
    float spread = 2.0 / float(shadowMapResolution);
    // Fixed taps avoid per-fragment hash noise and remain stable as the camera moves.
    for (int i = 0; i < 16; i++) {
        vec2 offset = poissonDisk[i] * spread;
        float sampleDepth = texture(shadowMap, clamp(projCoords.xy + offset, vec2(0.0), vec2(1.0))).r;
        float receiverDepth = currentDepth + clamp(dot(depthGradient, offset), -0.002, 0.002);
        shadow += (receiverDepth - bias > sampleDepth) ? 1.0 : 0.0;
    }
    shadow /= 16.0;

    // Fade shadows at edge of shadow map
    float fadeStart = 0.85;
    float fadeEdge = max(abs(projCoords.x * 2.0 - 1.0), abs(projCoords.y * 2.0 - 1.0));
    shadow *= 1.0 - smoothstep(fadeStart, 1.0, fadeEdge);

    return 1.0 - shadow;
}

// Apply fog to color
vec3 applyFog(vec3 color, vec3 fragPos) {
    float dist = length(viewPos - fragPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    return mix(fogColor, color, fogFactor);
}
