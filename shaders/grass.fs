#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

#include "common/lighting.glsl"

// Sand zone uniforms (grass-specific)
uniform int sandZoneCount;
uniform vec4 sandZones[16];

// Winter mode
uniform int winterMode;

out vec4 finalColor;

// Check how much this point is in sand (0 = grass, 1 = sand)
float getSandFactor(vec2 worldXZ) {
    float sandFactor = 0.0;
    for (int i = 0; i < sandZoneCount; i++) {
        vec2 center = sandZones[i].xy;
        vec2 halfSize = sandZones[i].zw * 0.5;
        vec2 d = abs(worldXZ - center) - halfSize;
        float distFromEdge = max(d.x, d.y);
        float factor = 1.0 - smoothstep(-5.0, 0.0, distFromEdge);
        sandFactor = max(sandFactor, factor);
    }
    return sandFactor;
}

void main() {
    vec2 worldXZ = fragWorldPos.xz;

    // Multi-scale noise for natural look
    float n1 = fbm(worldXZ * 0.5, 4);
    float n2 = fbm(worldXZ * 2.0 + 100.0, 4);
    float n3 = noise(worldXZ * 8.0);

    float combined = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;

    // Grass/Snow color palette (switches based on winter mode)
    vec3 darkGrass, midGrass, lightGrass;
    if (winterMode == 1) {
        // Snow colors
        darkGrass = vec3(0.85, 0.88, 0.92);
        midGrass = vec3(0.92, 0.94, 0.97);
        lightGrass = vec3(0.97, 0.98, 1.0);
    } else {
        // Normal grass colors
        darkGrass = vec3(0.1, 0.35, 0.1);
        midGrass = vec3(0.2, 0.5, 0.15);
        lightGrass = vec3(0.3, 0.6, 0.2);
    }

    // Sand color palette (also lighter in winter)
    vec3 darkSand, midSand, lightSand;
    if (winterMode == 1) {
        // Snowy sand
        darkSand = vec3(0.75, 0.72, 0.68);
        midSand = vec3(0.85, 0.82, 0.78);
        lightSand = vec3(0.92, 0.90, 0.87);
    } else {
        darkSand = vec3(0.6, 0.5, 0.3);
        midSand = vec3(0.76, 0.65, 0.45);
        lightSand = vec3(0.85, 0.75, 0.55);
    }

    // Blend between colors based on noise
    vec3 grassColor;
    if (combined < 0.4) {
        grassColor = mix(darkGrass, midGrass, combined / 0.4);
    } else {
        grassColor = mix(midGrass, lightGrass, (combined - 0.4) / 0.6);
    }

    vec3 sandColor;
    if (combined < 0.4) {
        sandColor = mix(darkSand, midSand, combined / 0.4);
    } else {
        sandColor = mix(midSand, lightSand, (combined - 0.4) / 0.6);
    }

    // Check if we're in a sand zone
    float sandFactor = getSandFactor(worldXZ);

    // Blend grass and sand
    vec3 groundColor = mix(grassColor, sandColor, sandFactor);
    groundColor += (n3 - 0.5) * 0.08;

    // Calculate lighting using common functions
    vec3 normal = normalize(fragNormal);
    float shadow = calcShadow(fragWorldPos, normal);

    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    vec3 pointLighting = calcAllPointLights(fragWorldPos, normal);

    vec3 litColor = groundColor * (ambientColor + diffuse + pointLighting);
    litColor = applyFog(litColor, fragWorldPos);

    finalColor = vec4(litColor, 1.0);
}
