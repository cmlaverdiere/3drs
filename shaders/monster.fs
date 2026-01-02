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

// Lighting uniforms
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform vec3 fogColor;
uniform float fogDensity;
uniform vec3 viewPos;

#define MAX_POINT_LIGHTS 16
uniform vec3 pointLightPositions[MAX_POINT_LIGHTS];
uniform vec3 pointLightColors[MAX_POINT_LIGHTS];
uniform int pointLightCount;

out vec4 finalColor;

// ============================================================
// Noise functions
// ============================================================

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec2 hash2(vec2 p) {
    return vec2(
        fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453),
        fract(sin(dot(p, vec2(269.5, 183.3))) * 43758.5453)
    );
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// ============================================================
// Tri-planar projection
// ============================================================

vec2 triplanarUV(vec3 pos, vec3 normal) {
    vec3 blend = abs(normal);
    blend = pow(blend, vec3(4.0));  // Sharper blend
    blend = blend / (blend.x + blend.y + blend.z + 0.001);
    vec2 uvX = pos.zy;
    vec2 uvY = pos.xz;
    vec2 uvZ = pos.xy;
    return uvX * blend.x + uvY * blend.y + uvZ * blend.z;
}

// ============================================================
// SCALES - Reptilian/dragon scales with 3D appearance
// ============================================================

vec3 scales(vec2 uv, float seed, vec3 baseColor) {
    vec2 p = uv * 6.0;  // Scale size

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

    // Scale edge with rim lighting effect
    float edge = smoothstep(0.5, 0.35, dist);
    float rim = smoothstep(0.35, 0.45, dist) * 0.6;

    // Per-scale color variation
    float colorVar = hash(cell + seed) * 0.3 - 0.15;

    // Fake 3D: darker at edges, lighter at center top
    float highlight = smoothstep(0.4, 0.0, dist) * 0.4;
    highlight *= smoothstep(0.5, 0.3, f.y);  // Top of scale brighter

    // Shadow under each scale
    float shadow = smoothstep(0.3, 0.5, f.y) * 0.3;

    vec3 color = baseColor * (1.0 + colorVar);
    color *= (0.7 + edge * 0.3);  // Darker in gaps
    color += highlight * vec3(1.2, 1.1, 1.0);  // Warm highlight
    color *= (1.0 - shadow);  // Shadow at bottom
    color += rim * vec3(0.3, 0.35, 0.4);  // Cool rim light

    return color;
}

// ============================================================
// STONE - Cracked rocky surface
// ============================================================

vec3 stone(vec2 uv, float seed, vec3 baseColor) {
    // Large cracks
    vec2 p = uv * 3.0;
    vec2 i = floor(p);
    vec2 f = fract(p);

    float minDist = 1.0;
    vec2 nearestCell = vec2(0.0);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 cell = i + neighbor;
            vec2 point = hash2(cell + seed) * 0.7 + 0.15;
            float dist = length(neighbor + point - f);
            if (dist < minDist) {
                minDist = dist;
                nearestCell = cell;
            }
        }
    }

    // Crack lines (dark)
    float crack = smoothstep(0.02, 0.08, minDist);

    // Per-chunk color variation
    float chunkVar = hash(nearestCell + seed * 2.0) * 0.4 - 0.2;

    // Surface roughness
    float rough = fbm(uv * 15.0 + seed, 3) * 0.2;

    // Weathering - darker in recesses
    float weather = fbm(uv * 8.0 + seed * 3.0, 2) * 0.25;

    // Subtle highlights on raised areas
    float highlight = (1.0 - rough) * 0.15;

    vec3 color = baseColor * (1.0 + chunkVar);
    color *= crack;  // Dark cracks
    color *= (0.85 + rough);  // Surface variation
    color *= (1.0 - weather * 0.5);  // Weathering
    color += highlight * vec3(1.0, 0.95, 0.9);

    return color;
}

// ============================================================
// FUR - Fluffy directional strands
// ============================================================

vec3 fur(vec2 uv, float seed, vec3 baseColor) {
    // Multiple layers of fur strands at different scales
    float strand1 = noise(vec2(uv.x * 80.0, uv.y * 8.0 + seed));
    float strand2 = noise(vec2(uv.x * 40.0 + 10.0, uv.y * 6.0 + seed));
    float strand3 = noise(vec2(uv.x * 120.0 - 5.0, uv.y * 10.0 + seed));

    // Combine strands
    float strands = strand1 * 0.5 + strand2 * 0.3 + strand3 * 0.2;

    // Clumping variation
    float clump = noise(uv * 5.0 + seed * 2.0);

    // Color variation (lighter tips, darker roots)
    float tipLight = noise(uv * 20.0 + seed) * 0.3;

    // Subtle warm/cool variation
    float warmCool = noise(uv * 3.0 + seed * 4.0) * 0.15;

    vec3 color = baseColor;
    color *= (0.6 + strands * 0.5);  // Strand darkness variation
    color *= (0.8 + clump * 0.3);  // Clumping
    color += tipLight * vec3(1.1, 1.05, 1.0);  // Lighter tips
    color += warmCool * vec3(0.1, -0.05, -0.1);  // Warm variation

    // Subtle sheen
    float sheen = pow(strands, 3.0) * 0.2;
    color += sheen * vec3(1.0, 0.95, 0.85);

    return color;
}

// ============================================================
// STRIPES - Bold tiger/zebra stripes
// ============================================================

vec3 stripes(vec2 uv, float seed, vec3 baseColor) {
    // Organic distortion
    float distort = fbm(uv * 2.0 + seed, 2) * 0.8;

    // Main stripe pattern
    float stripe = sin((uv.y * 8.0 + distort) * 3.14159);
    stripe = smoothstep(-0.3, 0.3, stripe);

    // Secondary smaller stripes
    float stripe2 = sin((uv.y * 16.0 + distort * 1.5 + 1.0) * 3.14159);
    stripe2 = smoothstep(-0.4, 0.4, stripe2) * 0.5;

    // Combine
    float pattern = max(stripe, stripe2);

    // Edge softness variation
    float edgeVar = noise(uv * 10.0 + seed) * 0.2;
    pattern = smoothstep(0.3 - edgeVar, 0.7 + edgeVar, pattern);

    // Dark stripe color (very dark, almost black)
    vec3 stripeColor = baseColor * 0.15;

    // Mix base and stripe
    vec3 color = mix(stripeColor, baseColor, pattern);

    // Subtle fur texture on top
    float furTex = noise(vec2(uv.x * 50.0, uv.y * 8.0) + seed) * 0.1;
    color *= (0.95 + furTex);

    return color;
}

// ============================================================
// SPOTS - Leopard/jaguar spots
// ============================================================

vec3 spots(vec2 uv, float seed, vec3 baseColor) {
    vec2 p = uv * 4.0;

    float spots = 0.0;
    float rosettes = 0.0;

    // Generate spots using layered voronoi
    for (int layer = 0; layer < 2; layer++) {
        vec2 lp = p * (1.0 + float(layer) * 0.5) + float(layer) * 10.0;
        vec2 i = floor(lp);
        vec2 f = fract(lp);

        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                vec2 neighbor = vec2(float(x), float(y));
                vec2 cell = i + neighbor;
                vec2 point = hash2(cell + seed + float(layer) * 100.0) * 0.6 + 0.2;
                float dist = length(neighbor + point - f);

                // Spot (filled circle)
                float spot = 1.0 - smoothstep(0.15, 0.25, dist);
                spots = max(spots, spot * (1.0 - float(layer) * 0.3));

                // Rosette ring (hollow circle) - only on first layer
                if (layer == 0) {
                    float ring = smoothstep(0.2, 0.25, dist) * (1.0 - smoothstep(0.3, 0.35, dist));
                    rosettes = max(rosettes, ring);
                }
            }
        }
    }

    // Combine spots and rosettes
    float pattern = max(spots * 0.9, rosettes * 0.7);

    // Spot color (dark brown/black)
    vec3 spotColor = baseColor * 0.2;

    // Mix
    vec3 color = mix(baseColor, spotColor, pattern);

    // Subtle fur texture
    float furTex = noise(vec2(uv.x * 40.0, uv.y * 8.0) + seed) * 0.08;
    color *= (0.96 + furTex);

    return color;
}

// ============================================================
// Lighting
// ============================================================

vec3 calcPointLight(vec3 lightPos, vec3 lightColor, vec3 fragPos, vec3 normal, vec3 viewDir) {
    vec3 lightDir = lightPos - fragPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    float radius = 12.0;
    float intensity = 1.2;
    float attenuation = intensity / (1.0 + 0.15 * distance + 0.03 * distance * distance);
    attenuation *= smoothstep(radius, radius * 0.1, distance);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0) * 0.2;
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

    // World-space normal for lighting
    vec3 N = normalize(fragNormal);

    // Apply material pattern
    vec3 patternedColor = baseColor;

    if (materialType == 1) {
        patternedColor = scales(uv, monsterSeed, baseColor);
    } else if (materialType == 2) {
        patternedColor = stone(uv, monsterSeed, baseColor);
    } else if (materialType == 3) {
        patternedColor = fur(uv, monsterSeed, baseColor);
    } else if (materialType == 4) {
        patternedColor = stripes(uv, monsterSeed, baseColor);
    } else if (materialType == 5) {
        patternedColor = spots(uv, monsterSeed, baseColor);
    }
    // materialType == 0 is flat (no pattern modification)

    // Lighting calculations
    vec3 L = normalize(-sunDirection);
    vec3 V = normalize(viewPos - fragWorldPos);
    vec3 H = normalize(L + V);

    // Diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = sunColor * NdotL;

    // Specular (Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, 32.0) * 0.4 * NdotL;
    vec3 specular = sunColor * spec;

    // Rim lighting
    float NdotV = max(dot(N, V), 0.0);
    float rim = pow(1.0 - NdotV, 3.0) * 0.25;
    float rimLight = max(0.0, dot(N, -L) * 0.5 + 0.5);
    vec3 rimColor = sunColor * rim * rimLight;

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
