#version 330

// Sky-view LUT: single-scattered radiance for sun and moon by view direction.
#include "common/atmosphere.glsl"

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
    vec3 dir = skyLutDirection(fragTexCoord);
    vec3 L = scatteredRadiance(dir, uSunDir.xyz, uSunColor.rgb, 24);
    if (uMoonDir.w > 0.001) L += scatteredRadiance(dir, uMoonDir.xyz, uMoonColor.rgb, 12);
    // Multiple-scattering lift (single scattering alone is too dark); must
    // match kSkyBoost in lighting.cpp so ambient light agrees with the sky
    float horizon = 1.0 - abs(dir.y);
    L *= 2.4 * (1.0 + 0.3 * horizon * horizon);
    finalColor = vec4(L, 1.0);
}
