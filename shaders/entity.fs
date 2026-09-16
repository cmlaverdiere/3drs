#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec4 fragColor;
in vec3 fragLocalPos;

// Raylib passes tint color through colDiffuse (for DrawModel)
// Immediate-mode functions use vertex color (fragColor)
uniform vec4 colDiffuse;

#include "common/lighting.glsl"

out vec4 finalColor;

// Calculate point light contribution
vec3 calcPointLight(vec3 lightPos, vec3 lightColor, vec3 fragPos, vec3 normal, vec3 viewDir) {
    vec3 lightDir = lightPos - fragPos;
    float distance = length(lightDir);
    lightDir /= max(distance, 0.0001);

    // Soft point light settings
    float radius = 12.0;
    float intensity = 1.2;

    // Inverse square falloff
    float attenuation = intensity / (1.0 + 0.15 * distance + 0.03 * distance * distance);
    attenuation *= (1.0 - smoothstep(radius * 0.1, radius, distance));

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0) * 0.2;

    return (diff + spec) * lightColor * attenuation;
}

void main() {
    // Get base color from colDiffuse (tint passed to DrawModelEx)
    vec3 baseColor = colDiffuse.rgb;
    float alpha = colDiffuse.a;

    // Normalize vectors
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-sunDirection);
    vec3 V = normalize(viewPos - fragWorldPos);
    vec3 H = normalize(L + V);  // Half vector for Blinn-Phong

    // Diffuse lighting (sun)
    float NdotL = max(dot(N, L), 0.0);
    float shadow = calcShadow(fragWorldPos, normalize(fragNormal));
    vec3 diffuse = sunColor * NdotL * shadow;

    // Specular lighting (Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0);
    float specularPower = 32.0;
    float specularStrength = 0.4;
    float spec = pow(NdotH, specularPower) * specularStrength * NdotL;
    vec3 specular = sunColor * spec * shadow;

    // Rim lighting (Fresnel-based backlight)
    float NdotV = max(dot(N, V), 0.0);
    float rimPower = 3.0;
    float rimStrength = 0.25;
    float rim = pow(1.0 - NdotV, rimPower) * rimStrength;
    // Rim is stronger when lit from behind
    float rimLight = max(0.0, dot(N, -L) * 0.5 + 0.5);
    vec3 rimColor = sunColor * rim * rimLight * shadow;

    // Point lights contribution
    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < pointLightCount && i < MAX_POINT_LIGHTS; i++) {
        pointLighting += calcPointLight(pointLightPositions[i], pointLightColors[i], fragWorldPos, N, V);
    }

    // Combine lighting
    vec3 litColor = baseColor * (ambientColor + diffuse + pointLighting) + specular + rimColor;

    // Apply fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    finalColor = vec4(litColor, alpha);
}
