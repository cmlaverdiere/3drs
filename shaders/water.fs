#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

uniform float time;

#include "common/lighting.glsl"

out vec4 finalColor;

void main() {
    vec2 uv = fragWorldPos.xz;

    // Scrolling water patterns
    float n1 = noise(uv * 0.3 + vec2(time * 0.5, time * 0.3));
    float n2 = noise(uv * 0.5 - vec2(time * 0.4, time * 0.2));
    float n3 = noise(uv * 1.0 + vec2(time * 0.3, -time * 0.5));

    float combined = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;

    // Water colors
    vec3 deepWater = vec3(0.0, 0.2, 0.4);
    vec3 shallowWater = vec3(0.1, 0.4, 0.6);
    vec3 highlight = vec3(0.3, 0.6, 0.8);

    // Blend based on noise
    vec3 waterColor = mix(deepWater, shallowWater, combined);

    // Add sparkle highlights
    float sparkle = noise(uv * 2.0 + vec2(time * 2.0, time * 1.5));
    if (sparkle > 0.85) {
        waterColor = mix(waterColor, highlight, (sparkle - 0.85) * 6.0);
    }

    // Calculate lighting
    vec3 normal = normalize(fragNormal);
    float shadow = calcShadow(fragWorldPos, normal);

    // Diffuse lighting
    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    // Simple specular for water shininess
    vec3 viewDir = normalize(viewPos - fragWorldPos);
    vec3 reflectDir = reflect(sunDirection, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = sunColor * spec * 0.5 * shadow;

    // Combine lighting
    vec3 litColor = waterColor * (ambientColor + diffuse + calcAllPointLights(fragWorldPos, normal)) + specular;

    // Apply fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    // Semi-transparent
    float alpha = 0.75 + combined * 0.15;

    finalColor = vec4(litColor, alpha);
}
