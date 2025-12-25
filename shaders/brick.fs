#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 finalColor;

#include "common/lighting.glsl"

// Brick pattern - returns: x = brick mask (1 inside brick, 0 in mortar), y = brick ID
vec2 brick(vec2 uv, float brickWidth, float brickHeight, float mortarWidth) {
    vec2 brickUV = uv / vec2(brickWidth, brickHeight);

    // Offset every other row
    float row = floor(brickUV.y);
    if (mod(row, 2.0) == 1.0) {
        brickUV.x += 0.5;
    }

    vec2 brickCell = floor(brickUV);
    vec2 brickFract = fract(brickUV);

    // Calculate mortar
    float mortarX = mortarWidth / brickWidth;
    float mortarY = mortarWidth / brickHeight;

    float brickMask = 1.0;
    if (brickFract.x < mortarX || brickFract.x > (1.0 - mortarX) ||
        brickFract.y < mortarY || brickFract.y > (1.0 - mortarY)) {
        brickMask = 0.0;
    }

    float brickID = hash(brickCell);
    return vec2(brickMask, brickID);
}

void main() {
    // Brick color palette
    vec3 darkBrick = vec3(0.5, 0.2, 0.15);
    vec3 midBrick = vec3(0.65, 0.25, 0.15);
    vec3 lightBrick = vec3(0.75, 0.35, 0.2);
    vec3 mortarColor = vec3(0.7, 0.68, 0.65);

    // Project onto visible face
    vec2 uv;
    if (abs(fragNormal.x) > 0.5) {
        uv = fragWorldPos.zy;
    } else if (abs(fragNormal.z) > 0.5) {
        uv = fragWorldPos.xy;
    } else {
        uv = fragWorldPos.xz;
    }

    // Get brick pattern
    vec2 brickData = brick(uv, 0.4, 0.2, 0.02);
    float brickMask = brickData.x;
    float brickID = brickData.y;

    // Calculate brick color with variation
    vec3 brickColor;
    if (brickID < 0.33) {
        brickColor = darkBrick;
    } else if (brickID < 0.66) {
        brickColor = midBrick;
    } else {
        brickColor = lightBrick;
    }

    // Add noise variation
    float brickNoise = fbm(uv * 15.0 + brickID * 100.0, 4) * 0.15;
    brickColor += vec3(brickNoise * 0.5, brickNoise * 0.3, brickNoise * 0.2);

    // Add wear
    float wear = noise(uv * 30.0) * 0.1;
    brickColor -= vec3(wear);

    // Mortar variation
    float mortarNoise = noise(uv * 50.0) * 0.08;
    vec3 finalMortar = mortarColor + vec3(mortarNoise);

    // Blend brick and mortar
    vec3 surfaceColor = mix(finalMortar, brickColor, brickMask);

    // Calculate lighting
    vec3 normal = normalize(fragNormal);
    float shadow = calcShadow(fragWorldPos, normal);

    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    vec3 pointLighting = calcAllPointLights(fragWorldPos, normal);

    vec3 litColor = surfaceColor * (ambientColor + diffuse + pointLighting);
    litColor = applyFog(litColor, fragWorldPos);

    finalColor = vec4(litColor, 1.0);
}
