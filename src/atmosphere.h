#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

// Single-scattering Rayleigh/Mie/ozone atmosphere. The GPU sky uses the same
// model (shaders/common/atmosphere.glsl); the CPU copy derives the sun colour
// at the ground and the sky's ambient irradiance so lighting matches the sky.

#include "raymath.h"
#include <cmath>

namespace atmosphere {

constexpr float kEarthRadius = 6360e3f;
constexpr float kTopRadius = 6460e3f;
constexpr float kViewerAltitude = 200.0f;
constexpr float kRayleighHeight = 8000.0f;
constexpr float kMieHeight = 1200.0f;
constexpr float kMieScattering = 3.996e-6f;
constexpr float kMieExtinction = 4.44e-6f;
constexpr float kMieG = 0.8f;
constexpr float kPi = 3.14159265f;
const Vector3 kRayleighScattering = {5.802e-6f, 13.558e-6f, 33.1e-6f};
const Vector3 kOzoneAbsorption = {0.650e-6f, 1.881e-6f, 0.085e-6f};

inline Vector3 Mul(Vector3 a, Vector3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
inline Vector3 Exp(Vector3 v) { return {expf(v.x), expf(v.y), expf(v.z)}; }

// Distance to the far intersection with a sphere centred at the planet origin.
inline float RaySphereExit(Vector3 origin, Vector3 dir, float radius) {
    float b = Vector3DotProduct(origin, dir);
    float c = Vector3DotProduct(origin, origin) - radius * radius;
    float h = b * b - c;
    if (h < 0.0f) return -1.0f;
    return -b + sqrtf(h);
}

inline bool RayHitsGround(Vector3 origin, Vector3 dir) {
    float b = Vector3DotProduct(origin, dir);
    float c = Vector3DotProduct(origin, origin) - kEarthRadius * kEarthRadius;
    return b < 0.0f && b * b - c >= 0.0f;
}

inline Vector3 Extinction(float altitude) {
    float rayleigh = expf(-altitude / kRayleighHeight);
    float mie = expf(-altitude / kMieHeight);
    float ozone = fmaxf(0.0f, 1.0f - fabsf(altitude - 25000.0f) / 15000.0f);
    return Vector3Add(Vector3Add(Vector3Scale(kRayleighScattering, rayleigh),
                                 Vector3{kMieExtinction * mie, kMieExtinction * mie, kMieExtinction * mie}),
                      Vector3Scale(kOzoneAbsorption, ozone));
}

// Transmittance from a point to the top of the atmosphere.
inline Vector3 TransmittanceToSpace(Vector3 origin, Vector3 dir, int steps = 12) {
    if (RayHitsGround(origin, dir)) return {0, 0, 0};
    float length = RaySphereExit(origin, dir, kTopRadius);
    float dt = length / steps;
    Vector3 depth = {0, 0, 0};
    for (int i = 0; i < steps; i++) {
        Vector3 p = Vector3Add(origin, Vector3Scale(dir, (i + 0.5f) * dt));
        depth = Vector3Add(depth, Vector3Scale(Extinction(Vector3Length(p) - kEarthRadius), dt));
    }
    return Exp(Vector3Negate(depth));
}

inline float RayleighPhase(float cosTheta) {
    return 3.0f / (16.0f * kPi) * (1.0f + cosTheta * cosTheta);
}

inline float MiePhase(float cosTheta) {
    // Cornette-Shanks, normalised.
    float g = kMieG, g2 = g * g;
    float k = 3.0f / (8.0f * kPi) * (1.0f - g2) / (2.0f + g2);
    return k * (1.0f + cosTheta * cosTheta) / powf(1.0f + g2 - 2.0f * g * cosTheta, 1.5f);
}

inline Vector3 ViewerOrigin() { return {0.0f, kEarthRadius + kViewerAltitude, 0.0f}; }

// Scattered radiance seen along dir for one directional light of the given illuminance.
inline Vector3 SkyRadiance(Vector3 dir, Vector3 lightDir, Vector3 illuminance, int steps = 16) {
    Vector3 origin = ViewerOrigin();
    float length = RaySphereExit(origin, dir, kTopRadius);
    if (RayHitsGround(origin, dir)) {
        float b = Vector3DotProduct(origin, dir);
        float c = Vector3DotProduct(origin, origin) - kEarthRadius * kEarthRadius;
        length = -b - sqrtf(fmaxf(b * b - c, 0.0f));
    }
    length = fminf(length, 400e3f);
    float cosTheta = Vector3DotProduct(dir, lightDir);
    float phaseR = RayleighPhase(cosTheta), phaseM = MiePhase(cosTheta);
    float dt = length / steps;
    Vector3 depth = {0, 0, 0};
    Vector3 sum = {0, 0, 0};
    for (int i = 0; i < steps; i++) {
        Vector3 p = Vector3Add(origin, Vector3Scale(dir, (i + 0.5f) * dt));
        float altitude = Vector3Length(p) - kEarthRadius;
        Vector3 ext = Extinction(altitude);
        Vector3 stepDepth = Vector3Scale(ext, dt * 0.5f);
        depth = Vector3Add(depth, stepDepth);
        Vector3 toLight = TransmittanceToSpace(p, lightDir, 6);
        Vector3 transmittance = Mul(Exp(Vector3Negate(depth)), toLight);
        float rayleigh = expf(-altitude / kRayleighHeight);
        float mie = expf(-altitude / kMieHeight);
        Vector3 scattering = Vector3Add(Vector3Scale(kRayleighScattering, rayleigh * phaseR),
                                        Vector3{kMieScattering * mie * phaseM, kMieScattering * mie * phaseM,
                                                kMieScattering * mie * phaseM});
        sum = Vector3Add(sum, Vector3Scale(Mul(transmittance, scattering), dt));
        depth = Vector3Add(depth, stepDepth);
    }
    return Mul(sum, illuminance);
}

}  // namespace atmosphere

#endif
