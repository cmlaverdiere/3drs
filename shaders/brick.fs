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
// Outputs: brickMask (1=brick, 0=mortar), brickID (per-brick hash), normalOffset for bump mapping
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

    // Calculate mortar thresholds (in brick-space units)
    float mortarX = mortarWidth / brickWidth;
    float mortarY = mortarWidth / brickHeight;

    // Brick mask - 1 inside brick, 0 in mortar
    float edgeSmooth = 0.02;
    float leftEdge = smoothstep(mortarX - edgeSmooth, mortarX + edgeSmooth, brickFract.x);
    float rightEdge = smoothstep(mortarX - edgeSmooth, mortarX + edgeSmooth, 1.0 - brickFract.x);
    float bottomEdge = smoothstep(mortarY - edgeSmooth, mortarY + edgeSmooth, brickFract.y);
    float topEdge = smoothstep(mortarY - edgeSmooth, mortarY + edgeSmooth, 1.0 - brickFract.y);

    brickMask = leftEdge * rightEdge * bottomEdge * topEdge;
    brickID = hash(brickCell);

    // === BUMP MAPPING ===
    // Think of height profile: bricks are raised plateaus, mortar is recessed valley
    // Normal = gradient of height field
    normalOffset = vec3(0.0);

    // Distance from each edge (positive = inside brick, negative = in mortar)
    float distLeft = brickFract.x - mortarX;
    float distRight = (1.0 - mortarX) - brickFract.x;
    float distBottom = brickFract.y - mortarY;
    float distTop = (1.0 - mortarY) - brickFract.y;

    // Find closest edge and distance to it
    float minDistX = min(distLeft, distRight);
    float minDistY = min(distBottom, distTop);
    float minDist = min(minDistX, minDistY);

    // Mortar depth effect - in mortar, normal points toward nearest brick
    // This creates the "valley" that makes mortar look recessed
    float mortarDepth = 3.0;  // How deep the mortar appears
    float mortarSlope = 2.5;  // How steep the sides are

    if (minDist < 0.0) {
        // We're in the mortar - create slope toward nearest brick
        // Direction toward brick center
        vec2 toBrickCenter = vec2(0.5, 0.5) - brickFract;

        // Stronger effect deeper in mortar, but limit it
        float mortarAmount = clamp(-minDist * 10.0, 0.0, 1.0);

        // Normal points toward brick (upward slope)
        normalOffset.x = sign(toBrickCenter.x) * mortarSlope * mortarAmount;
        normalOffset.y = sign(toBrickCenter.y) * mortarSlope * mortarAmount;
    }

    // Brick edge bevels - the raised lip where brick meets mortar
    float bevelWidth = 0.12;  // How wide the bevel is
    float bevelStrength = 2.0;

    // Only apply bevel on brick side (minDist > 0)
    if (minDist > 0.0 && minDist < bevelWidth) {
        float bevelAmount = 1.0 - (minDist / bevelWidth);
        bevelAmount = bevelAmount * bevelAmount;  // Quadratic falloff

        // Which edge are we closest to?
        if (minDistX < minDistY) {
            // Closer to left/right edge
            float sign_x = (distLeft < distRight) ? -1.0 : 1.0;
            normalOffset.x += sign_x * bevelStrength * bevelAmount;
        } else {
            // Closer to top/bottom edge
            float sign_y = (distBottom < distTop) ? -1.0 : 1.0;
            normalOffset.y += sign_y * bevelStrength * bevelAmount;
        }
    }

    // Surface roughness - noise bumps on brick surface only
    float roughScale = 25.0;
    float roughStrength = 0.5;
    float n1 = noise(uv * roughScale + brickID * 100.0);
    float n2 = noise(uv * roughScale + brickID * 100.0 + vec2(0.1, 0.0));
    float n3 = noise(uv * roughScale + brickID * 100.0 + vec2(0.0, 0.1));
    normalOffset.x += (n2 - n1) * roughStrength * brickMask;
    normalOffset.y += (n3 - n1) * roughStrength * brickMask;

    // Per-brick tilt - each brick slightly tilted differently
    float tiltStrength = 0.25;
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

    // Mortar variation - darker base to emphasize depth
    float mortarNoise = noise(uv * 50.0) * 0.08;
    vec3 finalMortar = mortarColor * 0.7 + vec3(mortarNoise);  // Darken mortar

    // Blend brick and mortar
    vec3 surfaceColor = mix(finalMortar, brickColor, brickMask);

    // Add ambient occlusion in mortar grooves
    float ao = mix(0.6, 1.0, brickMask);  // Darker in mortar
    surfaceColor *= ao;

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
