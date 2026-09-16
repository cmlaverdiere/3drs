#version 330

// Campfire flame, drawn additively as HDR emission: domain-warped turbulence
// shaped into licking tongues, a blackbody colour ramp, rising sparks and a
// soft fade where it meets the logs and ground.
in vec2 fragUV;
in vec3 fragWorldPos;

layout(location = 0) out vec4 outDirect;
layout(location = 1) out vec4 outAmbient;

uniform vec4 uFire;
uniform sampler2D uSceneDepth;

#include "common/frame.glsl"

float linearDepth(float d) {
    return uProj[3][2] / (d * 2.0 - 1.0 + uProj[2][2]);
}

// Temperature 0..1 -> linear radiance (deep red, orange, yellow, near white)
vec3 blackbody(float t) {
    vec3 c = vec3(0.9, 0.12, 0.01) * smoothstep(0.0, 0.3, t);
    c = mix(c, vec3(1.0, 0.42, 0.06), smoothstep(0.25, 0.55, t));
    c = mix(c, vec3(1.0, 0.72, 0.28), smoothstep(0.55, 0.8, t));
    c = mix(c, vec3(1.0, 0.92, 0.75), smoothstep(0.8, 1.0, t));
    return c;
}

void main() {
    float seed = uFire.w * 17.0;
    float t = uTime + seed;
    vec2 p = vec2(fragUV.x - 0.5, fragUV.y);

    // Flame body occupies the lower ~60% of the quad; sparks use the rest
    float h = p.y / 0.62;
    vec2 q = vec2(p.x * 4.0, p.y * 3.0 - t * 2.4);
    float warp = fbm(q * 0.7 + vec2(seed, -t * 0.6), 3);
    float turb = fbm(q * 1.6 + vec2(warp * 2.2, warp * 1.2) + seed, 4);
    float sway = (warp - 0.5) * 0.22 * h + sin(t * 1.7 + p.y * 3.0) * 0.02 * h;
    float width = 0.3 * pow(saturate(1.0 - h * 0.92), 0.6) + 0.015;
    float x = abs(p.x - sway) / width;
    // Tongues: turbulence eats into the envelope, more so toward the tips
    float flame = (1.0 - x * x) + (turb - 0.5) * (0.9 + 1.4 * h) - h * h * 0.9;
    flame *= smoothstep(0.0, 0.07, p.y);
    flame = saturate(flame);
    float heat = flame * mix(1.15, 0.55, saturate(h)) * (0.85 + 0.3 * turb);

    vec3 emission = blackbody(saturate(heat)) * pow(flame, 1.6) * (3.0 + 16.0 * saturate(heat));

    // Sparks: short streaks rising, drifting with the wind and fading
    for (int i = 0; i < 14; i++) {
        float fi = float(i);
        float s1 = hash(vec2(fi, seed + 1.3));
        float s2 = hash(vec2(seed + 7.1, fi));
        float life = fract(s2 + t * (0.28 + 0.25 * s1));
        float y = 0.08 + life * 0.9;
        float xPos = (s1 - 0.5) * 0.25 + sin(t * 2.3 + fi * 4.1) * 0.05 * life + life * life * 0.12 * (s2 - 0.5);
        vec2 d = vec2((p.x - xPos) * 1.0, (p.y - y) * 0.35);
        float spark = exp(-dot(d, d) / 0.00002) * smoothstep(0.0, 0.1, life) * (1.0 - smoothstep(0.5, 1.0, life));
        spark *= 0.6 + 0.4 * sin(t * 25.0 + fi * 7.0);
        emission += vec3(1.0, 0.45, 0.08) * spark * 14.0;
    }

    // Soft intersection with the opaque scene
    float sceneZ = linearDepth(texture(uSceneDepth, gl_FragCoord.xy * uScreen.zw).r);
    float fade = saturate((sceneZ - linearDepth(gl_FragCoord.z)) / 0.2);
    emission *= fade;
    if (dot(emission, vec3(1.0)) < 1e-3) discard;

    outDirect = vec4(emission, 1.0);
    outAmbient = vec4(0.0);
}
