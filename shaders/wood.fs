#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec4 fragColor;
in vec3 fragLocalPos;

uniform vec4 colDiffuse;

out vec4 finalColor;

#include "common/lighting.glsl"

// Wood grain pattern - uses local position for consistent look
float woodGrain(vec3 pos) {
    // Radial rings from center of trunk
    float dist = length(pos.xz * 0.5);
    float rings = sin(dist * 25.0 + noise(pos.xz * 2.0) * 3.0) * 0.5 + 0.5;

    // Vertical grain lines
    float grain = noise(vec2(pos.y * 20.0, dist * 8.0)) * 0.3;

    // Knots (occasional dark spots) - use 2D slice of position
    float knotNoise = noise(pos.xy * 3.0 + pos.z);
    float knots = smoothstep(0.7, 0.75, knotNoise) * 0.4;

    return rings * 0.6 + grain - knots;
}

// Bark texture for outer surface
float barkPattern(vec3 pos, vec3 normal) {
    // Detect if we're on the outside (horizontal normal)
    float outer = abs(normal.x) + abs(normal.z);

    // Vertical striations
    float stripes = sin(pos.y * 30.0 + noise(pos.xz * 5.0) * 4.0) * 0.5 + 0.5;

    // Rough texture
    float rough = fbm(vec2(pos.y * 8.0, atan(pos.x, pos.z) * 10.0), 3);

    return mix(0.5, stripes * 0.7 + rough * 0.3, outer);
}

void main() {
    // Base wood color (tinted by colDiffuse for variety)
    vec3 baseTint = colDiffuse.rgb;

    // Wood color palette - darker for bark
    vec3 darkBark = vec3(0.25, 0.15, 0.08) * baseTint * 2.0;
    vec3 midBark = vec3(0.4, 0.25, 0.12) * baseTint * 2.0;
    vec3 lightBark = vec3(0.5, 0.32, 0.15) * baseTint * 2.0;

    vec3 normal = normalize(fragNormal);

    // Calculate bark pattern (outer surface)
    float pattern = barkPattern(fragLocalPos, normal);

    // Add wood grain influence
    float grain = woodGrain(fragLocalPos);
    pattern = pattern * 0.7 + grain * 0.3;

    // Add noise variation
    float n = noise(fragLocalPos.xz * 6.0 + fragLocalPos.y * 3.0);
    pattern = pattern * 0.85 + n * 0.15;

    // Blend colors based on pattern
    vec3 woodColor;
    if (pattern < 0.4) {
        woodColor = mix(darkBark, midBark, pattern / 0.4);
    } else {
        woodColor = mix(midBark, lightBark, (pattern - 0.4) / 0.6);
    }

    // Lighting
    float shadow = calcShadow(fragWorldPos, normal);

    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    vec3 pointLighting = calcAllPointLights(fragWorldPos, normal);

    vec3 litColor = woodColor * (ambientColor + diffuse + pointLighting);
    litColor = applyFog(litColor, fragWorldPos);

    finalColor = vec4(litColor, 1.0);
}
