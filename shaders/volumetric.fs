#version 330

// Half-resolution volumetric light: shadowed key-light in-scattering through
// height fog (god rays), plus analytic halos around point lights.
#include "common/atmosphere.glsl"
#include "common/shadows.glsl"
#include "common/clouds.glsl"

in vec2 fragTexCoord;
uniform sampler2D uDepth;
out vec4 finalColor;

const int STEPS = 32;
const float MAX_DISTANCE = 150.0;

// Thin ground haze: the analytic fog already carries aerial perspective, this
// adds only the shadowed, strongly forward-scattered light (god rays).
float volumeDensity(vec3 p) {
    float height = exp(-max(p.y - uFog.z, -2.0) * 0.12);
    return 0.0006 + 0.0011 * height;
}

void main() {
    float depth = textureLod(uDepth, fragTexCoord, 0.0).r;
    vec4 wp = uInvViewProj * vec4(fragTexCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec3 world = wp.xyz / wp.w;
    vec3 ro = uCamera.xyz;
    vec3 rd = world - ro;
    float sceneDist = length(rd);
    rd /= max(sceneDist, 1e-4);
    float tMax = min(sceneDist, MAX_DISTANCE);

    vec3 result = vec3(0.0);
    vec3 L = uLightDir.xyz;
    if (uShadowParams.x > 0.5 && L.y > 0.0) {
        float jitter = ign(gl_FragCoord.xy);
        float dt = tMax / float(STEPS);
        float cosTheta = dot(rd, L);
        float phase = mix(hgPhase(cosTheta, 0.0), hgPhase(cosTheta, 0.76), 0.85);
        float transmittance = 1.0;
        float scattered = 0.0;
        for (int i = 0; i < STEPS; i++) {
            float t = (float(i) + jitter) * dt;
            vec3 p = ro + rd * t;
            float density = volumeDensity(p);
            scattered += transmittance * density * shadowSingleTap(p) * dt;
            transmittance *= exp(-density * dt);
        }
        float clouds = cloudShadow(ro + rd * min(tMax, 30.0), L);
        result += uLightColor.rgb * PI * phase * scattered * clouds;
    }

    // Point light halos: closed-form integral of inverse-square along the ray
    int count = int(uCounts.x);
    for (int i = 0; i < 16; i++) {
        if (i >= count) break;
        vec3 oc = uPointPos[i].xyz - ro;
        float tca = dot(oc, rd);
        float h2 = max(dot(oc, oc) - tca * tca, 0.04);
        float h = sqrt(h2);
        float radius = uPointPos[i].w;
        if (h > radius) continue;
        float integral = (atan((sceneDist - tca) / h) + atan(tca / h)) / h;
        float window = 1.0 - smoothstep(radius * 0.08, radius * 0.45, h);
        result += uPointColor[i].rgb * integral * window * 0.0009;
    }
    finalColor = vec4(result, 1.0);
}
