// Single-scattering atmosphere, matching src/atmosphere.h.
// The sky-view LUT (sky_lut.fs) stores scattered radiance by direction; every
// other shader samples it for sky colour, fog colour and reflections.

#include "frame.glsl"

uniform sampler2D uSkyLUT;

const float kEarthRadius = 6360e3;
const float kTopRadius = 6460e3;
const float kViewerAltitude = 200.0;
const float kRayleighHeight = 8000.0;
const float kMieHeight = 1200.0;
const float kMieScattering = 3.996e-6;
const float kMieExtinction = 4.44e-6;
const float kMieG = 0.8;
const vec3 kRayleighScattering = vec3(5.802e-6, 13.558e-6, 33.1e-6);
const vec3 kOzoneAbsorption = vec3(0.650e-6, 1.881e-6, 0.085e-6);

float raySphereExit(vec3 origin, vec3 dir, float radius) {
    float b = dot(origin, dir);
    float c = dot(origin, origin) - radius * radius;
    float h = b * b - c;
    return h < 0.0 ? -1.0 : -b + sqrt(h);
}

bool rayHitsGround(vec3 origin, vec3 dir) {
    float b = dot(origin, dir);
    float c = dot(origin, origin) - kEarthRadius * kEarthRadius;
    return b < 0.0 && b * b - c >= 0.0;
}

vec3 atmosphereExtinction(float altitude) {
    float ozone = max(0.0, 1.0 - abs(altitude - 25000.0) / 15000.0);
    return kRayleighScattering * exp(-altitude / kRayleighHeight) +
           vec3(kMieExtinction * exp(-altitude / kMieHeight)) + kOzoneAbsorption * ozone;
}

vec3 transmittanceToSpace(vec3 origin, vec3 dir, int steps) {
    if (rayHitsGround(origin, dir)) return vec3(0.0);
    float len = raySphereExit(origin, dir, kTopRadius);
    float dt = len / float(steps);
    vec3 depth = vec3(0.0);
    for (int i = 0; i < steps; i++) {
        vec3 p = origin + dir * ((float(i) + 0.5) * dt);
        depth += atmosphereExtinction(length(p) - kEarthRadius) * dt;
    }
    return exp(-depth);
}

float rayleighPhase(float c) { return 3.0 / (16.0 * PI) * (1.0 + c * c); }

float miePhase(float c) {
    float g = kMieG, g2 = g * g;
    float k = 3.0 / (8.0 * PI) * (1.0 - g2) / (2.0 + g2);
    return k * (1.0 + c * c) / pow(1.0 + g2 - 2.0 * g * c, 1.5);
}

// Henyey-Greenstein, used for fog and cloud forward scattering
float hgPhase(float c, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * c, 1.5));
}

vec3 atmosphereViewer() { return vec3(0.0, kEarthRadius + kViewerAltitude, 0.0); }

vec3 scatteredRadiance(vec3 dir, vec3 lightDir, vec3 illuminance, int steps) {
    vec3 origin = atmosphereViewer();
    float len = raySphereExit(origin, dir, kTopRadius);
    if (rayHitsGround(origin, dir)) {
        float b = dot(origin, dir);
        float c = dot(origin, origin) - kEarthRadius * kEarthRadius;
        len = -b - sqrt(max(b * b - c, 0.0));
    }
    len = min(len, 400e3);
    float cosTheta = dot(dir, lightDir);
    float phaseR = rayleighPhase(cosTheta), phaseM = miePhase(cosTheta);
    float dt = len / float(steps);
    vec3 depth = vec3(0.0);
    vec3 sum = vec3(0.0);
    for (int i = 0; i < steps; i++) {
        vec3 p = origin + dir * ((float(i) + 0.5) * dt);
        float altitude = length(p) - kEarthRadius;
        vec3 halfDepth = atmosphereExtinction(altitude) * dt * 0.5;
        depth += halfDepth;
        vec3 T = exp(-depth) * transmittanceToSpace(p, lightDir, 6);
        vec3 scattering = kRayleighScattering * exp(-altitude / kRayleighHeight) * phaseR +
                          vec3(kMieScattering * exp(-altitude / kMieHeight) * phaseM);
        sum += T * scattering * dt;
        depth += halfDepth;
    }
    return sum * illuminance;
}

// Sky-view LUT parameterisation: azimuth in u, sqrt-compressed elevation in v
vec2 skyLutUV(vec3 dir) {
    float azimuth = atan(dir.z, dir.x);
    float u = azimuth / (2.0 * PI) + 0.5;
    float el = asin(clamp(dir.y, -1.0, 1.0));
    float v = 0.5 + 0.5 * sign(el) * sqrt(abs(el) / (0.5 * PI));
    return vec2(u, v);
}

vec3 skyLutDirection(vec2 uv) {
    float azimuth = (uv.x - 0.5) * 2.0 * PI;
    float s = uv.y * 2.0 - 1.0;
    float el = sign(s) * s * s * 0.5 * PI;
    return vec3(cos(el) * cos(azimuth), sin(el), cos(el) * sin(azimuth));
}

// Sky radiance behind a direction (below the horizon we see hazy horizon)
vec3 skyRadiance(vec3 dir) {
    dir.y = max(dir.y, 0.004);
    return texture(uSkyLUT, skyLutUV(normalize(dir))).rgb;
}

// Radiance of the sun disc (limb darkened) including atmospheric extinction
vec3 sunDisc(vec3 dir, vec3 sunDir, vec3 transmittance) {
    const float cosRadius = 0.99996;   // ~0.5 degree disc
    float c = dot(dir, sunDir);
    if (c < cosRadius - 0.00002) return vec3(0.0);
    float edge = smoothstep(cosRadius - 0.00002, cosRadius + 0.000005, c);
    float r = sqrt(max(0.0, 1.0 - (c - cosRadius) / (1.0 - cosRadius)));
    float limb = 1.0 - 0.6 * (1.0 - sqrt(max(0.0, 1.0 - r * r)));
    return transmittance * uSunColor.rgb * 60.0 * limb * edge;
}
