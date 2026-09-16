#version 330

in vec3 fragWorldPos;
in vec3 fragNormal;
in vec3 fragObjectNormal;
in vec4 fragColor;
in vec3 fragObjectPos;

uniform vec4 colDiffuse;

// Monster material uniforms
uniform int materialType;   // 0=flat, 1=scales, 2=stone, 3=fur, 4=striped, 5=spotted
uniform float monsterSeed;

#include "common/lighting.glsl"

out vec4 finalColor;

// ============================================================
// Noise functions
// ============================================================



vec2 hash2(vec2 p) {
    return vec2(
        fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453),
        fract(sin(dot(p, vec2(269.5, 183.3))) * 43758.5453)
    );
}





// ============================================================
// Tri-planar projection
// ============================================================

vec3 getTriplanarBlend(vec3 normal) {
    vec3 blend = abs(normal);
    blend = pow(blend, vec3(4.0));
    blend = blend / (blend.x + blend.y + blend.z + 0.001);
    return blend;
}

vec2 triplanarUV(vec3 pos, vec3 normal) {
    vec3 blend = getTriplanarBlend(normal);
    vec2 uvX = pos.zy;
    vec2 uvY = pos.xz;
    vec2 uvZ = pos.xy;
    return uvX * blend.x + uvY * blend.y + uvZ * blend.z;
}

// ============================================================
// SCALES - Reptilian/dragon scales with bump mapping
// ============================================================

void scales(vec2 uv, float seed, vec3 baseColor, out vec3 color, out vec3 normalOffset) {
    vec2 p = uv * 6.0;

    // Offset every other row for proper scale pattern
    float row = floor(p.y);
    if (mod(row, 2.0) > 0.5) {
        p.x += 0.5;
    }

    vec2 cell = floor(p);
    vec2 f = fract(p);

    // Create pointed oval scale shape
    vec2 center = vec2(0.5, 0.5);
    vec2 d = f - center;
    d.y *= 1.4;  // Elongate vertically
    float dist = length(d);

    // Scale edge detection
    float edge = (1.0 - smoothstep(0.35, 0.5, dist));
    float rim = smoothstep(0.35, 0.45, dist) * 0.6;

    // Per-scale color variation
    float colorVar = hash(cell + seed) * 0.3 - 0.15;

    // === BUMP MAPPING ===
    // Each scale is like a dome - normal points outward from center
    // Stronger effect = more obvious 3D
    vec2 scaleNormalXY = -d * 2.5;  // Point away from center
    scaleNormalXY *= (1.0 - smoothstep(0.2, 0.5, dist));  // Fade at edges

    // Add rim highlight normal (edges catch light)
    float rimStrength = smoothstep(0.3, 0.45, dist) * (1.0 - smoothstep(0.45, 0.5, dist));
    scaleNormalXY += normalize(d + 0.001) * rimStrength * 1.5;

    normalOffset = vec3(scaleNormalXY.x, scaleNormalXY.y, 0.0);

    // Color
    color = baseColor * (1.0 + colorVar);
    color *= (0.7 + edge * 0.3);  // Darker in gaps
    color += rim * vec3(0.2, 0.22, 0.25);  // Subtle rim tint
}

// ============================================================
// STONE - Cracked rocky surface with bump mapping
// ============================================================

void stone(vec2 uv, float seed, vec3 baseColor, out vec3 color, out vec3 normalOffset) {
    vec2 p = uv * 3.0;
    vec2 i = floor(p);
    vec2 f = fract(p);

    float minDist = 1.0;
    float secondDist = 1.0;
    vec2 nearestCell = vec2(0.0);
    vec2 nearestPoint = vec2(0.0);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 cell = i + neighbor;
            vec2 point = hash2(cell + seed) * 0.7 + 0.15;
            vec2 diff = neighbor + point - f;
            float dist = length(diff);
            if (dist < minDist) {
                secondDist = minDist;
                minDist = dist;
                nearestCell = cell;
                nearestPoint = diff;
            } else if (dist < secondDist) {
                secondDist = dist;
            }
        }
    }

    // Crack detection
    float crack = secondDist - minDist;
    float crackLine = smoothstep(0.0, 0.1, crack);

    // === BUMP MAPPING ===
    // Stone chunks are raised, cracks are recessed
    // Normal points toward center of each chunk
    vec2 chunkNormalXY = -nearestPoint * 1.5;
    chunkNormalXY *= smoothstep(0.0, 0.3, crack);  // Flatten near cracks

    // Crack edges - sharp normal change
    float crackEdge = (1.0 - smoothstep(0.0, 0.05, crack));
    chunkNormalXY += normalize(nearestPoint + 0.001) * crackEdge * 2.0;

    // Surface roughness bumps
    float rough1 = noise(uv * 20.0 + seed);
    float rough2 = noise(uv * 20.0 + seed + vec2(0.1, 0.0));
    float rough3 = noise(uv * 20.0 + seed + vec2(0.0, 0.1));
    chunkNormalXY.x += (rough2 - rough1) * 1.0;
    chunkNormalXY.y += (rough3 - rough1) * 1.0;

    normalOffset = vec3(chunkNormalXY.x, chunkNormalXY.y, 0.0);

    // Color
    float chunkVar = hash(nearestCell + seed * 2.0) * 0.4 - 0.2;
    color = baseColor * (1.0 + chunkVar);
    color *= crackLine;  // Dark cracks
    color *= (0.9 + rough1 * 0.2);
}

// ============================================================
// FUR - Fluffy directional strands with bump mapping
// ============================================================

void fur(vec2 uv, float seed, vec3 baseColor, out vec3 color, out vec3 normalOffset) {
    // Multiple layers of fur strands
    float strand1 = noise(vec2(uv.x * 80.0, uv.y * 8.0 + seed));
    float strand2 = noise(vec2(uv.x * 40.0 + 10.0, uv.y * 6.0 + seed));
    float strand3 = noise(vec2(uv.x * 120.0 - 5.0, uv.y * 10.0 + seed));

    float strands = strand1 * 0.5 + strand2 * 0.3 + strand3 * 0.2;

    // === BUMP MAPPING ===
    // Fur strands run vertically, create horizontal normal variation
    float strandDx1 = noise(vec2((uv.x + 0.01) * 80.0, uv.y * 8.0 + seed)) - strand1;
    float strandDx2 = noise(vec2((uv.x + 0.01) * 40.0 + 10.0, uv.y * 6.0 + seed)) - strand2;

    // Strands create ridges - strong horizontal normal
    float normalX = (strandDx1 * 0.5 + strandDx2 * 0.3) * 40.0;

    // Slight vertical variation for clumping
    float clumpNoise = noise(uv * 8.0 + seed * 2.0);
    float clumpDy = noise(uv * 8.0 + seed * 2.0 + vec2(0.0, 0.01)) - clumpNoise;
    float normalY = clumpDy * 10.0;

    normalOffset = vec3(normalX, normalY, 0.0);

    // Color with lighter tips effect
    float tipLight = noise(uv * 20.0 + seed) * 0.3;
    color = baseColor;
    color *= (0.6 + strands * 0.5);
    color += tipLight * vec3(0.15, 0.12, 0.1);

    // Sheen on raised strands
    color += pow(strands, 3.0) * 0.15 * vec3(1.0, 0.95, 0.85);
}

// ============================================================
// STRIPES - Bold tiger/zebra stripes with bump mapping
// ============================================================

void stripes(vec2 uv, float seed, vec3 baseColor, out vec3 color, out vec3 normalOffset) {
    // Organic distortion
    float distort = fbm(uv * 2.0 + seed, 2) * 0.8;

    // Main stripe pattern
    float stripeY = uv.y * 8.0 + distort;
    float stripe = sin(stripeY * 3.14159);
    float stripeSmooth = smoothstep(-0.3, 0.3, stripe);

    // === BUMP MAPPING ===
    // Stripes are slightly raised ridges
    // Normal changes at stripe edges
    float stripeEdge = cos(stripeY * 3.14159);  // Derivative of sine
    float edgeStrength = 1.0 - abs(stripe);  // Strong at transitions
    edgeStrength = pow(edgeStrength, 0.5) * 2.0;

    // Stripe ridges point perpendicular to stripe direction
    float normalY = stripeEdge * edgeStrength * 0.8;

    // Add some distortion-based normal variation
    float distortDx = fbm(uv * 2.0 + seed + vec2(0.01, 0.0), 2) - fbm(uv * 2.0 + seed, 2);
    float normalX = distortDx * 5.0;

    normalOffset = vec3(normalX, normalY, 0.0);

    // Color
    float pattern = smoothstep(0.3, 0.7, stripeSmooth);
    vec3 stripeColor = baseColor * 0.15;  // Dark stripes
    color = mix(stripeColor, baseColor, pattern);

    // Subtle fur texture
    float furTex = noise(vec2(uv.x * 50.0, uv.y * 8.0) + seed) * 0.1;
    color *= (0.95 + furTex);
}

// ============================================================
// SPOTS - Leopard/jaguar spots with bump mapping
// ============================================================

void spots(vec2 uv, float seed, vec3 baseColor, out vec3 color, out vec3 normalOffset) {
    vec2 p = uv * 4.0;

    float totalSpots = 0.0;
    vec3 totalNormal = vec3(0.0);

    // Generate spots using layered approach
    for (int layer = 0; layer < 2; layer++) {
        vec2 lp = p * (1.0 + float(layer) * 0.5) + float(layer) * 10.0;
        vec2 i = floor(lp);
        vec2 f = fract(lp);

        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                vec2 neighbor = vec2(float(x), float(y));
                vec2 cell = i + neighbor;
                vec2 point = hash2(cell + seed + float(layer) * 100.0) * 0.6 + 0.2;
                vec2 diff = neighbor + point - f;
                float dist = length(diff);

                // Spot (filled circle) - raised bump
                float spotRadius = 0.22;
                float spot = 1.0 - smoothstep(spotRadius - 0.05, spotRadius + 0.05, dist);
                spot *= (1.0 - float(layer) * 0.3);

                if (spot > 0.01) {
                    totalSpots = max(totalSpots, spot);

                    // === BUMP MAPPING ===
                    // Each spot is a dome - normal points outward from center
                    vec2 spotNormalXY = -diff / (dist + 0.001) * spot * 2.0;
                    // Smooth falloff
                    spotNormalXY *= (1.0 - smoothstep(spotRadius - 0.1, spotRadius + 0.05, dist));
                    totalNormal.xy += spotNormalXY;
                }

                // Rosette ring (hollow circle) - first layer only
                if (layer == 0) {
                    float ringInner = 0.25;
                    float ringOuter = 0.35;
                    float ring = smoothstep(ringInner, ringInner + 0.03, dist) *
                                 (1.0 - smoothstep(ringOuter - 0.03, ringOuter, dist));

                    if (ring > 0.01) {
                        totalSpots = max(totalSpots, ring * 0.7);
                        // Ring creates a ridge - normal points away from ring center
                        float ringMid = (ringInner + ringOuter) * 0.5;
                        float ridgeDir = sign(dist - ringMid);
                        vec2 ringNormalXY = diff / (dist + 0.001) * ring * ridgeDir * 1.5;
                        totalNormal.xy += ringNormalXY;
                    }
                }
            }
        }
    }

    normalOffset = totalNormal;

    // Color
    vec3 spotColor = baseColor * 0.2;
    color = mix(baseColor, spotColor, totalSpots);

    // Subtle fur texture
    float furTex = noise(vec2(uv.x * 40.0, uv.y * 8.0) + seed) * 0.08;
    color *= (0.96 + furTex);
}

// ============================================================
// Apply tangent-space normal to world normal
// ============================================================

vec3 perturbNormal(vec3 N, vec3 pos, vec2 uv, vec3 normalOffset) {
    // Build tangent space from derivatives
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
    vec3 perturbedN = normalize(N + T * normalOffset.x + B * normalOffset.y);
    return perturbedN;
}

// ============================================================
// Lighting
// ============================================================

vec3 calcPointLight(vec3 lightPos, vec3 lightColor, vec3 fragPos, vec3 normal, vec3 viewDir) {
    vec3 lightDir = lightPos - fragPos;
    float distance = length(lightDir);
    lightDir /= max(distance, 0.0001);
    float radius = 12.0;
    float intensity = 1.2;
    float attenuation = intensity / (1.0 + 0.15 * distance + 0.03 * distance * distance);
    attenuation *= (1.0 - smoothstep(radius * 0.1, radius, distance));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0) * 0.3;
    return (diff + spec) * lightColor * attenuation;
}

// ============================================================
// Main
// ============================================================

void main() {
    vec3 baseColor = colDiffuse.rgb;
    float alpha = colDiffuse.a;

    // Get tri-planar UVs using object-space normal for stable mapping
    vec3 objN = normalize(fragObjectNormal);
    vec2 uv = triplanarUV(fragObjectPos, objN);

    // World-space normal for lighting (will be perturbed)
    vec3 N = normalize(fragNormal);

    // Apply material pattern and get normal perturbation
    vec3 patternedColor = baseColor;
    vec3 normalOffset = vec3(0.0);

    if (materialType == 1) {
        scales(uv, monsterSeed, baseColor, patternedColor, normalOffset);
    } else if (materialType == 2) {
        stone(uv, monsterSeed, baseColor, patternedColor, normalOffset);
    } else if (materialType == 3) {
        fur(uv, monsterSeed, baseColor, patternedColor, normalOffset);
    } else if (materialType == 4) {
        stripes(uv, monsterSeed, baseColor, patternedColor, normalOffset);
    } else if (materialType == 5) {
        spots(uv, monsterSeed, baseColor, patternedColor, normalOffset);
    }

    // Apply normal perturbation (bump mapping)
    if (materialType > 0) {
        N = perturbNormal(N, fragWorldPos, uv, normalOffset);
    }

    // Lighting calculations with perturbed normal
    vec3 L = normalize(-sunDirection);
    vec3 V = normalize(viewPos - fragWorldPos);
    vec3 H = normalize(L + V);

    // Diffuse
    float NdotL = max(dot(N, L), 0.0);
    float shadow = calcShadow(fragWorldPos, normalize(fragNormal));
    vec3 diffuse = sunColor * NdotL * shadow;

    // Specular (Blinn-Phong) - stronger for textured materials
    float specPower = (materialType == 1) ? 64.0 : 32.0;  // Shinier scales
    float specStrength = (materialType == 1) ? 0.8 : 0.5;  // More specular on scales
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, specPower) * specStrength * NdotL;
    vec3 specular = sunColor * spec * shadow;

    // Rim lighting - enhanced for textured materials
    float NdotV = max(dot(N, V), 0.0);
    float rim = pow(1.0 - NdotV, 3.0) * 0.3;
    float rimLight = max(0.0, dot(N, -L) * 0.5 + 0.5);
    vec3 rimColor = sunColor * rim * rimLight * shadow;

    // Point lights
    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < pointLightCount && i < MAX_POINT_LIGHTS; i++) {
        pointLighting += calcPointLight(pointLightPositions[i], pointLightColors[i], fragWorldPos, N, V);
    }

    // Combine lighting
    vec3 litColor = patternedColor * (ambientColor + diffuse + pointLighting) + specular + rimColor;

    // Fog
    float dist = length(viewPos - fragWorldPos);
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    litColor = mix(fogColor, litColor, fogFactor);

    finalColor = vec4(litColor, alpha);
}
