// Scene lighting: cascaded shadows, sky ambient (SH), GGX, point lights,
// height fog / aerial perspective and the two-target scene output.
// Include in every lit fragment shader: #include "common/lighting.glsl"

#include "frame.glsl"
#include "atmosphere.glsl"
#include "clouds.glsl"
#include "shadows.glsl"

layout(location = 0) out vec4 outDirect;   // direct + emissive + fog in-scatter
layout(location = 1) out vec4 outAmbient;  // ambient (SSAO applies to this only)

// ---------------------------------------------------------------------------
// Ambient and BRDF
// ---------------------------------------------------------------------------
vec3 ambientIrradiance(vec3 n) {
    return max(uAmbientSH[0].rgb + uAmbientSH[1].rgb * n.x + uAmbientSH[2].rgb * n.y + uAmbientSH[3].rgb * n.z,
               vec3(0.0));
}

float D_GGX(float NdotH, float a) {
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float V_SmithGGX(float NdotV, float NdotL, float a) {
    float k = a * 0.5;
    return 0.25 / ((NdotV * (1.0 - k) + k) * (NdotL * (1.0 - k) + k));
}

vec3 F_Schlick(vec3 f0, float VdotH) {
    return f0 + (1.0 - f0) * pow(1.0 - VdotH, 5.0);
}

vec3 F_SchlickRoughness(vec3 f0, float NdotV, float rough) {
    return f0 + (max(vec3(1.0 - rough), f0) - f0) * pow(1.0 - NdotV, 5.0);
}

struct Surface {
    vec3 albedo;        // linear
    vec3 normal;        // shading normal
    float roughness;
    float metallic;
    float specular;     // dielectric reflectance scale (1 = 4%)
    float occlusion;    // material AO (cavities)
    float wrap;         // diffuse wrap (foliage)
    float translucency; // back-lit transmission
    float shadowSoftness;
    vec3 emissive;
};

Surface defaultSurface(vec3 albedo, vec3 normal) {
    Surface s;
    s.albedo = albedo;
    s.normal = normal;
    s.roughness = 0.7;
    s.metallic = 0.0;
    s.specular = 1.0;
    s.occlusion = 1.0;
    s.wrap = 0.0;
    s.translucency = 0.0;
    s.shadowSoftness = 0.03;
    s.emissive = vec3(0.0);
    return s;
}

// ---------------------------------------------------------------------------
// Point lights (lamps, campfires): smooth windowed inverse-square falloff
// ---------------------------------------------------------------------------
vec3 pointLighting(Surface s, vec3 worldPos, vec3 V, vec3 f0, float a) {
    vec3 sum = vec3(0.0);
    int count = int(uCounts.x);
    for (int i = 0; i < 16; i++) {
        if (i >= count) break;
        vec3 toLight = uPointPos[i].xyz - worldPos;
        float dist2 = dot(toLight, toLight);
        float radius = uPointPos[i].w;
        if (dist2 > radius * radius) continue;
        float dist = sqrt(dist2);
        vec3 L = toLight / max(dist, 1e-4);
        float window = pow(saturate(1.0 - pow(dist / radius, 4.0)), 2.0);
        float atten = window / (dist2 + 0.35);
        float NdotL = saturate((dot(s.normal, L) + s.wrap) / (1.0 + s.wrap));
        vec3 H = normalize(L + V);
        float NdotV = max(dot(s.normal, V), 1e-3);
        float NdotLs = max(dot(s.normal, L), 0.0);
        vec3 F = F_Schlick(f0, saturate(dot(V, H)));
        vec3 spec = D_GGX(saturate(dot(s.normal, H)), a) * V_SmithGGX(NdotV, max(NdotLs, 1e-3), a) * F * PI * NdotLs;
        vec3 diffuse = s.albedo * (1.0 - s.metallic) * NdotL;
        sum += (diffuse + spec) * uPointColor[i].rgb * atten;
    }
    return sum;
}

// ---------------------------------------------------------------------------
// Full surface shading. Returns direct (key light + point lights + emissive)
// and ambient (sky irradiance + sky reflections) separately.
// ---------------------------------------------------------------------------
void shadeSurface(Surface s, vec3 worldPos, vec3 geomNormal, out vec3 direct, out vec3 ambient) {
    vec3 V = normalize(uCamera.xyz - worldPos);
    vec3 N = s.normal;
    vec3 L = uLightDir.xyz;
    float a = max(s.roughness * s.roughness, 0.002);
    vec3 f0 = mix(vec3(0.04 * s.specular), s.albedo, s.metallic);
    float NdotV = max(dot(N, V), 1e-3);

    float shadow = shadowVisibility(worldPos, geomNormal, s.shadowSoftness);
    shadow *= cloudShadow(worldPos, L);

    // Key light
    float NdotLraw = dot(N, L);
    float NdotL = saturate(NdotLraw);
    float diffuseTerm = saturate((NdotLraw + s.wrap) / ((1.0 + s.wrap) * (1.0 + s.wrap)));
    vec3 H = normalize(L + V);
    vec3 F = F_Schlick(f0, saturate(dot(V, H)));
    vec3 spec = D_GGX(saturate(dot(N, H)), a) * V_SmithGGX(NdotV, max(NdotL, 1e-3), a) * F * PI * NdotL;
    vec3 kd = (1.0 - F) * (1.0 - s.metallic);
    vec3 keyDiffuse = s.albedo * kd * diffuseTerm;
    float backlit = pow(saturate(dot(-V, L)), 3.0) * 0.8 + 0.2 * saturate(-NdotLraw);
    vec3 transmitted = s.albedo * s.translucency * backlit;
    direct = (keyDiffuse + spec + transmitted) * uLightColor.rgb * shadow;
    direct += pointLighting(s, worldPos, V, f0, a);
    direct += s.emissive;

    // Ambient: diffuse sky irradiance plus a rough sky reflection
    vec3 Fr = F_SchlickRoughness(f0, NdotV, s.roughness);
    vec3 diffuseAmbient = s.albedo * (1.0 - Fr) * (1.0 - s.metallic) * ambientIrradiance(N);
    vec3 R = reflect(-V, N);
    vec3 envSpec = mix(ambientIrradiance(R), skyRadiance(R), (1.0 - s.roughness) * (1.0 - s.roughness));
    float horizon = saturate(1.0 + dot(R, geomNormal));
    ambient = (diffuseAmbient + envSpec * Fr * horizon * horizon) * s.occlusion;
}

// ---------------------------------------------------------------------------
// Height fog / aerial perspective: fog takes the colour of the sky behind it
// ---------------------------------------------------------------------------
float fogOpticalDepth(vec3 worldPos) {
    vec3 d = worldPos - uCamera.xyz;
    float dist = length(d);
    float h0 = uCamera.y - uFog.z;
    float k = uFog.y * d.y;
    float integral = abs(k) > 1e-4 ? (1.0 - exp(-k)) / k : 1.0 - 0.5 * k;
    return uFog.x * exp(-uFog.y * h0) * dist * integral;
}

void fogTerms(vec3 worldPos, out float transmittance, out vec3 inscatter) {
    float od = fogOpticalDepth(worldPos);
    transmittance = exp(-od);
    vec3 dir = normalize(worldPos - uCamera.xyz);
    inscatter = skyRadiance(dir) * (1.0 - transmittance) * uFog.w;
}

void writeScene(vec3 direct, vec3 ambient, vec3 worldPos, float alpha) {
    float T; vec3 inscatter;
    fogTerms(worldPos, T, inscatter);
    outDirect = vec4(direct * T + inscatter, alpha);
    outAmbient = vec4(ambient * T, alpha);
}

void writeSurface(Surface s, vec3 worldPos, vec3 geomNormal, float alpha) {
    vec3 direct, ambient;
    shadeSurface(s, worldPos, geomNormal, direct, ambient);
    writeScene(direct, ambient, worldPos, alpha);
}

// Screen-derivative bump mapping: perturbs N by the gradient of a height field
vec3 bumpNormal(vec3 N, vec3 worldPos, float height, float strength) {
    vec3 dpdx = dFdx(worldPos);
    vec3 dpdy = dFdy(worldPos);
    float dhdx = dFdx(height);
    float dhdy = dFdy(height);
    vec3 r1 = cross(dpdy, N);
    vec3 r2 = cross(N, dpdx);
    float det = dot(dpdx, r1);
    if (abs(det) < 1e-12) return N;
    vec3 grad = sign(det) * (dhdx * r1 + dhdy * r2);
    return normalize(abs(det) * N - grad * strength);
}
