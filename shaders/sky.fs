#version 330

in vec3 fragPos;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 skyColorZenith;
uniform vec3 skyColorHorizon;
uniform float timeOfDay;

out vec4 finalColor;

void main() {
    vec3 viewDir = normalize(fragPos);

    // Height-based gradient (y component of view direction)
    float height = viewDir.y * 0.5 + 0.5;  // Remap -1..1 to 0..1
    height = clamp(height, 0.0, 1.0);

    // Exponential falloff for more realistic horizon blend
    float gradientFactor = pow(height, 0.4);

    // Base sky color (gradient from horizon to zenith)
    vec3 skyColor = mix(skyColorHorizon, skyColorZenith, gradientFactor);

    // Sun disc
    vec3 toSun = -sunDirection;
    float sunAngle = dot(viewDir, toSun);

    // Sun core (sharp disc)
    float sunDisc = smoothstep(0.9995, 0.9998, sunAngle);

    // Sun glow (soft halo)
    float sunGlow = pow(max(sunAngle, 0.0), 256.0) * 0.5;
    sunGlow += pow(max(sunAngle, 0.0), 32.0) * 0.3;

    // Add sun to sky
    vec3 sunFullColor = sunColor * 2.0;  // Brighter sun
    skyColor += sunDisc * sunFullColor;
    skyColor += sunGlow * sunColor;

    // Horizon glow near sun
    float horizonGlow = pow(max(sunAngle, 0.0), 8.0) * (1.0 - height) * 0.4;
    skyColor += horizonGlow * sunColor;

    // Subtle color variation based on angle to sun
    float sunInfluence = pow(max(dot(viewDir, toSun), 0.0), 4.0);
    skyColor = mix(skyColor, skyColor * sunColor, sunInfluence * 0.2);

    finalColor = vec4(skyColor, 1.0);
}
