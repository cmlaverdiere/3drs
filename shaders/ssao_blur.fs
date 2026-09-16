#version 330

// Separable depth-aware blur of (ao, linear depth)
#include "common/frame.glsl"

in vec2 fragTexCoord;
uniform sampler2D uSource;
uniform vec2 uDirection;
out vec4 finalColor;

void main() {
    vec2 center = texture(uSource, fragTexCoord).rg;
    float sum = center.r;
    float weights = 1.0;
    float tolerance = 0.04 * center.g + 0.05;
    for (int i = -4; i <= 4; i++) {
        if (i == 0) continue;
        vec2 s = texture(uSource, fragTexCoord + uDirection * float(i)).rg;
        float w = exp(-float(i * i) / 10.0) * exp(-abs(s.g - center.g) / tolerance);
        sum += s.r * w;
        weights += w;
    }
    finalColor = vec4(sum / weights, center.g, 0.0, 1.0);
}
