#version 330

// Running-bond brick with recessed mortar, per-brick tone and chipped edges
#include "common/walls.glsl"

void main() {
    vec3 N = normalize(fragNormal);
    vec2 uv = wallUV(N, fragWorldPos);
    const vec2 size = vec2(0.42, 0.18);
    const float mortar = 0.018;
    vec2 b = uv / size;
    float row = floor(b.y);
    b.x += mod(row, 2.0) * 0.5;
    vec2 cell = floor(b);
    vec2 f = (fract(b) - 0.5) * size;           // metres from brick centre
    vec2 halfSize = size * 0.5 - mortar * 0.5;
    float chip = (noise(uv * 38.0) - 0.5) * 0.008;
    vec2 d = abs(f) - halfSize + chip;
    float edgeDist = -max(d.x, d.y);            // >0 inside brick
    float brickMask = smoothstep(0.0, 0.004, edgeDist);
    float id = hash(cell);

    vec3 tones[4] = vec3[](vec3(0.33, 0.085, 0.045), vec3(0.42, 0.12, 0.06),
                           vec3(0.27, 0.07, 0.04), vec3(0.46, 0.17, 0.09));
    vec3 brick = tones[int(id * 3.99)];
    brick *= 0.8 + 0.4 * fbm(uv * 11.0 + id * 31.0, 3);
    brick = mix(brick, vec3(0.12, 0.09, 0.07), step(0.93, id) * 0.6);   // over-fired bricks
    vec3 mortarColor = vec3(0.30, 0.28, 0.25) * (0.8 + 0.3 * noise(uv * 60.0));
    vec3 albedo = mix(mortarColor, brick, brickMask);

    float rough;
    albedo = weatherWall(albedo, N, fragWorldPos, rough);
    // Rounded brick faces in the height field; mortar sits 8mm back
    float height = brickMask * (0.008 + smoothstep(0.0, 0.02, edgeDist) * 0.004) + noise(uv * 25.0) * 0.0015;
    vec3 n = bumpNormal(N, fragWorldPos, height, 1.0);

    Surface s = defaultSurface(albedo, n);
    s.roughness = mix(0.95, 0.78, brickMask) + rough;
    s.occlusion = mix(0.55, 1.0, brickMask);
    writeSurface(s, fragWorldPos, N, 1.0);
}
