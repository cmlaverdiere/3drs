#version 330

// HDR resolve: direct + ambient * AO (depth-aware upsample) + volumetric light
#include "common/frame.glsl"
#include "common/shadows.glsl"

in vec2 fragTexCoord;
uniform sampler2D uDirect;
uniform sampler2D uAmbient;
uniform sampler2D uAO;
uniform sampler2D uVolumetric;
uniform sampler2D uDepth;
uniform vec2 uHalfTexel;
uniform int uDebugView;
out vec4 finalColor;

float linearDepth(float d) {
    float ndc = d * 2.0 - 1.0;
    return uProj[3][2] / (ndc + uProj[2][2]);
}

void main() {
    vec2 uv = fragTexCoord;
    vec3 direct = texture(uDirect, uv).rgb;
    vec3 ambient = texture(uAmbient, uv).rgb;
    float z = linearDepth(texture(uDepth, uv).r);

    float ao = 0.0, weights = 0.0;
    for (int i = 0; i < 4; i++) {
        vec2 o = vec2(float(i & 1), float(i >> 1)) - 0.5;
        vec2 s = texture(uAO, uv + o * uHalfTexel).rg;
        float w = 1.0 / (1e-3 + abs(s.g - z) / max(z, 0.1));
        ao += s.r * w;
        weights += w;
    }
    ao /= max(weights, 1e-4);

    vec3 volumetric = texture(uVolumetric, uv).rgb;
    vec3 color = direct + ambient * ao + volumetric;
    if (uDebugView == 7) color = volumetric * 8.0;
    if (uDebugView == 8) color = direct;
    if (uDebugView == 9) color = vec3(ao);
    if (uDebugView == 10 || uDebugView == 11) {
        float d = texture(uDepth, uv).r;
        vec4 wp = uInvViewProj * vec4(uv * 2.0 - 1.0, d * 2.0 - 1.0, 1.0);
        vec3 world = wp.xyz / wp.w;
        if (uDebugView == 10) color = vec3(shadowSingleTap(world));
        else {
            int c = 0; float vd = viewDepth(world);
            if (vd > uShadowSplits.x) c = 1; if (vd > uShadowSplits.y) c = 2; if (vd > uShadowSplits.z) c = 3;
            vec4 lp = uShadowMatrix[c] * vec4(world, 1.0);
            color = vec3(lp.xy * 0.5 + 0.5, lp.z * 0.5 + 0.5);
        }
    }
    finalColor = vec4(max(color, vec3(0.0)), 1.0);
}
