#version 330

// Sky: atmosphere LUT + sun and moon discs + stars/milky way + a lit cloud layer.
#include "common/atmosphere.glsl"
#include "common/clouds.glsl"

in vec2 fragTexCoord;
layout(location = 0) out vec4 outDirect;
layout(location = 1) out vec4 outAmbient;

vec3 ambientIrradiance(vec3 n) {
    return max(uAmbientSH[0].rgb + uAmbientSH[1].rgb * n.x + uAmbientSH[2].rgb * n.y + uAmbientSH[3].rgb * n.z,
               vec3(0.0));
}

vec3 viewDirection(vec2 uv) {
    vec4 p = uInvViewProj * vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    return normalize(p.xyz / p.w - uCamera.xyz);
}

vec3 starField(vec3 dir) {
    vec3 p = dir * 150.0;
    // Keep every star at least ~1 pixel wide so it survives resolve and FXAA
    float pixel = max(length(fwidth(p)), 1e-4);
    vec3 id = floor(p);
    vec3 f = fract(p) - 0.5;
    float rnd = hash13(id);
    if (rnd < 0.93) return vec3(0.0);
    vec3 offset = (vec3(hash13(id + 1.3), hash13(id + 2.7), hash13(id + 5.1)) - 0.5) * 0.5;
    float d = length(f - offset);
    float magnitude = pow(hash13(id + 9.2), 5.0);
    float radius = max(mix(0.04, 0.12, magnitude), pixel * 0.8);
    float star = exp(-d * d / (radius * radius));
    float twinkle = 0.7 + 0.3 * sin(uTime * (1.5 + rnd * 4.0) + rnd * 91.0);
    float temperature = hash13(id + 4.4);
    vec3 tint = mix(vec3(0.65, 0.76, 1.0), vec3(1.0, 0.84, 0.66), temperature);
    float brightness = 0.12 + 0.9 * pow((rnd - 0.93) / 0.07, 3.0) + 2.5 * magnitude;
    return tint * star * brightness * twinkle * 1.8;
}

vec3 milkyWay(vec3 dir) {
    vec3 bandNormal = normalize(vec3(0.35, 0.55, -0.76));
    float band = dot(dir, bandNormal);
    float glow = exp(-band * band * 18.0);
    // Isotropic noise on the sphere: no azimuth seam or polar streaking
    float clouds = fbm3(dir * 6.0, 5);
    float dust = smoothstep(0.4, 0.7, fbm3(dir * 11.0 + 7.0, 4));
    return vec3(0.55, 0.62, 0.9) * glow * (0.3 + clouds) * (1.0 - 0.8 * dust * glow) * 0.02;
}

vec3 moonDisc(vec3 dir, out float coverage) {
    vec3 m = uMoonDir.xyz;
    const float radius = 0.0175;  // ~1 degree, larger than life
    vec3 up = abs(m.y) > 0.99 ? vec3(1, 0, 0) : vec3(0, 1, 0);
    vec3 tx = normalize(cross(up, m));
    vec3 ty = cross(m, tx);
    vec2 q = vec2(dot(dir, tx), dot(dir, ty)) / radius;
    float r2 = dot(q, q);
    coverage = smoothstep(1.0, 0.96, sqrt(r2)) * step(0.0, dot(dir, m));
    if (coverage <= 0.0) return vec3(0.0);
    vec3 n = normalize(q.x * tx + q.y * ty + sqrt(max(0.0, 1.0 - r2)) * m);
    float lit = smoothstep(-0.05, 0.25, dot(n, normalize(uSunDir.xyz + m * 1.6)));
    float maria = fbm(q * 2.3 + 4.0, 5);
    float craters = smoothstep(0.6, 0.9, noise(q * 9.0));
    vec3 albedo = vec3(0.92, 0.9, 0.86) * (0.62 + 0.45 * smoothstep(0.35, 0.65, maria)) * (1.0 - 0.15 * craters);
    return albedo * lit * 1.6;
}

// Illuminance reaching the cloud layer from a light (sun or moon)
vec3 lightAtClouds(vec3 lightDir, vec3 illuminance) {
    vec3 origin = vec3(0.0, kEarthRadius + kViewerAltitude + uClouds.y, 0.0);
    return illuminance * transmittanceToSpace(origin, lightDir, 8);
}

vec4 cloudLayer(vec3 dir, vec3 skyBehind) {
    if (dir.y < 0.015) return vec4(0.0);
    float t = (uClouds.y - uCamera.y) / dir.y;
    vec2 xz = uCamera.xz + dir.xz * t;
    float density = cloudDensity(xz, 6);
    if (density < 0.002) return vec4(0.0);

    vec3 color = vec3(0.0);
    for (int li = 0; li < 2; li++) {
        vec3 L = li == 0 ? uSunDir.xyz : uMoonDir.xyz;
        vec3 E = li == 0 ? uSunColor.rgb : uMoonColor.rgb;
        if (li == 1 && uMoonDir.w < 0.01) break;
        if (L.y < -0.12) continue;
        vec3 illum = lightAtClouds(L, E);
        // Short march toward the light through the layer
        vec2 stepXZ = L.xz / max(abs(L.y), 0.12) * 55.0;
        float od = 0.0;
        for (int i = 1; i <= 4; i++) od += cloudDensity(xz + stepXZ * float(i), 3);
        float beer = exp(-od * 1.1);
        float powder = 1.0 - exp(-density * 3.0);
        float c = dot(dir, L);
        float phase = mix(hgPhase(c, -0.15), hgPhase(c, 0.72), 0.55) * 4.0 * PI;
        color += illum / PI * (beer * phase * mix(0.55, 1.0, powder) + 0.12 * powder);
    }
    // Ambient from the sky above and a darker base
    vec3 ambientTop = ambientIrradiance(vec3(0, 1, 0));
    color += ambientTop * mix(1.35, 0.7, density) * 1.1;

    float alpha = 1.0 - exp(-density * 5.0);
    // Aerial perspective: distant clouds melt into the horizon haze
    float haze = 1.0 - exp(-t / 26000.0);
    color = mix(color, skyBehind, haze);
    alpha *= smoothstep(0.015, 0.09, dir.y);
    return vec4(color, alpha);
}

void main() {
    vec3 dir = viewDirection(fragTexCoord);
    vec3 sky = skyRadiance(dir);
    vec3 skyDir = vec3(dir.x, max(dir.y, 0.004), dir.z);

    vec3 T = transmittanceToSpace(atmosphereViewer(), normalize(skyDir), 12);
    float night = uSeason.w;

    // Stars and milky way sit behind the atmosphere
    vec3 space = (starField(dir) + milkyWay(dir)) * night * T;
    float moonCoverage;
    vec3 moon = moonDisc(dir, moonCoverage) * uMoonColor.rgb / max(uMoonColor.g, 1e-4) * 0.9 * T * uMoonDir.w;
    space = mix(space, vec3(0.0), moonCoverage) + moon;
    // Moon halo
    float mc = saturate(dot(dir, uMoonDir.xyz));
    space += uMoonColor.rgb * (pow(mc, 900.0) * 1.5 + pow(mc, 60.0) * 0.12) * T;

    vec3 color = sky + space * step(0.0, dir.y);
    color += sunDisc(dir, uSunDir.xyz, T) * uSunDir.w;

    vec4 clouds = cloudLayer(dir, sky);
    color = mix(color, clouds.rgb, clouds.a);

    outDirect = vec4(color, 1.0);
    outAmbient = vec4(0.0, 0.0, 0.0, 1.0);
}

