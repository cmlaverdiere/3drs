#version 330

out vec4 fragColor;

void main() {
    // Write depth to color channel (for shadow map sampling)
    float depth = gl_FragCoord.z;
    fragColor = vec4(depth, depth, depth, 1.0);
}
