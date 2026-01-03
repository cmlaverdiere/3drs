#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec4 fragColor;
in vec3 fragLocalPos;

uniform vec4 colDiffuse;

#include "common/lighting.glsl"

out vec4 finalColor;

// Procedural leaf cluster pattern
float leafCluster(vec3 pos) {
    // Multiple scales of noise for organic look
    float large = noise(pos.xy * 3.0 + pos.z * 2.0);
    float medium = noise(pos.xz * 8.0 + pos.y * 5.0);
    float small = noise(pos.yz * 15.0 + pos.x * 12.0);

    // Combine for leafy texture
    float pattern = large * 0.5 + medium * 0.35 + small * 0.15;

    // Add some gaps (darker areas between leaf clusters)
    float gaps = smoothstep(0.3, 0.5, pattern);

    return gaps;
}

// Subsurface scattering approximation for leaves
float subsurfaceScatter(vec3 viewDir, vec3 lightDir, vec3 normal) {
    // Light passing through thin leaves
    vec3 scatterDir = normalize(lightDir + normal * 0.5);
    float scatter = pow(max(dot(viewDir, -scatterDir), 0.0), 3.0);
    return scatter * 0.4;
}

void main() {
    vec3 baseColor = colDiffuse.rgb;
    float alpha = colDiffuse.a;

    // Get procedural leaf pattern
    float leafPattern = leafCluster(fragLocalPos * 2.0);

    // Vary color based on pattern (lighter tips, darker depths)
    vec3 darkLeaf = baseColor * 0.6;
    vec3 lightLeaf = baseColor * 1.2;
    vec3 leafColor = mix(darkLeaf, lightLeaf, leafPattern);

    // Add slight color variation (yellow-green to blue-green)
    float colorShift = noise(fragLocalPos.xz * 4.0);
    leafColor.r += (colorShift - 0.5) * 0.08;
    leafColor.b += (0.5 - colorShift) * 0.05;

    // Normalize vectors
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-sunDirection);
    vec3 V = normalize(viewPos - fragWorldPos);

    // Diffuse lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = sunColor * NdotL;

    // Subsurface scattering - light through leaves
    float scatter = subsurfaceScatter(V, -sunDirection, N);
    vec3 subsurface = sunColor * scatter * baseColor;

    // Ambient occlusion based on leaf pattern (gaps are darker)
    float ao = 0.7 + 0.3 * leafPattern;

    // Point lights
    vec3 pointLighting = calcAllPointLights(fragWorldPos, N);

    // Combine lighting
    vec3 litColor = leafColor * (ambientColor * ao + diffuse + pointLighting) + subsurface;

    // Softer shadows on foliage (self-shadowing looks bad)
    float shadow = calcShadow(fragWorldPos, N);
    shadow = 0.4 + shadow * 0.6;  // Never fully shadowed
    litColor *= shadow;

    // Apply fog
    litColor = applyFog(litColor, fragWorldPos);

    finalColor = vec4(litColor, alpha);
}
