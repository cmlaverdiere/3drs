#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;

uniform vec4 colDiffuse;  // Material color from raylib
uniform vec3 viewPos;
uniform float stretch;    // How stretched the droplet is (based on velocity)

out vec4 finalColor;

void main() {
    // Simple circular splat shape
    vec2 p = fragTexCoord - vec2(0.5);

    // Slight vertical stretch for droplet feel
    p.y *= (1.0 + stretch * 0.3);

    float dist = length(p);

    // Soft circular edge
    if (dist > 0.4) {
        discard;
    }

    // Base blood color from material
    vec3 color = colDiffuse.rgb;

    // Darker at edges, lighter in center (simple depth)
    float edgeDark = smoothstep(0.15, 0.4, dist);
    color *= (1.0 - edgeDark * 0.4);

    // Shine highlight (small, off-center)
    vec2 highlightPos = fragTexCoord - vec2(0.35, 0.35);
    float highlight = 1.0 - smoothstep(0.0, 0.15, length(highlightPos));
    color = mix(color, vec3(1.0, 0.85, 0.85), highlight * 0.55);

    // Soft alpha falloff at edges
    float alpha = colDiffuse.a * smoothstep(0.4, 0.25, dist);

    finalColor = vec4(color, alpha);
}
