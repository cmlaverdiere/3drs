#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform sampler2D depthTexture;
uniform mat4 inverseProjection;
uniform vec2 texelSize;
out vec4 finalColor;

float viewDepth(vec2 uv, float depth) {
    vec4 p = inverseProjection * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    return p.z / p.w;
}

void main() {
    float depth = texture(depthTexture, fragTexCoord).r;
    if (depth >= 0.9999999) {
        finalColor = vec4(1.0);
        return;
    }
    float center = viewDepth(fragTexCoord, depth);
    float result = 0.0;
    float totalWeight = 0.0;
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 uv = clamp(fragTexCoord + vec2(x, y) * texelSize, vec2(0), vec2(1));
            float sampleDepth = texture(depthTexture, uv).r;
            if (sampleDepth >= 0.9999999) continue;
            float dz = abs(center - viewDepth(uv, sampleDepth));
            float weight = exp(-float(x*x + y*y) / 8.0) * exp(-dz * 20.0);
            result += texture(texture0, uv).r * weight;
            totalWeight += weight;
        }
    }
    finalColor = vec4(vec3(result / max(totalWeight, 0.0001)), 1.0);
}
