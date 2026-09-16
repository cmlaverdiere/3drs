#version 330

// Separable depth-aware blur for the volumetric buffer (depth from the SSAO buffer)
#include "common/frame.glsl"

in vec2 fragTexCoord;
uniform sampler2D uSource;
uniform sampler2D uAO;
uniform vec2 uDirection;
out vec4 finalColor;

void main() {
    float centerDepth = texture(uAO, fragTexCoord).g;
    vec3 sum = texture(uSource, fragTexCoord).rgb;
    float weights = 1.0;
    float tolerance = 0.08 * min(centerDepth, 400.0) + 0.2;
    for (int i = -5; i <= 5; i++) {
        if (i == 0) continue;
        vec2 uv = fragTexCoord + uDirection * float(i) * 1.5;
        float d = texture(uAO, uv).g;
        float w = exp(-float(i * i) / 14.0) * exp(-abs(min(d, 400.0) - min(centerDepth, 400.0)) / tolerance);
        sum += texture(uSource, uv).rgb * w;
        weights += w;
    }
    finalColor = vec4(sum / weights, 1.0);
}
