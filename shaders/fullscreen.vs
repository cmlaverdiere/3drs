#version 330

// Fullscreen triangle vertex shader
// Generates a fullscreen triangle from vertex ID (no vertex buffer needed)
// Usage: Draw 3 vertices with no VBO bound

in vec3 vertexPosition;
in vec2 vertexTexCoord;

out vec2 fragTexCoord;

void main() {
    fragTexCoord = vertexTexCoord;
    gl_Position = vec4(vertexPosition, 1.0);
}
