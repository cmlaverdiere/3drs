#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;

uniform vec4 colDiffuse;  // Material color from raylib
uniform vec3 viewPos;
uniform float stretch;    // How stretched the droplet is (based on velocity)

out vec4 finalColor;

// Procedural blood droplet shape using signed distance functions
float dropletShape(vec2 uv, float stretchFactor) {
    // Center UV
    vec2 p = uv - vec2(0.5);

    // Apply stretch (elongate in Y direction based on velocity)
    float s = 1.0 + stretchFactor * 0.5;
    p.y *= s;

    // Teardrop shape: circle with pointed tail
    float r = length(p);

    // Main body - ellipse
    float body = length(p * vec2(1.0, 0.8)) - 0.35;

    // Add pointed tail at bottom (direction of motion)
    float tail = p.y + 0.3;  // Distance from tail point
    float tailWidth = max(0.0, -p.y - 0.1) * 0.8;  // Narrow toward tail
    float tailShape = abs(p.x) - tailWidth;

    // Combine body and tail
    float d = body;
    if (p.y < -0.1) {
        d = min(d, max(tailShape, tail));
    }

    // Round the top more
    float topRound = length(p - vec2(0.0, 0.15)) - 0.32;
    d = min(d, topRound);

    return d;
}

// Inner highlight for wet/glossy look
float highlight(vec2 uv) {
    vec2 p = uv - vec2(0.45, 0.6);  // Offset toward top-left
    return length(p) - 0.12;
}

// Secondary smaller highlight
float highlight2(vec2 uv) {
    vec2 p = uv - vec2(0.55, 0.55);
    return length(p) - 0.06;
}

void main() {
    // Get droplet shape
    float d = dropletShape(fragTexCoord, stretch);

    // Sharp cutout
    if (d > 0.0) {
        discard;
    }

    // Base blood color from material
    vec3 baseColor = colDiffuse.rgb;

    // Darker edges for depth (subsurface scattering approximation)
    float edge = smoothstep(-0.01, -0.12, d);
    vec3 color = baseColor * (0.6 + 0.4 * edge);

    // Inner darkening toward center-bottom (thickness)
    vec2 centerP = fragTexCoord - vec2(0.5, 0.4);
    float thickness = 1.0 - smoothstep(0.0, 0.35, length(centerP));
    color *= (0.85 + 0.15 * (1.0 - thickness));

    // Wet highlight (specular-like)
    float h1 = highlight(fragTexCoord);
    if (h1 < 0.0) {
        float highlightStrength = smoothstep(0.0, -0.08, h1);
        color = mix(color, vec3(1.0, 0.9, 0.9), highlightStrength * 0.6);
    }

    // Second smaller highlight
    float h2 = highlight2(fragTexCoord);
    if (h2 < 0.0) {
        float highlightStrength = smoothstep(0.0, -0.04, h2);
        color = mix(color, vec3(1.0, 0.95, 0.95), highlightStrength * 0.4);
    }

    // Slight color variation across the droplet
    float variation = sin(fragTexCoord.x * 15.0) * sin(fragTexCoord.y * 12.0) * 0.03;
    color.r += variation;

    // Rim darkening (blood is darker at very edge)
    float rim = smoothstep(-0.02, 0.0, d);
    color *= (1.0 - rim * 0.3);

    // Alpha - slightly translucent with solid core
    float alpha = colDiffuse.a;
    // More opaque in center
    alpha *= (0.85 + 0.15 * edge);

    finalColor = vec4(color, alpha);
}
