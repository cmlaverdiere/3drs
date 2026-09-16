#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform sampler2D bloomTexture;
uniform sampler2D aoTexture;
uniform float bloomIntensity;
uniform int bloomEnabled;
uniform int ssaoEnabled;
out vec4 finalColor;

void main() {
    vec3 scene = texture(texture0, fragTexCoord).rgb;
    float ao = ssaoEnabled != 0 ? texture(aoTexture, fragTexCoord).r : 1.0;
    vec3 bloom = bloomEnabled != 0 ? texture(bloomTexture, fragTexCoord).rgb * bloomIntensity : vec3(0.0);
    // Screen blending preserves LDR highlight detail and avoids additive clipping.
    scene *= ao;
    finalColor = vec4(1.0 - (1.0 - scene) * (1.0 - clamp(bloom, 0.0, 1.0)), 1.0);
}
