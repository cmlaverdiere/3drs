#version 330

// Separable Gaussian blur for bloom
// Run twice: once horizontal (direction = vec2(1,0)), once vertical (direction = vec2(0,1))

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 direction;      // (1,0) for horizontal, (0,1) for vertical
uniform vec2 texelSize;      // 1.0 / textureSize

out vec4 finalColor;

// 9-tap Gaussian weights (sigma ~= 2.5)
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec3 result = texture(texture0, fragTexCoord).rgb * weights[0];

    vec2 offset = direction * texelSize;

    // Sample in both directions
    for (int i = 1; i < 5; i++) {
        vec2 sampleOffset = offset * float(i);
        result += texture(texture0, fragTexCoord + sampleOffset).rgb * weights[i];
        result += texture(texture0, fragTexCoord - sampleOffset).rgb * weights[i];
    }

    finalColor = vec4(result, 1.0);
}
