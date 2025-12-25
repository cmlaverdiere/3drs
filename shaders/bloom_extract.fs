#version 330

// Bloom bright pass - extract pixels above threshold

in vec2 fragTexCoord;

uniform sampler2D texture0;  // Scene texture
uniform float threshold;     // Brightness threshold (default: 0.8)

out vec4 finalColor;

void main() {
    vec4 color = texture(texture0, fragTexCoord);

    // Calculate luminance
    float luminance = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));

    // Soft threshold with smooth falloff
    float brightness = max(0.0, luminance - threshold);
    float contribution = brightness / (brightness + 1.0);  // Soft knee

    // Output bright pixels only
    finalColor = vec4(color.rgb * contribution, 1.0);
}
