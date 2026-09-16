#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;

#include "common/lighting.glsl"
#include "common/terrain.glsl"

void main() {
    vec3 N = normalize(fragNormal);
    float slope = 1.0 - N.y;
    GroundSample g = sampleGround(fragWorldPos.xz, slope);

    // Near the camera the ground sits under grass blades: darken it so the
    // gaps between blades read as depth rather than bare paint.
    float dist = length(uCamera.xyz - fragWorldPos);
    float underBlades = g.grass * clamp(1.0 - (dist - 12.0) / 42.0, 0.0, 1.0) * (1.0 - smoothstep(40.0, 50.0, dist));
    vec3 albedo = g.albedo * mix(1.0, 0.62, underBlades);

    // Fade bump detail with distance to avoid shimmer
    float bumpFade = 1.0 - smoothstep(25.0, 70.0, dist);
    vec3 n = bumpNormal(N, fragWorldPos, g.height * bumpFade, 1.0);

    Surface s = defaultSurface(albedo, n);
    s.roughness = g.roughness;
    s.shadowSoftness = 0.05;
    s.occlusion = mix(1.0, 0.8, underBlades);
    // Snow sparkle: tiny glints that catch the light
    if (g.snow > 0.0) {
        float glint = step(0.985, hash(floor(fragWorldPos.xz * 40.0))) * g.snow;
        vec3 V = normalize(uCamera.xyz - fragWorldPos);
        float facing = pow(saturate(dot(reflect(-uLightDir.xyz, n), V)), 8.0);
        s.emissive = uLightColor.rgb * glint * facing * 6.0 * (1.0 - smoothstep(10.0, 30.0, dist));
    }
    writeSurface(s, fragWorldPos, N, 1.0);
}
