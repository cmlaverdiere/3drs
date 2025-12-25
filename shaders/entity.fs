#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec4 fragColor;
in vec3 fragLocalPos;

// Raylib passes tint color through colDiffuse (for DrawModel)
// Immediate-mode functions use vertex color (fragColor)
uniform vec4 colDiffuse;

// Lighting uniforms
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform vec3 fogColor;
uniform float fogDensity;
uniform vec3 viewPos;

out vec4 finalColor;

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
    vec3 diffuse = sunColor * NdotL;

    // Specular lighting (Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0);
    float specularPower = 32.0;
    float specularStrength = 0.4;
    float spec = pow(NdotH, specularPower) * specularStrength * NdotL;
    vec3 specular = sunColor * spec;

    // Rim lighting (Fresnel-based backlight)
    float NdotV = max(dot(N, V), 0.0);
    float rimPower = 3.0;
    float rimStrength = 0.25;
    float rim = pow(1.0 - NdotV, rimPower) * rimStrength;
    // Rim is stronger when lit from behind
    float rimLight = max(0.0, dot(N, -L) * 0.5 + 0.5);
    vec3 rimColor = sunColor * rim * rimLight;

    // Combine lighting
    vec3 litColor = baseColor * (ambientColor + diffuse) + specular + rimColor;

    // Apply fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    finalColor = vec4(litColor, alpha);
}
