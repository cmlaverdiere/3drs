#version 330

// FXAA 3.11-style edge anti-aliasing (luma in alpha)
in vec2 fragTexCoord;
uniform sampler2D uSource;
uniform vec2 uTexel;
out vec4 finalColor;

#define EDGE_MIN 0.0312
#define EDGE_MAX 0.125
#define ITERATIONS 12
#define SUBPIXEL 0.7

float quality(int i) {
    return i < 5 ? 1.0 : (i == 5 ? 1.5 : (i < 10 ? 2.0 : (i == 10 ? 4.0 : 8.0)));
}

float lumaAt(vec2 uv) { return textureLod(uSource, uv, 0.0).a; }

void main() {
    vec2 uv = fragTexCoord;
    vec2 inv = uTexel;
    vec4 center = textureLod(uSource, uv, 0.0);
    float lumaC = center.a;
    float lumaD = lumaAt(uv + vec2(0, -inv.y));
    float lumaU = lumaAt(uv + vec2(0, inv.y));
    float lumaL = lumaAt(uv + vec2(-inv.x, 0));
    float lumaR = lumaAt(uv + vec2(inv.x, 0));
    float lumaMin = min(lumaC, min(min(lumaD, lumaU), min(lumaL, lumaR)));
    float lumaMax = max(lumaC, max(max(lumaD, lumaU), max(lumaL, lumaR)));
    float range = lumaMax - lumaMin;
    if (range < max(EDGE_MIN, lumaMax * EDGE_MAX)) {
        finalColor = vec4(center.rgb, 1.0);
        return;
    }
    float lumaDL = lumaAt(uv + vec2(-inv.x, -inv.y));
    float lumaUR = lumaAt(uv + vec2(inv.x, inv.y));
    float lumaUL = lumaAt(uv + vec2(-inv.x, inv.y));
    float lumaDR = lumaAt(uv + vec2(inv.x, -inv.y));
    float lumaDU = lumaD + lumaU;
    float lumaLR = lumaL + lumaR;
    float leftCorners = lumaDL + lumaUL;
    float downCorners = lumaDL + lumaDR;
    float rightCorners = lumaDR + lumaUR;
    float upCorners = lumaUR + lumaUL;
    float edgeH = abs(-2.0 * lumaL + leftCorners) + abs(-2.0 * lumaC + lumaDU) * 2.0 + abs(-2.0 * lumaR + rightCorners);
    float edgeV = abs(-2.0 * lumaU + upCorners) + abs(-2.0 * lumaC + lumaLR) * 2.0 + abs(-2.0 * lumaD + downCorners);
    bool horizontal = edgeH >= edgeV;
    float luma1 = horizontal ? lumaD : lumaL;
    float luma2 = horizontal ? lumaU : lumaR;
    float gradient1 = luma1 - lumaC;
    float gradient2 = luma2 - lumaC;
    bool steep1 = abs(gradient1) >= abs(gradient2);
    float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));
    float stepLength = horizontal ? inv.y : inv.x;
    float localAverage;
    if (steep1) {
        stepLength = -stepLength;
        localAverage = 0.5 * (luma1 + lumaC);
    } else {
        localAverage = 0.5 * (luma2 + lumaC);
    }
    vec2 current = uv;
    if (horizontal) current.y += stepLength * 0.5; else current.x += stepLength * 0.5;
    vec2 offset = horizontal ? vec2(inv.x, 0.0) : vec2(0.0, inv.y);
    vec2 uv1 = current - offset;
    vec2 uv2 = current + offset;
    float end1 = lumaAt(uv1) - localAverage;
    float end2 = lumaAt(uv2) - localAverage;
    bool reached1 = abs(end1) >= gradientScaled;
    bool reached2 = abs(end2) >= gradientScaled;
    if (!reached1) uv1 -= offset;
    if (!reached2) uv2 += offset;
    if (!(reached1 && reached2)) {
        for (int i = 2; i < ITERATIONS; i++) {
            if (!reached1) end1 = lumaAt(uv1) - localAverage;
            if (!reached2) end2 = lumaAt(uv2) - localAverage;
            reached1 = abs(end1) >= gradientScaled;
            reached2 = abs(end2) >= gradientScaled;
            if (!reached1) uv1 -= offset * quality(i);
            if (!reached2) uv2 += offset * quality(i);
            if (reached1 && reached2) break;
        }
    }
    float dist1 = horizontal ? (uv.x - uv1.x) : (uv.y - uv1.y);
    float dist2 = horizontal ? (uv2.x - uv.x) : (uv2.y - uv.y);
    bool direction1 = dist1 < dist2;
    float distFinal = min(dist1, dist2);
    float thickness = dist1 + dist2;
    float pixelOffset = -distFinal / thickness + 0.5;
    bool centerSmaller = lumaC < localAverage;
    bool correct = ((direction1 ? end1 : end2) < 0.0) != centerSmaller;
    float finalOffset = correct ? pixelOffset : 0.0;
    float average = (1.0 / 12.0) * (2.0 * (lumaDU + lumaLR) + leftCorners + rightCorners);
    float sub1 = clamp(abs(average - lumaC) / range, 0.0, 1.0);
    float sub2 = (-2.0 * sub1 + 3.0) * sub1 * sub1;
    finalOffset = max(finalOffset, sub2 * sub2 * SUBPIXEL);
    vec2 finalUv = uv;
    if (horizontal) finalUv.y += finalOffset * stepLength; else finalUv.x += finalOffset * stepLength;
    finalColor = vec4(textureLod(uSource, finalUv, 0.0).rgb, 1.0);
}
