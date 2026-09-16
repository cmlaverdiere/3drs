#version 330

// Fullscreen triangle from gl_VertexID; z = 1 puts it on the far plane so the
// sky can be depth-tested against the opaque scene.
out vec2 fragTexCoord;

void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    fragTexCoord = p;
    gl_Position = vec4(p * 2.0 - 1.0, 1.0, 1.0);
}
