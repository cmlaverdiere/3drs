#version 330

// Half-resolution normal-oriented hemisphere SSAO. Output: (ao, linear depth).
#include "common/frame.glsl"

in vec2 fragTexCoord;
uniform sampler2D uDepth;
uniform mat4 uInvProj;
out vec4 finalColor;

vec3 viewPosition(vec2 uv) {
    float depth = textureLod(uDepth, uv, 0.0).r;
    vec4 p = uInvProj * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    return p.xyz / p.w;
}

void main() {
    vec2 uv = fragTexCoord;
    float depth = textureLod(uDepth, uv, 0.0).r;
    if (depth >= 1.0) {
        finalColor = vec4(1.0, 60000.0, 0.0, 1.0);
        return;
    }
    vec3 P = viewPosition(uv);
    vec2 texel = uScreen.zw;
    // Pick the closer neighbour on each axis to avoid silhouette halos
    vec3 l = P - viewPosition(uv - vec2(texel.x, 0.0));
    vec3 r = viewPosition(uv + vec2(texel.x, 0.0)) - P;
    vec3 d = P - viewPosition(uv - vec2(0.0, texel.y));
    vec3 u = viewPosition(uv + vec2(0.0, texel.y)) - P;
    vec3 dx = abs(l.z) < abs(r.z) ? l : r;
    vec3 dy = abs(d.z) < abs(u.z) ? d : u;
    vec3 N = normalize(cross(dx, dy));
    if (dot(N, -P) < 0.0) N = -N;

    float dist = -P.z;
    float radius = 0.55 + dist * 0.012;
    float rotation = ign(gl_FragCoord.xy) * 6.2831853;
    vec3 rv = vec3(cos(rotation), sin(rotation), 0.0);
    vec3 T = normalize(rv - N * dot(rv, N) + vec3(1e-4, 0.0, 0.0));
    vec3 B = cross(N, T);

    const int SAMPLES = 14;
    float occlusion = 0.0;
    for (int i = 0; i < SAMPLES; i++) {
        float u1 = (float(i) + 0.5) / float(SAMPLES);
        float phi = float(i) * 2.39996323;
        float rr = sqrt(u1);
        vec3 h = vec3(rr * cos(phi), rr * sin(phi), sqrt(max(0.0, 1.0 - u1)));
        float scale = mix(0.12, 1.0, fract(float(i) * 0.7548776662 + 0.31));
        scale *= scale;
        vec3 S = P + (T * h.x + B * h.y + N * h.z) * radius * scale;
        vec4 clip = uProj * vec4(S, 1.0);
        vec2 suv = clip.xy / clip.w * 0.5 + 0.5;
        if (any(lessThan(suv, vec2(0.0))) || any(greaterThan(suv, vec2(1.0)))) continue;
        float sceneZ = viewPosition(suv).z;
        float range = smoothstep(0.0, 1.0, radius / max(abs(P.z - sceneZ), 1e-4));
        occlusion += (sceneZ >= S.z + 0.02 + dist * 0.0008 ? 1.0 : 0.0) * range;
    }
    float ao = 1.0 - occlusion / float(SAMPLES);
    ao = mix(ao, 1.0, smoothstep(90.0, 160.0, dist));
    finalColor = vec4(pow(saturate(ao), 1.6), dist, 0.0, 1.0);
}
