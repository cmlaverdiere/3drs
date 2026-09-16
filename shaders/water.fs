#version 330

// Water: procedural ripple normals, Fresnel sky reflection, sun glint,
// depth-based absorption of the refracted scene and shoreline foam.
in vec3 fragWorldPos;
in vec3 fragBaseWorldPos;
in vec2 fragUV;

uniform vec2 uWaterSize;   // metres, for soft rectangle edges

uniform sampler2D uSceneColor;   // opaque scene (half res)
uniform sampler2D uSceneDepth;   // opaque depth

#include "common/lighting.glsl"

float linearDepth(float d) {
    return uProj[3][2] / (d * 2.0 - 1.0 + uProj[2][2]);
}

float waveHeight(vec2 p) {
    float t = uTime;
    vec2 wind = uWind.xy;
    float h = fbm(p * 0.35 + wind * t * 0.25, 3) * 0.6;
    h += fbm(p * 1.3 - vec2(wind.y, -wind.x) * t * 0.35 + 7.0, 3) * 0.3;
    h += noise(p * 4.5 + wind * t * 0.9) * 0.1;
    return h;
}

void main() {
    vec2 p = fragBaseWorldPos.xz;
    float e = 0.06;
    float h0 = waveHeight(p);
    float hx = waveHeight(p + vec2(e, 0.0));
    float hz = waveHeight(p + vec2(0.0, e));
    vec3 V = normalize(uCamera.xyz - fragWorldPos);
    float dist = length(uCamera.xyz - fragWorldPos);
    float detail = mix(0.55, 0.12, smoothstep(5.0, 120.0, dist));
    vec3 N = normalize(vec3(-(hx - h0) / e * detail, 1.0, -(hz - h0) / e * detail));

    // Depth of water along the view ray
    vec2 screenUV = gl_FragCoord.xy * uScreen.zw;
    float surfaceZ = linearDepth(gl_FragCoord.z);
    float groundZ = linearDepth(texture(uSceneDepth, screenUV).r);
    float thickness = max(groundZ - surfaceZ, 0.0);
    // Refraction offset, pulled back where it would sample above-water geometry
    vec2 offset = N.xz * 0.04 * saturate(thickness * 0.5);
    vec2 refrUV = screenUV + offset;
    float refrZ = linearDepth(texture(uSceneDepth, refrUV).r);
    if (refrZ < surfaceZ) { refrUV = screenUV; refrZ = groundZ; }
    float refrThickness = max(refrZ - surfaceZ, 0.0);
    vec3 refracted = texture(uSceneColor, refrUV).rgb;

    // Absorption (Beer-Lambert) and in-scattering toward a teal body colour
    vec3 absorb = vec3(0.55, 0.16, 0.11);
    vec3 transmittance = exp(-absorb * refrThickness * 1.2);
    vec3 bodyColor = vec3(0.01, 0.055, 0.06) * (ambientIrradiance(vec3(0, 1, 0)) + uLightColor.rgb * 0.5);
    vec3 underwater = refracted * transmittance + bodyColor * (1.0 - transmittance);

    // Reflection: sky + sun glint
    vec3 R = reflect(-V, N);
    R.y = abs(R.y);
    vec3 reflection = skyRadiance(R);
    float NdotV = saturate(dot(N, V));
    float fresnel = 0.02 + 0.98 * pow(1.0 - NdotV, 5.0);
    float shadow = shadowVisibility(fragWorldPos, vec3(0, 1, 0), 0.05) * cloudShadow(fragWorldPos, uLightDir.xyz);
    vec3 L = uLightDir.xyz;
    vec3 H = normalize(L + V);
    float a = 0.045;
    float NdotL = saturate(dot(N, L));
    vec3 glint = D_GGX(saturate(dot(N, H)), a * a) * V_SmithGGX(max(NdotV, 1e-3), max(NdotL, 1e-3), a * a) *
                 F_Schlick(vec3(0.02), saturate(dot(V, H))) * PI * NdotL * uLightColor.rgb * shadow;
    glint = min(glint, vec3(60.0));

    // Foam where the water is shallow against the shore
    float foamNoise = fbm(p * 2.2 + uWind.xy * uTime * 0.3, 3);
    float foam = (1.0 - smoothstep(0.0, 0.35 + foamNoise * 0.4, thickness)) * smoothstep(0.35, 0.65, foamNoise + 0.25);
    vec3 foamColor = vec3(0.8) * (ambientIrradiance(vec3(0, 1, 0)) + uLightColor.rgb * saturate(L.y) * shadow);

    vec3 color = mix(underwater, reflection, fresnel) + glint;
    color = mix(color, foamColor, foam * 0.8);
    color += pointLighting(defaultSurface(vec3(0.02), N), fragWorldPos, V, vec3(0.02), 0.05) * 0.5;

    // Soft edge where the water meets the shore and at the rectangle's borders
    vec2 border = min(fragUV, 1.0 - fragUV) * uWaterSize;
    float rectEdge = smoothstep(0.0, 1.2, min(border.x, border.y));
    float edge = smoothstep(0.0, 0.08, thickness) * rectEdge;
    color = mix(refracted, color, edge);

    float T; vec3 inscatter;
    fogTerms(fragWorldPos, T, inscatter);
    outDirect = vec4(color * T + inscatter, 1.0);
    outAmbient = vec4(0.0, 0.0, 0.0, 1.0);
}
