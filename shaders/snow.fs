#version 330

in vec2 vUV;
in vec3 vWorldPos;

#include "common/lighting.glsl"

void main() {
    float r = length(vUV);
    if (r > 1.0) discard;
    float alpha = smoothstep(1.0, 0.25, r) * 0.9;
    // Flakes scatter sky light and a little sunlight (no shadow lookups)
    vec3 albedo = vec3(0.9, 0.92, 0.96);
    vec3 light = ambientIrradiance(vec3(0.0, 1.0, 0.0)) * 1.2 + uLightColor.rgb * 0.8 +
                 pointLighting(defaultSurface(albedo, normalize(uCamera.xyz - vWorldPos)), vWorldPos,
                               normalize(uCamera.xyz - vWorldPos), vec3(0.04), 0.5);
    writeScene(albedo * light, vec3(0.0), vWorldPos, alpha);
}
