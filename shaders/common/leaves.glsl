// Leaf-cluster mask for canopy cards: a spray of small pointed leaves.
// uv in [-1, 1]; returns > 0 inside a leaf, vein ~0 along leaf midribs.
float leafMask(vec2 uv, float seed, out float vein) {
    float best = -1.0;
    vein = 1.0;
    for (int i = 0; i < 9; i++) {
        float fi = float(i);
        float a = fi * 2.39996 + seed * 6.283;
        float ring = i == 0 ? 0.0 : (i < 4 ? 0.33 : 0.62);
        vec2 dir = vec2(cos(a), sin(a));
        vec2 c = dir * ring * (0.85 + 0.3 * fract(seed * 7.31 + fi * 0.37));
        // Leaves point outward from the cluster centre, with some twist
        float twist = (fract(seed * 3.7 + fi * 0.61) - 0.5) * 1.2;
        vec2 ld = i == 0 ? vec2(cos(seed * 9.0), sin(seed * 9.0)) : vec2(cos(a + twist), sin(a + twist));
        vec2 p = uv - c;
        vec2 q = vec2(dot(p, ld), dot(p, vec2(-ld.y, ld.x)));
        float len = 0.27 + 0.08 * fract(seed * 5.1 + fi * 0.73);
        float along = q.x / len;
        float width = len * 0.42 * (1.0 - along * along) * (1.0 - 0.25 * along);
        float d = abs(along) > 1.0 ? -1.0 : width - abs(q.y);
        if (d > best) {
            best = d;
            vein = smoothstep(0.0, 0.012, abs(q.y)) * 0.5 + 0.5 * smoothstep(-0.9, 0.2, along);
        }
    }
    return best;
}
