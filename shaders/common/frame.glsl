// Per-frame uniforms shared by every shader (std140 mirror of FrameUniforms in lighting.h)
layout(std140, row_major) uniform FrameData {  // raylib Matrix is row-major in memory
    mat4 uView;
    mat4 uProj;
    mat4 uViewProj;
    mat4 uInvViewProj;
    vec4 uCamera;          // xyz position, w time (s)
    vec4 uLightDir;        // toward key light (sun by day, moon by night), w = moonlit
    vec4 uLightColor;      // key light irradiance / pi at the ground
    vec4 uSunDir;          // toward sun, w = disc visibility
    vec4 uSunColor;        // sun illuminance above the atmosphere
    vec4 uMoonDir;         // toward moon, w = visibility
    vec4 uMoonColor;       // moon illuminance above the atmosphere
    vec4 uAmbientSH[4];    // L1 SH sky irradiance / pi
    vec4 uFog;             // density, height falloff, base height, strength
    vec4 uClouds;          // coverage, altitude, wind offset xz
    vec4 uWind;            // direction xz, strength, gust
    vec4 uSeason;          // season id, snow cover, time of day, night factor
    mat4 uShadowMatrix[4];
    vec4 uShadowSplits;
    vec4 uShadowTexel;
    vec4 uShadowParams;    // enabled, cascade count, fade start, fade end
    vec4 uPointPos[16];    // xyz, radius
    vec4 uPointColor[16];
    vec4 uCounts;          // point light count
    vec4 uScreen;          // size, 1/size
    vec4 uExposure;        // exposure, night factor
};

#define PI 3.14159265
#define uTime (uCamera.w)
#define uSeasonId (int(uSeason.x + 0.5))

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash13(vec3 p3) {
    p3 = fract(p3 * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

// Value noise with quintic fade
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float vnoise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);
    float n000 = hash13(i), n100 = hash13(i + vec3(1, 0, 0));
    float n010 = hash13(i + vec3(0, 1, 0)), n110 = hash13(i + vec3(1, 1, 0));
    float n001 = hash13(i + vec3(0, 0, 1)), n101 = hash13(i + vec3(1, 0, 1));
    float n011 = hash13(i + vec3(0, 1, 1)), n111 = hash13(i + vec3(1, 1, 1));
    return mix(mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
               mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y), u.z);
}

// Fractal noise (rotated octaves avoid axis-aligned artifacts)
float fbm(vec2 p, int octaves) {
    const mat2 rot = mat2(0.8, 0.6, -0.6, 0.8);
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p);
        p = rot * p * 2.03 + 17.1;
        amplitude *= 0.5;
    }
    return value;
}

float fbm3(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * vnoise3(p);
        p = p * 2.02 + vec3(11.3, 7.1, 3.7);
        amplitude *= 0.5;
    }
    return value;
}

// Voronoi: x = distance to nearest cell point, y = distance to border, zw = cell id
vec4 voronoiCell(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 bestCell = vec2(0.0);
    vec2 bestOffset = vec2(0.0);
    float best = 8.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 g = vec2(float(x), float(y));
            vec2 o = hash22(i + g);
            vec2 r = g + o - f;
            float d = dot(r, r);
            if (d < best) { best = d; bestOffset = r; bestCell = i + g; }
        }
    }
    float border = 8.0;
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            vec2 g = vec2(float(x), float(y));
            vec2 o = hash22(i + g);
            vec2 r = g + o - f;
            if (dot(r - bestOffset, r - bestOffset) > 1e-5) {
                border = min(border, dot(0.5 * (bestOffset + r), normalize(r - bestOffset)));
            }
        }
    }
    return vec4(sqrt(best), border, bestCell);
}

vec3 srgbToLinear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

float luminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// Interleaved gradient noise (per-pixel dither)
float ign(vec2 pixel) {
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate(vec3 x) { return clamp(x, 0.0, 1.0); }
