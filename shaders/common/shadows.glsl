// Cascaded shadow map lookups (hardware PCF through sampler2DShadow).
#include "frame.glsl"

uniform sampler2DShadow uShadowMap;

const vec2 kShadowDisk[12] = vec2[](
    vec2(-0.326, -0.406), vec2(-0.840, -0.074), vec2(-0.696, 0.457), vec2(-0.203, 0.621),
    vec2(0.962, -0.195), vec2(0.473, -0.480), vec2(0.519, 0.767), vec2(0.185, -0.893),
    vec2(0.507, 0.064), vec2(0.896, 0.412), vec2(-0.322, -0.933), vec2(-0.792, -0.598)
);

float viewDepth(vec3 worldPos) {
    return -(uView * vec4(worldPos, 1.0)).z;
}

float sampleShadowCascade(int c, vec3 worldPos, vec3 N, vec3 L, float softness) {
    float texel = uShadowTexel[c];
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    // Normal offset removes acne without peter-panning contact shadows
    vec3 p = worldPos + N * texel * (1.2 + 2.2 * (1.0 - NdotL)) + L * texel * 0.6;
    vec4 lp = uShadowMatrix[c] * vec4(p, 1.0);
    vec3 uvz = lp.xyz * 0.5 + 0.5;
    if (uvz.z >= 1.0) return 1.0;
    vec2 tile = vec2(float(c & 1), float(c >> 1)) * 0.5;
    float radius = max(1.0, softness / texel) / 2048.0;
    float depth = uvz.z - texel * 0.4 / 400.0;
    float sum = 0.0;
    for (int i = 0; i < 12; i++) {
        vec2 uv = clamp(uvz.xy + kShadowDisk[i] * radius, vec2(0.0005), vec2(0.9995));
        sum += texture(uShadowMap, vec3(uv * 0.5 + tile, depth));
    }
    return sum / 12.0;
}

// Visibility of the key light (1 = lit). softness is the penumbra radius in metres.
float shadowVisibility(vec3 worldPos, vec3 N, float softness) {
    if (uShadowParams.x < 0.5) return 1.0;
    vec3 L = uLightDir.xyz;
    float d = viewDepth(worldPos);
    if (d > uShadowParams.w) return 1.0;
    int c = 0;
    if (d > uShadowSplits.x) c = 1;
    if (d > uShadowSplits.y) c = 2;
    if (d > uShadowSplits.z) c = 3;
    float vis = sampleShadowCascade(c, worldPos, N, L, softness);
    // Blend into the next cascade across the last 12% of this one
    float splitFar = uShadowSplits[c];
    float blend = smoothstep(splitFar * 0.88, splitFar, d);
    if (blend > 0.0 && c < 3) {
        vis = mix(vis, sampleShadowCascade(c + 1, worldPos, N, L, softness), blend);
    }
    float fade = smoothstep(uShadowParams.z, uShadowParams.w, d);
    return mix(vis, 1.0, fade);
}


// Single hardware-PCF tap, for ray marching (volumetric light)
float shadowSingleTap(vec3 worldPos) {
    float d = viewDepth(worldPos);
    int c = 0;
    if (d > uShadowSplits.x) c = 1;
    if (d > uShadowSplits.y) c = 2;
    if (d > uShadowSplits.z) c = 3;
    vec4 lp = uShadowMatrix[c] * vec4(worldPos, 1.0);
    vec3 uvz = lp.xyz * 0.5 + 0.5;
    if (any(lessThan(uvz, vec3(0.0))) || any(greaterThan(uvz, vec3(1.0)))) return 1.0;
    vec2 tile = vec2(float(c & 1), float(c >> 1)) * 0.5;
    return texture(uShadowMap, vec3(uvz.xy * 0.5 + tile, uvz.z - 0.0005));
}
