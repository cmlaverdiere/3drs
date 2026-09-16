#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;
in float bladeHeight;

#include "common/lighting.glsl"
uniform int season;  // 0=Spring, 1=Summer, 2=Autumn, 3=Winter

out vec4 finalColor;

void main() {
    // Grass/snow blade color gradient - darker at base, lighter at tip
    vec3 baseColor, tipColor;
    float variation = fract(sin(dot(fragWorldPos.xz, vec2(12.9898, 78.233))) * 43758.5453);

    if (season == 3) {
        // === WINTER - Frost-covered grass blades ===
        vec3 frozenBase = vec3(0.70, 0.78, 0.88);
        vec3 snowyTip = vec3(0.92, 0.94, 0.98);

        float frostAmount = fract(sin(dot(fragWorldPos.xz, vec2(43.12, 17.89))) * 12345.67);

        if (frostAmount > 0.7) {
            baseColor = vec3(0.65, 0.75, 0.88);
            tipColor = vec3(0.80, 0.88, 0.95);
        } else if (frostAmount > 0.3) {
            baseColor = frozenBase;
            tipColor = snowyTip;
        } else {
            baseColor = vec3(0.82, 0.85, 0.90);
            tipColor = vec3(0.96, 0.97, 1.0);
        }
    } else if (season == 0) {
        // === SPRING - Fresh bright green with yellow tints ===
        baseColor = vec3(0.12, 0.45, 0.08);
        tipColor = vec3(0.40, 0.72, 0.20);
        // Add some yellow flower hints
        if (variation > 0.92) {
            tipColor = vec3(0.85, 0.80, 0.25);  // Yellow flower
        } else if (variation > 0.88) {
            tipColor = vec3(0.90, 0.70, 0.80);  // Pink flower
        }
    } else if (season == 2) {
        // === AUTUMN - Golden/orange/brown dying grass ===
        float autumnVariation = fract(sin(dot(fragWorldPos.xz, vec2(23.45, 67.89))) * 98765.43);
        if (autumnVariation > 0.7) {
            // Orange/red blade
            baseColor = vec3(0.45, 0.25, 0.08);
            tipColor = vec3(0.75, 0.40, 0.12);
        } else if (autumnVariation > 0.4) {
            // Golden/yellow blade
            baseColor = vec3(0.40, 0.32, 0.10);
            tipColor = vec3(0.70, 0.55, 0.18);
        } else {
            // Brown/dying blade
            baseColor = vec3(0.30, 0.22, 0.10);
            tipColor = vec3(0.50, 0.38, 0.15);
        }
    } else {
        // === SUMMER - Deep vibrant green (default) ===
        baseColor = vec3(0.08, 0.35, 0.06);
        tipColor = vec3(0.25, 0.65, 0.12);
    }
    vec3 bladeColor = mix(baseColor, tipColor, bladeHeight);

    // Add slight color variation based on world position
    bladeColor += (variation - 0.5) * 0.08;

    // Simple diffuse lighting
    vec3 normal = normalize(fragNormal);
    float NdotL = max(dot(normal, -sunDirection), 0.0);

    // Add some fake subsurface scattering for grass
    float backlight = max(dot(normal, sunDirection), 0.0) * 0.3;

    float shadow = calcShadow(fragWorldPos, normal);
    vec3 diffuse = sunColor * (NdotL + backlight) * shadow;
    vec3 litColor = bladeColor * (ambientColor + diffuse + calcAllPointLights(fragWorldPos, normal));

    // Distance fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    // Alpha fade at tip for softer look
    float alpha = smoothstep(0.0, 0.1, 1.0 - bladeHeight * 0.3);

    finalColor = vec4(litColor, alpha);
}
