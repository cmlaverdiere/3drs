#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 finalColor;

#include "common/lighting.glsl"

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

void main() {
    // Stone color palette
    vec3 darkStone = vec3(0.3, 0.3, 0.32);
    vec3 midStone = vec3(0.5, 0.5, 0.52);
    vec3 lightStone = vec3(0.65, 0.63, 0.6);

    // Use world position for consistent texturing
    vec2 uv = fragWorldPos.xz + fragWorldPos.y * 0.3;

    // Create stone block pattern
    float blocks = voronoi(uv * 1.5);
    float roughness = fbm(uv * 8.0, 5) * 0.4;
    float detail = noise(uv * 20.0) * 0.15;
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
    float shadow = calcShadow(fragWorldPos, normal);

    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    vec3 pointLighting = calcAllPointLights(fragWorldPos, normal);

    vec3 litColor = stoneColor * (ambientColor + diffuse + pointLighting);
    litColor = applyFog(litColor, fragWorldPos);

    finalColor = vec4(litColor, 1.0);
}
