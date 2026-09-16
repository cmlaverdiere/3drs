// Shared helpers for wall materials (brick, stone, wood planks)

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;
in vec4 fragColor;
in vec3 fragLocalPos;

uniform float uWallBase;   // world height of the wall's base

#include "lighting.glsl"

// Planar mapping by dominant axis; u runs along the wall, v up.
vec2 wallUV(vec3 N, vec3 p) {
    vec3 a = abs(N);
    if (a.y > 0.7) return p.xz;
    return a.x > a.z ? vec2(p.z * sign(N.x), p.y) : vec2(-p.x * sign(N.z), p.y);
}

// Weathering common to all walls: grime at the base, moss or snow on top faces
vec3 weatherWall(vec3 albedo, vec3 N, vec3 p, out float roughnessBoost) {
    float above = p.y - uWallBase;
    float grimeNoise = fbm(p.xz * 0.9 + p.y * 0.4, 3);
    float grime = (1.0 - smoothstep(0.0, 0.9 + grimeNoise * 0.8, above)) * 0.55;
    albedo = mix(albedo, albedo * vec3(0.45, 0.42, 0.36), grime);
    float streaks = smoothstep(0.55, 0.85, noise(vec2(p.x * 3.1 + p.z * 3.1, p.y * 0.25))) * 0.25;
    albedo *= 1.0 - streaks * (1.0 - abs(N.y));
    roughnessBoost = grime * 0.1;
    float top = smoothstep(0.6, 0.95, N.y);
    if (uSeasonId == 3) {
        albedo = mix(albedo, vec3(0.82, 0.85, 0.9), top);
    } else {
        // Walkable tops (decks, battlements) stay mostly clean; moss clings to edges
        float moss = top * smoothstep(0.62, 0.8, fbm(p.xz * 1.7, 3)) * 0.5;
        moss += (1.0 - smoothstep(0.0, 0.35, above)) * smoothstep(0.55, 0.75, grimeNoise) * 0.8;
        albedo = mix(albedo, vec3(0.05, 0.09, 0.02), saturate(moss) * (uSeasonId == 2 ? 0.5 : 0.8));
    }
    return albedo;
}
