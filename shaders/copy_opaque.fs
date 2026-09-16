#version 330

// Opaque scene snapshot (direct + ambient) for water refraction
in vec2 fragTexCoord;
uniform sampler2D uDirect;
uniform sampler2D uAmbient;
out vec4 finalColor;

void main() {
    finalColor = vec4(texture(uDirect, fragTexCoord).rgb + texture(uAmbient, fragTexCoord).rgb, 1.0);
}
