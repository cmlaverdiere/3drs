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

out vec4 finalColor;

void main() {
    // Grass blade color gradient - darker at base, lighter at tip
    vec3 baseColor = vec3(0.08, 0.35, 0.06);
    vec3 tipColor = vec3(0.25, 0.65, 0.12);
    vec3 bladeColor = mix(baseColor, tipColor, bladeHeight);

    // Add slight color variation based on world position
    float variation = fract(sin(dot(fragWorldPos.xz, vec2(12.9898, 78.233))) * 43758.5453);
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
