#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 finalColor;

#include "common/lighting.glsl"

// Wood grain pattern
float woodGrain(vec3 pos) {
    float dist = length(pos.xz * 0.5);
    float rings = sin(dist * 20.0 + noise(pos.xz * 2.0) * 3.0) * 0.5 + 0.5;
    float grain = noise(vec2(pos.y * 15.0, dist * 5.0)) * 0.3;
    return rings * 0.7 + grain;
}

void main() {
    // Wood color palette
    vec3 darkWood = vec3(0.35, 0.2, 0.1);
    vec3 lightWood = vec3(0.6, 0.4, 0.2);
    vec3 midWood = vec3(0.5, 0.3, 0.15);

    // Calculate wood pattern
    float pattern = woodGrain(fragWorldPos);
    float n = noise(fragWorldPos.xz * 4.0 + fragWorldPos.y * 2.0);
    pattern = pattern * 0.8 + n * 0.2;

    // Blend colors based on pattern
    vec3 woodColor;
    if (pattern < 0.4) {
        woodColor = mix(darkWood, midWood, pattern / 0.4);
    } else {
        woodColor = mix(midWood, lightWood, (pattern - 0.4) / 0.6);
    }

    // Calculate lighting
    vec3 normal = normalize(fragNormal);
    float shadow = calcShadow(fragWorldPos, normal);

    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    vec3 pointLighting = calcAllPointLights(fragWorldPos, normal);

    vec3 litColor = woodColor * (ambientColor + diffuse + pointLighting);
    litColor = applyFog(litColor, fragWorldPos);

    finalColor = vec4(litColor, 1.0);
}
