#version 330

// Weathered vertical planks: grain, knots, dark gaps and iron nails
#include "common/walls.glsl"

void main() {
    vec3 N = normalize(fragNormal);
    vec2 uv = wallUV(N, fragWorldPos);
    const float plankW = 0.24;
    float px = uv.x / plankW;
    float plank = floor(px);
    float id = hash(vec2(plank, 1.7));
    float local = fract(px);
    // Plank boards are cut into lengths with staggered joints
    float boardLen = 1.6 + id * 1.2;
    float vy = uv.y / boardLen + id * 3.0;
    float board = floor(vy);
    float bid = hash(vec2(plank, board));
    float gapX = 1.0 - smoothstep(0.0, 0.035, min(local, 1.0 - local));
    float gapY = 1.0 - smoothstep(0.0, 0.01, min(fract(vy), 1.0 - fract(vy)) * boardLen);
    float gap = max(gapX, gapY);

    float grain = fbm(vec2(uv.x * 26.0 + bid * 9.0, uv.y * 1.8), 4);
    float rings = sin((uv.x * 60.0 + grain * 9.0 + bid * 20.0)) * 0.5 + 0.5;
    vec2 knotC = vec2(plank + 0.5, (board + 0.3 + bid * 0.4) * boardLen);
    float knot = 1.0 - smoothstep(0.02, 0.06, length((vec2(px, uv.y) - knotC) * vec2(plankW, 1.0)));
    knot *= step(0.6, bid);

    vec3 wood = mix(vec3(0.19, 0.11, 0.055), vec3(0.32, 0.2, 0.1), id * 0.6 + rings * 0.25 + grain * 0.3);
    wood = mix(wood, vec3(0.22, 0.21, 0.19), smoothstep(0.4, 0.9, fbm(uv * 3.0 + 5.0, 3)) * 0.55); // silvered
    wood *= 1.0 - knot * 0.55;
    // Nails near board ends
    vec2 nailC = vec2(plank + 0.5, (board + 0.06) * boardLen);
    float nail = 1.0 - smoothstep(0.006, 0.012, length((vec2(px, uv.y) - nailC) * vec2(plankW, 1.0)));
    vec3 albedo = mix(wood, vec3(0.03, 0.025, 0.02), gap);
    albedo = mix(albedo, vec3(0.08, 0.07, 0.065), nail);

    float rough;
    albedo = weatherWall(albedo, N, fragWorldPos, rough);
    float height = (1.0 - gap) * 0.006 + grain * 0.002 + rings * 0.0008 - knot * 0.001;
    vec3 n = bumpNormal(N, fragWorldPos, height, 1.0);

    Surface s = defaultSurface(albedo, n);
    s.roughness = mix(0.8, 0.45, nail) + rough;
    s.metallic = nail * 0.8;
    s.occlusion = mix(1.0, 0.35, gap);
    writeSurface(s, fragWorldPos, N, 1.0);
}
