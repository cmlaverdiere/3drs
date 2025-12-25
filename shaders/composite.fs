#version 330

// Composite shader - combine scene with bloom

in vec2 fragTexCoord;

uniform sampler2D texture0;    // Scene texture
uniform sampler2D bloomTexture; // Blurred bloom
uniform float bloomIntensity;   // Bloom strength (default: 1.0)

out vec4 finalColor;

void main() {
    vec3 scene = texture(texture0, fragTexCoord).rgb;
    vec3 bloom = texture(bloomTexture, fragTexCoord).rgb;

    // DEBUG: Just output scene, no bloom
    finalColor = vec4(scene, 1.0);
}
