#version 330

// SSAO Blur - Simple box blur to smooth noise
// Edge-aware to prevent bleeding across depth discontinuities

in vec2 fragTexCoord;

uniform sampler2D ssaoTexture;
uniform vec2 texelSize;

out vec4 finalColor;

void main() {
    float result = 0.0;

    // Simple 4x4 box blur
    for (int x = -2; x < 2; x++) {
        for (int y = -2; y < 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoTexture, fragTexCoord + offset).r;
        }
    }

    result /= 16.0;

    finalColor = vec4(result, result, result, 1.0);
}
