#version 330

// Castle ashlar: courses of irregular dressed blocks with lichen and chipped arrises
#include "common/walls.glsl"

void main() {
    vec3 N = normalize(fragNormal);
    vec2 uv = wallUV(N, fragWorldPos);
    // Courses of varying height
    float course = floor(uv.y / 0.55);
    float courseH = 0.55;
    float y = uv.y - course * courseH;
    float offset = hash(vec2(course, 3.1)) * 2.0;
    float blockW = 0.7 + hash(vec2(course, 7.7)) * 0.5;
    float bx = (uv.x + offset) / blockW;
    float block = floor(bx);
    float jitter = (hash(vec2(block, course)) - 0.5) * 0.25;
    bx = (uv.x + offset) / blockW + jitter * step(0.5, fract(bx));
    vec2 f = vec2(fract(bx) - 0.5, y / courseH - 0.5) * vec2(blockW, courseH);
    vec2 halfSize = vec2(blockW, courseH) * 0.5 - 0.02;
    float chip = (fbm(uv * 14.0, 2) - 0.5) * 0.03;
    vec2 d = abs(f) - halfSize + chip;
    float edgeDist = -max(d.x, d.y);
    float mask = smoothstep(0.0, 0.01, edgeDist);
    vec2 id2 = vec2(floor(bx), course);
    float id = hash(id2);

    vec3 stone = mix(vec3(0.26, 0.25, 0.23), vec3(0.42, 0.41, 0.38), id);
    stone *= 0.75 + 0.45 * fbm(uv * 4.0 + id * 17.0, 4);
    float speck = noise(uv * 90.0);
    stone *= 0.92 + 0.12 * speck;
    float lichen = smoothstep(0.62, 0.78, fbm(uv * 2.3 + 40.0, 4));
    stone = mix(stone, vec3(0.30, 0.31, 0.16), lichen * 0.45);
    vec3 mortarColor = vec3(0.17, 0.16, 0.14);
    vec3 albedo = mix(mortarColor, stone, mask);

    float rough;
    albedo = weatherWall(albedo, N, fragWorldPos, rough);
    float dome = smoothstep(0.0, 0.06, edgeDist);
    float height = mask * (0.01 + dome * 0.012) + fbm(uv * 9.0, 3) * 0.006 * mask;
    vec3 n = bumpNormal(N, fragWorldPos, height, 1.0);

    Surface s = defaultSurface(albedo, n);
    s.roughness = 0.88 + rough;
    s.occlusion = mix(0.5, 1.0, mask);
    writeSurface(s, fragWorldPos, N, 1.0);
}
