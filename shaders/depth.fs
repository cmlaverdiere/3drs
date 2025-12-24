#version 330

out vec4 fragColor;

void main() {
    // Depth is written automatically, but we need some output for the shader to compile
    fragColor = vec4(1.0);
}
