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

    // Normalize the interpolated normal
    vec3 normal = normalize(fragNormal);

    // Directional lighting (sun)
    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL;

    // Combine ambient and diffuse
    vec3 litColor = baseColor * (ambientColor + diffuse);

    // Apply fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    finalColor = vec4(litColor, alpha);
}
