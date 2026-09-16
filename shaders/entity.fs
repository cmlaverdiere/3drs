#version 330

// Characters, props and items built from primitives. Cubes get rounded-edge
// shading (BEVEL) so the blocky figures read as crafted, softly lit objects.
in vec3 fragWorldPos;
in vec3 fragNormal;
in vec3 fragLocalPos;
#ifdef BEVEL
in vec3 fragBoxPos;
flat in vec3 fragHalfExtent;
flat in mat3 fragBoxFrame;
#endif

uniform vec4 colDiffuse;
uniform float uEmissive;   // > 0 for glowing parts (flames, lamp glass, eyes)

#include "common/lighting.glsl"

void main() {
    vec3 albedo = srgbToLinear(colDiffuse.rgb);
    vec3 geomN = normalize(fragNormal);
    vec3 N = geomN;
    float edgeWear = 0.0;
#ifdef BEVEL
    float minHalf = min(fragHalfExtent.x, min(fragHalfExtent.y, fragHalfExtent.z));
    float r = min(0.045, minHalf * 0.45);
    vec3 inner = max(fragHalfExtent - r, vec3(0.0));
    vec3 q = clamp(fragBoxPos, -inner, inner);
    vec3 nb = fragBoxPos - q;
    float len = length(nb);
    if (len > 1e-5) {
        N = normalize(fragBoxFrame * (nb / len));
        if (dot(N, geomN) < 0.2) N = geomN;
    }
    // Edges catch a little light and wear
    vec3 toEdge = fragHalfExtent - abs(fragBoxPos);
    float sorted = min(max(toEdge.x, toEdge.y), min(max(toEdge.y, toEdge.z), max(toEdge.x, toEdge.z)));
    edgeWear = 1.0 - smoothstep(0.0, r * 1.2, sorted);
#endif
    // Subtle material grain so flat colours are not plastic
    float grain = fbm3(fragWorldPos * 7.0, 3);
    albedo *= 0.9 + 0.2 * grain;
    albedo = mix(albedo, albedo * 1.25 + 0.01, edgeWear * 0.5);

    Surface s = defaultSurface(albedo, N);
    s.roughness = 0.62 - edgeWear * 0.15;
    s.occlusion = 0.92 + 0.08 * edgeWear;
    s.shadowSoftness = 0.02;
    s.emissive = albedo * uEmissive;
    writeSurface(s, fragWorldPos, geomN, colDiffuse.a);
}
