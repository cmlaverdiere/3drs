#version 330

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;
in float vT;
in float vGust;
flat in float vFlower;

#include "common/lighting.glsl"

void main() {
    // Dark, occluded base to a sunlit, slightly yellow tip
    vec3 tip = vColor * 1.45 + vec3(0.02, 0.02, 0.0);
    vec3 albedo = mix(vColor * 0.42, tip, smoothstep(0.0, 1.0, vT));
    albedo *= 1.0 + vGust * 0.22 * vT;   // bent blades show their paler side
    if (uSeasonId == 3) albedo = mix(albedo, vec3(0.72, 0.76, 0.82), smoothstep(0.6, 1.0, vT) * 0.5);
    if (vFlower > 0.5 && vT > 0.82) {
        vec3 flower = vFlower < 1.5 ? vec3(0.85, 0.7, 0.08) : (vFlower < 2.5 ? vec3(0.8, 0.8, 0.85) : vec3(0.6, 0.25, 0.65));
        albedo = flower;
    }
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;
    N = normalize(mix(N, vec3(0.0, 1.0, 0.0), 0.5));
    Surface s = defaultSurface(albedo, N);
    s.roughness = 0.55;
    s.specular = 0.7;
    s.wrap = 0.45;
    s.translucency = 0.75;
    s.occlusion = mix(0.28, 1.0, vT);
    s.shadowSoftness = 0.04;
    writeSurface(s, vWorldPos, vec3(0.0, 1.0, 0.0), 1.0);
}
