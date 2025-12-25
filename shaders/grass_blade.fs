#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;
in float bladeHeight;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform vec3 fogColor;
uniform float fogDensity;
uniform vec3 viewPos;
uniform int winterMode;

out vec4 finalColor;

void main() {
    // Grass/snow blade color gradient - darker at base, lighter at tip
    vec3 baseColor, tipColor;
    float variation = fract(sin(dot(fragWorldPos.xz, vec2(12.9898, 78.233))) * 43758.5453);

    if (winterMode == 1) {
        // Frost-covered grass blades with more variation
        // Some blades more blue (frozen), some more white (snow-covered)
        vec3 frozenBase = vec3(0.70, 0.78, 0.88);   // Icy blue at base
        vec3 snowyTip = vec3(0.92, 0.94, 0.98);     // Snow white at tip

        // Vary between frozen and snowy based on position
        float frostAmount = fract(sin(dot(fragWorldPos.xz, vec2(43.12, 17.89))) * 12345.67);

        if (frostAmount > 0.7) {
            // More frozen/icy blade
            baseColor = vec3(0.65, 0.75, 0.88);
            tipColor = vec3(0.80, 0.88, 0.95);
        } else if (frostAmount > 0.3) {
            // Normal snow-covered
            baseColor = frozenBase;
            tipColor = snowyTip;
        } else {
            // Heavily snow-laden (whiter)
            baseColor = vec3(0.82, 0.85, 0.90);
            tipColor = vec3(0.96, 0.97, 1.0);
        }
    } else {
        baseColor = vec3(0.08, 0.35, 0.06);
        tipColor = vec3(0.25, 0.65, 0.12);
    }
    vec3 bladeColor = mix(baseColor, tipColor, bladeHeight);

    // Add slight color variation based on world position
    bladeColor += (variation - 0.5) * 0.08;

    // Simple diffuse lighting
    vec3 normal = normalize(fragNormal);
    float NdotL = max(dot(normal, -sunDirection), 0.0);

    // Add some fake subsurface scattering for grass
    float backlight = max(dot(normal, sunDirection), 0.0) * 0.3;

    vec3 diffuse = sunColor * (NdotL + backlight);
    vec3 litColor = bladeColor * (ambientColor + diffuse);

    // Distance fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    // Alpha fade at tip for softer look
    float alpha = smoothstep(0.0, 0.1, 1.0 - bladeHeight * 0.3);

    finalColor = vec4(litColor, alpha);
}
