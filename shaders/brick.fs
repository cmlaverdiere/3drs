#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 finalColor;

#include "common/lighting.glsl"

// Apply tangent-space normal perturbation to world normal
vec3 perturbNormal(vec3 N, vec3 pos, vec2 uv, vec3 normalOffset) {
    // Build tangent space from screen-space derivatives
    vec3 dp1 = dFdx(pos);
    vec3 dp2 = dFdy(pos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    // Solve for tangent and bitangent
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);

    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    T *= invmax;
    B *= invmax;

    // Apply the normal offset in tangent space
    return normalize(N + T * normalOffset.x + B * normalOffset.y);
}

// Brick pattern with bump mapping
// Returns: x = brick mask (1 inside brick, 0 in mortar), y = brick ID
// Outputs normalOffset for bump mapping
void brick(vec2 uv, float brickWidth, float brickHeight, float mortarWidth,
           out float brickMask, out float brickID, out vec3 normalOffset) {
    vec2 brickUV = uv / vec2(brickWidth, brickHeight);

    // Offset every other row
    float row = floor(brickUV.y);
    if (mod(row, 2.0) == 1.0) {
        brickUV.x += 0.5;
    }

    vec2 brickCell = floor(brickUV);
    vec2 brickFract = fract(brickUV);

    // Calculate mortar thresholds
    float mortarX = mortarWidth / brickWidth;
    float mortarY = mortarWidth / brickHeight;

    // Smooth brick mask with soft edges for better bump mapping
    float edgeSmooth = 0.02;
    float leftEdge = smoothstep(mortarX - edgeSmooth, mortarX + edgeSmooth, brickFract.x);
    float rightEdge = smoothstep(mortarX - edgeSmooth, mortarX + edgeSmooth, 1.0 - brickFract.x);
    float bottomEdge = smoothstep(mortarY - edgeSmooth, mortarY + edgeSmooth, brickFract.y);
    float topEdge = smoothstep(mortarY - edgeSmooth, mortarY + edgeSmooth, 1.0 - brickFract.y);

    brickMask = leftEdge * rightEdge * bottomEdge * topEdge;
    brickID = hash(brickCell);

    // === BUMP MAPPING ===
    normalOffset = vec3(0.0);

    // 1. Brick edges - normals point outward at edges to catch light
    float edgeFalloff = 0.15;  // How far edge effect extends into brick

    // Left edge - normal points left (-X)
    float leftBevel = smoothstep(mortarX + edgeFalloff, mortarX, brickFract.x);
    // Right edge - normal points right (+X)
    float rightBevel = smoothstep(1.0 - mortarX - edgeFalloff, 1.0 - mortarX, brickFract.x);
    // Bottom edge - normal points down (-Y)
    float bottomBevel = smoothstep(mortarY + edgeFalloff, mortarY, brickFract.y);
    // Top edge - normal points up (+Y)
    float topBevel = smoothstep(1.0 - mortarY - edgeFalloff, 1.0 - mortarY, brickFract.y);

    float bevelStrength = 1.5;
    normalOffset.x = (rightBevel - leftBevel) * bevelStrength;
    normalOffset.y = (topBevel - bottomBevel) * bevelStrength;

    // 2. Surface roughness - add noise-based bumps on brick surface
    float roughScale = 25.0;
    float roughStrength = 0.4;
    float n1 = noise(uv * roughScale + brickID * 100.0);
    float n2 = noise(uv * roughScale + brickID * 100.0 + vec2(0.1, 0.0));
    float n3 = noise(uv * roughScale + brickID * 100.0 + vec2(0.0, 0.1));
    normalOffset.x += (n2 - n1) * roughStrength * brickMask;
    normalOffset.y += (n3 - n1) * roughStrength * brickMask;

    // 3. Per-brick tilt variation - each brick slightly tilted differently
    float tiltStrength = 0.2;
    normalOffset.x += (hash(brickCell + vec2(1.0, 0.0)) - 0.5) * tiltStrength * brickMask;
    normalOffset.y += (hash(brickCell + vec2(0.0, 1.0)) - 0.5) * tiltStrength * brickMask;
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

    // Get brick pattern with bump mapping
    float brickMask;
    float brickID;
    vec3 normalOffset;
    brick(uv, 0.4, 0.2, 0.02, brickMask, brickID, normalOffset);

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

    // Calculate perturbed normal for bump mapping
    vec3 baseNormal = normalize(fragNormal);
    vec3 normal = perturbNormal(baseNormal, fragWorldPos, uv, normalOffset);

    // Calculate lighting with perturbed normal
    float shadow = calcShadow(fragWorldPos, baseNormal);  // Use base normal for shadow bias

    float NdotL = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = sunColor * NdotL * shadow;

    // Specular highlight for bricks (subtle)
    vec3 viewDir = normalize(viewPos - fragWorldPos);
    vec3 halfDir = normalize(-sunDirection + viewDir);
    float NdotH = max(dot(normal, halfDir), 0.0);
    float spec = pow(NdotH, 32.0) * 0.15 * brickMask * shadow;
    vec3 specular = sunColor * spec;

    vec3 pointLighting = calcAllPointLights(fragWorldPos, normal);

    vec3 litColor = surfaceColor * (ambientColor + diffuse + pointLighting) + specular;
    litColor = applyFog(litColor, fragWorldPos);

    finalColor = vec4(litColor, 1.0);
}
