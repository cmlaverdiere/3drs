#version 330

out vec4 fragColor;

void main() {
    // The framebuffer depth attachment stores the hardware depth, at full precision.
    fragColor = vec4(1.0);
}
