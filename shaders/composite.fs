#version 330

// Exposure, bloom, AgX tonemapping and grading. Output is display-encoded
// with perceptual luma in alpha for FXAA.
#include "common/atmosphere.glsl"

in vec2 fragTexCoord;
uniform sampler2D uHDR;
uniform sampler2D uBloom;
uniform float uBloomStrength;
uniform int uDebugView;
out vec4 finalColor;

vec3 agxContrast(vec3 x) {
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
    return 15.5 * x4 * x2 - 40.14 * x4 * x + 31.96 * x4 - 6.868 * x2 * x + 0.4298 * x2 + 0.1191 * x - 0.00232;
}

vec3 agx(vec3 v) {
    const mat3 inset = mat3(0.842479062253094, 0.0423282422610123, 0.0423756549057051,
                            0.0784335999999992, 0.878468636469772, 0.0784336,
                            0.0792237451477643, 0.0791661274605434, 0.879142973793104);
    const float minEv = -12.47393, maxEv = 4.026069;
    v = inset * v;
    v = clamp(log2(max(v, vec3(1e-10))), minEv, maxEv);
    v = (v - minEv) / (maxEv - minEv);
    return agxContrast(v);
}

vec3 agxOutset(vec3 v) {
    const mat3 outset = mat3(1.19687900512017, -0.0528968517574562, -0.0529716355144438,
                             -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
                             -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
    return outset * v;
}

vec3 agxLook(vec3 v, float saturation, float power) {
    float luma = dot(v, vec3(0.2126, 0.7152, 0.0722));
    v = pow(max(v, vec3(0.0)), vec3(power));
    luma = pow(max(luma, 0.0), power);
    return luma + saturation * (v - luma);
}

void main() {
    vec2 uv = fragTexCoord;
    vec3 hdr = texture(uHDR, uv).rgb;
    vec3 bloom = texture(uBloom, uv).rgb / 6.0;
    vec3 color = mix(hdr, bloom, uBloomStrength) * uExposure.x;
    if (uDebugView == 1) { finalColor = vec4(hdr, 1.0); return; }
    if (uDebugView == 3) { finalColor = vec4(bloom, 1.0); return; }
    if (uDebugView == 6) { finalColor = vec4(texture(uSkyLUT, uv).rgb * uExposure.x * 2.0, 1.0); return; }

    // Scotopic shift: low light loses colour and drifts blue
    float night = uExposure.y;
    float lum = luminance(color);
    float scotopic = night * (1.0 - smoothstep(0.02, 0.5, lum));
    color = mix(color, vec3(lum) * vec3(0.72, 0.86, 1.18), scotopic * 0.55);

    // Filmic split tone before the curve: warm highlights, cool shadows
    color *= mix(vec3(0.95, 0.99, 1.07), vec3(1.05, 1.0, 0.94), smoothstep(0.02, 1.2, lum));

    vec3 mapped = agxOutset(agxLook(agx(color), 1.3, 1.1));

    // Vignette
    vec2 q = uv - 0.5;
    float vignette = 1.0 - dot(q, q) * 0.55;
    mapped *= mix(1.0, vignette, 0.85);

    // Dither against banding in skies and fog
    float dither = (ign(gl_FragCoord.xy) + ign(gl_FragCoord.xy + 17.0) - 1.0) / 255.0;
    mapped = saturate(mapped + dither);
    finalColor = vec4(mapped, dot(mapped, vec3(0.299, 0.587, 0.114)));
}
