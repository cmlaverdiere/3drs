// Procedural ground material shared by the terrain and the grass blades, so
// blades take the colour of the ground they grow from.

#include "frame.glsl"

uniform int uSandCount;
uniform vec4 uSandZones[16];   // center xz, size xz
uniform int uWaterCount;
uniform vec4 uWaterZones[16];

float boxDistance(vec2 p, vec4 zone) {
    vec2 d = abs(p - zone.xy) - zone.zw * 0.5;
    return length(max(d, vec2(0.0))) + min(max(d.x, d.y), 0.0);
}

float sandAmount(vec2 xz) {
    float amount = 0.0;
    float wobble = (noise(xz * 0.11) - 0.5) * 7.0 + (noise(xz * 0.6) - 0.5) * 1.5;
    for (int i = 0; i < 16; i++) {
        if (i >= uSandCount) break;
        float d = boxDistance(xz, uSandZones[i]) + wobble;
        amount = max(amount, 1.0 - smoothstep(-5.0, 1.5, d));
    }
    return amount;
}

float waterDistance(vec2 xz) {
    float dist = 1e4;
    for (int i = 0; i < 16; i++) {
        if (i >= uWaterCount) break;
        dist = min(dist, boxDistance(xz, uWaterZones[i]));
    }
    return dist;
}

struct GroundSample {
    vec3 albedo;
    float roughness;
    float height;     // bump height (metres)
    float grass;      // 0..1 how grassy (blade density)
    float snow;
};

// Grass palette for the current season, varied by two noise values
vec3 grassColor(float variation, float dryness) {
    int season = uSeasonId;
    vec3 a, b, dry;
    if (season == 0) {          // spring: fresh, bright
        a = vec3(0.07, 0.19, 0.025); b = vec3(0.19, 0.34, 0.04); dry = vec3(0.30, 0.32, 0.08);
    } else if (season == 2) {   // autumn: golden
        a = vec3(0.14, 0.12, 0.03); b = vec3(0.32, 0.21, 0.05); dry = vec3(0.42, 0.27, 0.08);
    } else {                    // summer (and base under winter snow)
        a = vec3(0.05, 0.15, 0.022); b = vec3(0.13, 0.28, 0.035); dry = vec3(0.28, 0.26, 0.08);
    }
    return mix(mix(a, b, variation), dry, dryness);
}

GroundSample sampleGround(vec2 xz, float slope) {
    GroundSample g;
    float macro = fbm(xz * 0.011, 3);
    float meso = fbm(xz * 0.085 + 13.0, 3);
    float micro = noise(xz * 1.1);
    float fine = noise(xz * 6.5);

    // Grass with large dry patches and clumpy variation
    float dryness = smoothstep(0.55, 0.78, macro) * 0.55 + smoothstep(0.6, 0.9, meso) * 0.25;
    vec3 grass = grassColor(saturate(meso * 1.25 - 0.1 + (micro - 0.5) * 0.3), dryness);
    grass *= 0.8 + 0.35 * micro;

    // Dirt: worn patches, steep slopes, mud along the water
    vec3 dirt = mix(vec3(0.15, 0.10, 0.06), vec3(0.24, 0.18, 0.11), fine);
    float waterDist = waterDistance(xz);
    float shore = 1.0 - smoothstep(0.0, 3.5 + micro * 2.0, waterDist);
    float worn = smoothstep(0.68, 0.8, fbm(xz * 0.045 + 31.0, 3)) * 0.8;
    float dirtAmount = max(max(worn, smoothstep(0.18, 0.35, slope)), shore * 0.85);
    dirtAmount = saturate(dirtAmount + (fine - 0.5) * 0.35 * dirtAmount);

    vec3 albedo = mix(grass, dirt, dirtAmount);
    float roughness = 0.92;
    float height = micro * 0.03 + fine * 0.012;
    float grassiness = 1.0 - dirtAmount;

    int season = uSeasonId;
    if (season == 0) {
        // Scattered flowers in spring meadows
        vec4 v = voronoiCell(xz * 2.2);
        float petal = step(0.86, hash(v.zw)) * (1.0 - smoothstep(0.06, 0.13, v.x)) * grassiness;
        vec3 flower = hash(v.zw + 3.0) > 0.5 ? vec3(0.75, 0.62, 0.08) : vec3(0.7, 0.35, 0.55);
        albedo = mix(albedo, flower, petal);
    } else if (season == 2) {
        // Fallen leaves
        vec4 v = voronoiCell(xz * 3.1);
        float h = hash(v.zw);
        float coverage = smoothstep(0.35, 0.65, fbm(xz * 0.12, 2));
        float leaf = step(1.0 - coverage * 0.7, h) * (1.0 - smoothstep(0.18, 0.3, v.x));
        vec3 leafColor = h > 0.8 ? vec3(0.42, 0.06, 0.02) : (h > 0.6 ? vec3(0.55, 0.2, 0.02) : vec3(0.45, 0.3, 0.04));
        albedo = mix(albedo, leafColor * (0.7 + 0.5 * hash(v.zw + 1.7)), leaf);
        height += leaf * 0.01;
    }

    // Sand zones: rippled dunes, darker and wet near water
    float sand = sandAmount(xz);
    if (sand > 0.0) {
        vec2 dir = normalize(vec2(0.8, 0.6));
        float ripple = sin(dot(xz, dir) * 5.5 + noise(xz * 0.4) * 6.0) * 0.5 + 0.5;
        vec3 sandColor = mix(vec3(0.40, 0.30, 0.17), vec3(0.56, 0.44, 0.27), meso * 0.7 + fine * 0.3);
        sandColor *= 0.92 + 0.12 * ripple;
        sandColor = mix(sandColor, sandColor * 0.55, shore);
        albedo = mix(albedo, sandColor, sand);
        roughness = mix(roughness, mix(0.95, 0.5, shore), sand);
        height = mix(height, ripple * 0.025 + fine * 0.006, sand);
        grassiness *= 1.0 - sand;
    }

    // Winter snow blanket with drifts and occasional bare earth
    g.snow = 0.0;
    if (season == 3) {
        float drift = fbm(xz * 0.06, 3);
        float bare = smoothstep(0.08, 0.02, noise(xz * 0.35 + 50.0)) * 0.9 + shore * 0.6;
        float snow = saturate(1.0 - bare - smoothstep(0.4, 0.7, slope));
        vec3 snowColor = mix(vec3(0.68, 0.72, 0.80), vec3(0.86, 0.88, 0.92), drift) * (0.96 + 0.06 * fine);
        albedo = mix(albedo * 0.6, snowColor, snow);
        roughness = mix(roughness, 0.55, snow);
        height = mix(height, drift * 0.06 + fine * 0.004, snow);
        g.snow = snow;
        grassiness *= 1.0 - snow * 0.85;
    }

    g.albedo = albedo;
    g.roughness = roughness;
    g.height = height;
    g.grass = grassiness;
    return g;
}
