// Cloud layer density, shared by the sky (clouds), the scene (cloud shadows)
// and the volumetric pass so shadows line up with the clouds overhead.

#include "frame.glsl"

// Coverage-thresholded, domain-warped fbm. xz in metres on the cloud plane.
float cloudDensity(vec2 xz, int octaves) {
    vec2 p = (xz + uClouds.zw) * 0.00042;
    vec2 warp = vec2(fbm(p * 0.7 + 3.1, 3), fbm(p * 0.7 + 9.7, 3)) - 0.5;
    p += warp * 0.9;
    float base = fbm(p, octaves);
    // Large-scale coverage variation: clear patches and heavier banks
    float coverage = uClouds.x + (noise(p * 0.18 + 5.3) - 0.5) * 0.35;
    float d = smoothstep(1.0 - coverage, 1.0 - coverage + 0.32, base);
    return d;
}

// Fraction of sunlight reaching a point after passing through the cloud layer
float cloudShadow(vec3 worldPos, vec3 lightDir) {
    if (lightDir.y < 0.03) return 1.0;
    float t = (uClouds.y - worldPos.y) / lightDir.y;
    vec2 xz = worldPos.xz + lightDir.xz * t;
    float d = cloudDensity(xz, 4);
    return 1.0 - 0.72 * d;
}
