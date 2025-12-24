#version 330

in vec3 fragPos;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 skyColorZenith;
uniform vec3 skyColorHorizon;
uniform float timeOfDay;

out vec4 finalColor;

// Simple hash for noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Value noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal noise for clouds
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec3 viewDir = normalize(fragPos);

    // Height factor (0 at horizon, 1 at zenith)
    float height = max(viewDir.y, 0.0);

    // Smooth gradient with exponential falloff
    float gradientFactor = 1.0 - exp(-3.0 * height);

    // Base sky color (gradient from horizon to zenith)
    vec3 skyColor = mix(skyColorHorizon, skyColorZenith, gradientFactor);

    // Add slight color variation based on horizontal angle
    float hueShift = sin(atan(viewDir.z, viewDir.x) * 2.0) * 0.02;
    skyColor += vec3(hueShift, 0.0, -hueShift);

    // Sun calculations
    vec3 toSun = -sunDirection;
    float sunAngle = dot(viewDir, toSun);

    // Sun disc (sharp edge)
    float sunDisc = smoothstep(0.9993, 0.9995, sunAngle);

    // Sun glow layers
    float innerGlow = pow(max(sunAngle, 0.0), 256.0) * 0.6;
    float outerGlow = pow(max(sunAngle, 0.0), 32.0) * 0.4;
    float wideGlow = pow(max(sunAngle, 0.0), 8.0) * 0.15;

    // Add sun to sky
    vec3 sunFullColor = sunColor * 2.5;
    skyColor += sunDisc * sunFullColor;
    skyColor += (innerGlow + outerGlow) * sunColor;
    skyColor += wideGlow * sunColor * 0.5;

    // Horizon glow (stronger when sun is near horizon)
    float horizonGlow = pow(max(sunAngle, 0.0), 4.0) * (1.0 - height) * 0.5;
    skyColor += horizonGlow * sunColor;

    // Procedural clouds (only during day, above horizon)
    float dayFactor = smoothstep(0.1, 0.3, timeOfDay) * smoothstep(0.9, 0.7, timeOfDay);
    if (height > 0.05 && dayFactor > 0.0) {
        // Project view direction onto a plane for cloud coordinates
        vec2 cloudCoord = viewDir.xz / (viewDir.y + 0.1) * 0.3;

        // Animate clouds slowly
        cloudCoord += vec2(timeOfDay * 0.5, 0.0);

        // Generate cloud density
        float cloudNoise = fbm(cloudCoord * 3.0);
        float cloudDensity = smoothstep(0.4, 0.7, cloudNoise);

        // Fade clouds near horizon and zenith
        float cloudMask = smoothstep(0.05, 0.2, height) * smoothstep(0.9, 0.5, height);
        cloudDensity *= cloudMask * dayFactor * 0.6;

        // Cloud color (white with slight sun tinting)
        vec3 cloudColor = vec3(1.0) + sunColor * 0.1;

        // Cloud shading (darker on bottom)
        float cloudShade = 0.8 + 0.2 * smoothstep(0.5, 0.7, cloudNoise);
        cloudColor *= cloudShade;

        // Blend clouds into sky
        skyColor = mix(skyColor, cloudColor, cloudDensity);
    }

    // Atmospheric scattering effect near horizon
    float atmosphereFactor = pow(1.0 - height, 4.0) * 0.3;
    skyColor = mix(skyColor, skyColorHorizon, atmosphereFactor);

    finalColor = vec4(skyColor, 1.0);
}
