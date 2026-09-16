#version 330

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec3 vColor;
in float vAO;
in vec3 vLocal;
flat in float vSeed;
flat in vec4 vExtra;

// Includes expand unconditionally (include-once), so the shared frame block
// comes first and only the lit variants pull in the lighting code.
#include "common/frame.glsl"
#ifndef SHADOW
#include "common/lighting.glsl"
#endif
#include "common/leaves.glsl"

void main() {
#ifdef CARDS
    float vein;
    float mask = leafMask(vUV, vExtra.a, vein);
    if (mask < 0.0) discard;
#endif
#ifndef SHADOW
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing && dot(N, uCamera.xyz - vWorldPos) < 0.0) N = -N;
  #if defined(CARDS) || defined(CORE)
    vec3 albedo = vColor;
    #ifdef CARDS
    // Per-card tone and hue shifts, darker veins and leaf edges
    float tone = 0.78 + 0.44 * vExtra.r;
    albedo *= tone * vec3(1.0 + (vExtra.b - 0.5) * 0.16, 1.0, 1.0 - (vExtra.b - 0.5) * 0.2);
    albedo *= mix(0.72, 1.0, vein) * mix(0.8, 1.0, smoothstep(0.0, 0.06, mask));
    // Spherical normals go edge-on at the canopy silhouette; bias them toward the
    // viewer so the rough sky Fresnel doesn't paint a grey rim on the leaves.
    vec3 n = normalize(N + normalize(uCamera.xyz - vWorldPos) * 0.35);
    #else
    // Inner foliage mass: clustered leaf clumps rather than a flat shell
    vec3 lp = vLocal * 6.0 + vSeed * 7.0;
    float clumps = vnoise3(lp) * 0.6 + vnoise3(lp * 2.7) * 0.4;
    albedo *= 0.62 * mix(0.5, 1.15, clumps);
    vec3 n = bumpNormal(N, vWorldPos, clumps * 0.12, 1.0);
    #endif
    Surface s = defaultSurface(albedo, n);
    s.roughness = 0.75;
    s.specular = 0.3;
    s.wrap = 0.55;
    s.translucency = 0.75;
    s.occlusion = vAO;
    s.shadowSoftness = 0.09;
    writeSurface(s, vWorldPos, N, 1.0);
  #elif defined(TRUNK)
    // Bark plates split by vertical fissures: ridged 3D noise on the unit
    // cylinder, stretched along the trunk, so there is no seam.
    vec2 dir = normalize(vLocal.xz + vec2(1e-5));
    float yh = vLocal.y;
    vec3 q = vec3(dir.x * 3.2, yh * 0.55, dir.y * 3.2) + vSeed * 11.0;
    float plates = fbm3(q, 3);
    float ridge = abs(plates - 0.5);
    float fw = fwidth(plates) * 1.5;
    float fissure = smoothstep(0.02, 0.08 + fw, ridge);           // 0 in the cracks
    float detailFade = 1.0 - smoothstep(0.015, 0.05, fwidth(yh * 6.0));
    float grain = vnoise3(vec3(dir.x * 22.0, yh * 3.0, dir.y * 22.0) + vSeed * 5.0);
    float flakes = smoothstep(0.55, 0.75, vnoise3(q * 4.0));
    float height = fissure * (0.75 + 0.25 * mix(0.5, grain, detailFade)) + flakes * 0.1 * detailFade;
    vec3 albedo = vColor * mix(0.22, 1.2, height) * (0.85 + 0.3 * plates);
    float lichen = smoothstep(0.62, 0.8, fbm3(q * 0.7 + 7.0, 3)) * smoothstep(0.3, 1.2, yh) * fissure;
    albedo = mix(albedo, vec3(0.2, 0.24, 0.12), lichen * 0.55);
    albedo = mix(albedo * 0.55, albedo, smoothstep(0.0, 0.5, yh));
    if (uSeasonId == 3) albedo = mix(albedo, vec3(0.8, 0.83, 0.88), smoothstep(0.55, 0.9, N.y) * 0.8);
    float bumpFade = 1.0 - smoothstep(8.0, 30.0, length(uCamera.xyz - vWorldPos));
    vec3 n = bumpNormal(N, vWorldPos, height * 0.02 * bumpFade, 1.0);
    Surface s = defaultSurface(albedo, n);
    s.roughness = 0.92;
    s.occlusion = vAO * mix(0.55, 1.0, fissure);
    writeSurface(s, vWorldPos, N, 1.0);
  #elif defined(PINE)
    // Needle streaks run down the fall line of each tier; clumps in world space
    vec3 down = normalize(vec3(0.0, -1.0, 0.0) + N * N.y + vec3(1e-4, 0.0, 0.0));
    vec3 wp = vWorldPos * 3.0;
    vec3 streakPos = wp - down * dot(wp, down) * 0.88;
    float clumps = fbm3(wp * 0.8, 3);
    float detailFade = 1.0 - smoothstep(0.08, 0.3, fwidth(streakPos.x * 5.0) + fwidth(streakPos.z * 5.0));
    float streaks = vnoise3(streakPos * 5.0);
    float needles = mix(0.5, streaks, detailFade) * 0.55 + clumps * 0.45;
    vec3 albedo = vColor * (0.5 + 0.85 * needles);
    vec3 n = bumpNormal(N, vWorldPos, (clumps * 0.05 + streaks * 0.012 * detailFade), 1.0);
    float snow = 0.0;
    if (uSeasonId == 3) {
        // Snow settles in clumpy patches on the upper surfaces; tips and undersides stay green
        float patches = fbm3(vWorldPos * 1.3 + vSeed * 17.0, 3) + (vnoise3(wp * 2.5) - 0.5) * 0.18;
        float cover = patches + (N.y - 0.72) * 1.4;
        snow = smoothstep(0.45, 0.5 + fwidth(cover), cover) * smoothstep(0.35, 0.7, vAO);
        albedo = mix(albedo, vec3(0.8, 0.84, 0.9) * (0.9 + 0.12 * streaks), snow);
    }
    Surface s = defaultSurface(albedo, n);
    s.roughness = mix(0.8, 0.6, snow);
    s.wrap = mix(0.3, 0.0, snow);
    s.translucency = mix(0.25, 0.0, snow);
    s.occlusion = vAO * mix(0.7, 1.0, clumps) * mix(0.85, 1.0, streaks * detailFade + (1.0 - detailFade) * 0.5);
    s.shadowSoftness = 0.07;
    writeSurface(s, vWorldPos, N, 1.0);
  #elif defined(ROCK)
    vec3 p = vLocal * 2.2;
    float large = fbm3(p * 0.9, 4);
    float strata = sin(vLocal.y * 14.0 + large * 6.0) * 0.5 + 0.5;
    float grit = vnoise3(p * 9.0);
    vec3 stone = mix(vec3(0.16, 0.15, 0.14), vec3(0.34, 0.32, 0.29), large);
    stone *= 0.85 + 0.2 * strata + 0.15 * grit;
    // Ore veins and nuggets
    float veinNoise = abs(fbm3(p * 1.4 + 3.0, 3) - 0.5);
    float veins = 1.0 - smoothstep(0.02, 0.06, veinNoise);
    float nuggets = smoothstep(0.72, 0.8, vnoise3(p * 3.5 + 11.0));
    float ore = saturate(veins * 0.85 + nuggets);
    vec3 albedo = mix(stone, vColor, ore);
    float height = large * 0.05 + grit * 0.012 - veins * 0.006 + nuggets * 0.01;
    vec3 n = bumpNormal(N, vWorldPos, height, 1.0);
    float up = smoothstep(0.45, 0.85, n.y);
    float coverNoise = fbm3(p * 1.7 + 20.0, 3);
    if (uSeasonId == 3) {
        albedo = mix(albedo, vec3(0.82, 0.85, 0.9), up * smoothstep(0.35, 0.55, coverNoise + 0.2));
    } else {
        float moss = up * smoothstep(0.5, 0.7, coverNoise) * (1.0 - ore);
        albedo = mix(albedo, vec3(0.07, 0.12, 0.03), moss * (uSeasonId == 2 ? 0.4 : 0.85));
    }
    Surface s = defaultSurface(albedo, n);
    s.roughness = mix(0.85, 0.32, ore);
    s.metallic = ore * 0.85;
    s.occlusion = vAO;
    s.shadowSoftness = 0.03;
    writeSurface(s, vWorldPos, N, 1.0);
  #endif
#endif
}
