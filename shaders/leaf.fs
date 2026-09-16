#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;

in vec4 fragColor;        // sRGB leaf colour, alpha

#include "common/lighting.glsl"


// Procedural leaf shape using signed distance functions
float leafShape(vec2 uv) {
    // Center UV
    vec2 p = uv - vec2(0.5);

    // Scale for leaf proportions (taller than wide)
    p.x *= 1.8;

    // Leaf body - modified ellipse with pointed tip
    float leafBody = length(p * vec2(1.0, 0.7)) - 0.35;

    // Add point at top
    float tip = p.y - 0.25;
    leafBody = max(leafBody, tip);

    // Add point at bottom (stem attachment)
    float stem = -p.y - 0.35;
    leafBody = max(leafBody, stem);

    // Curve the sides inward slightly for more leaf-like shape
    float curve = abs(p.x) * 2.0 - (0.35 - abs(p.y) * 0.5);
    leafBody = max(leafBody, curve);

    return leafBody;
}

// Leaf vein pattern
float leafVeins(vec2 uv) {
    vec2 p = uv - vec2(0.5);

    // Central vein
    float centerVein = abs(p.x) - 0.015;

    // Side veins (angled lines from center)
    float veins = 1.0;
    for (int i = 1; i <= 4; i++) {
        float y = float(i) * 0.08;
        // Left side vein
        float leftVein = abs(p.x + (p.y + y) * 0.6) - 0.008;
        // Right side vein
        float rightVein = abs(p.x - (p.y + y) * 0.6) - 0.008;
        veins = min(veins, min(leftVein, rightVein));
    }

    return min(centerVein, veins);
}

void main() {
    // Get leaf shape
    float d = leafShape(fragTexCoord);

    // Sharp cutout
    if (d > 0.0) {
        discard;
    }

    // Base color from material diffuse color
    vec3 baseColor = fragColor.rgb;

    // Darken edges slightly
    float edge = smoothstep(-0.02, -0.08, d);
    baseColor *= 0.85 + 0.15 * edge;

    // Add vein detail (slightly darker)
    float vein = leafVeins(fragTexCoord);
    if (vein < 0.0) {
        baseColor *= 0.75;
    }

    // Add subtle variation across the leaf
    float variation = sin(fragTexCoord.x * 20.0) * sin(fragTexCoord.y * 15.0) * 0.05;
    baseColor += variation;

    // Simple lighting - fake ambient occlusion at edges
    float ao = smoothstep(-0.01, -0.15, d);
    baseColor *= 0.7 + 0.3 * ao;

    vec3 N = normalize(cross(dFdx(fragWorldPos), dFdy(fragWorldPos)));
    if (dot(N, uCamera.xyz - fragWorldPos) < 0.0) N = -N;
    Surface s = defaultSurface(srgbToLinear(saturate(baseColor)), N);
    s.wrap = 0.6;
    s.translucency = 0.8;
    s.roughness = 0.6;
    writeSurface(s, fragWorldPos, N, fragColor.a);
}
