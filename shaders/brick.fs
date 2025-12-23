#version 330

in vec2 fragTexCoord;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 finalColor;

// Hash function for noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Value noise
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

// Fractal noise
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Brick pattern
// Returns: x = brick mask (1 inside brick, 0 in mortar), y = brick ID for color variation
vec2 brick(vec2 uv, float brickWidth, float brickHeight, float mortarWidth) {
    // Scale UV to brick size
    vec2 brickUV = uv / vec2(brickWidth, brickHeight);

    // Offset every other row
    float row = floor(brickUV.y);
    if (mod(row, 2.0) == 1.0) {
        brickUV.x += 0.5;
    }

    // Get brick cell coordinates
    vec2 brickCell = floor(brickUV);
    vec2 brickFract = fract(brickUV);

    // Calculate mortar (edges of each brick)
    float mortarX = mortarWidth / brickWidth;
    float mortarY = mortarWidth / brickHeight;

    float brickMask = 1.0;
    if (brickFract.x < mortarX || brickFract.x > (1.0 - mortarX) ||
        brickFract.y < mortarY || brickFract.y > (1.0 - mortarY)) {
        brickMask = 0.0;
    }

    // Generate unique ID for each brick for color variation
    float brickID = hash(brickCell);

    return vec2(brickMask, brickID);
}

void main() {
    // Brick color palette
    vec3 darkBrick = vec3(0.5, 0.2, 0.15);    // Dark red-brown
    vec3 midBrick = vec3(0.65, 0.25, 0.15);   // Medium brick red
    vec3 lightBrick = vec3(0.75, 0.35, 0.2);  // Light brick
    vec3 mortarColor = vec3(0.7, 0.68, 0.65); // Light gray mortar

    // Use world position for consistent texturing
    // Project onto the most visible face
    vec2 uv;
    if (abs(fragNormal.x) > 0.5) {
        uv = fragWorldPos.zy;
    } else if (abs(fragNormal.z) > 0.5) {
        uv = fragWorldPos.xy;
    } else {
        uv = fragWorldPos.xz;
    }

    // Brick parameters
    float brickWidth = 0.4;
    float brickHeight = 0.2;
    float mortarWidth = 0.02;

    // Get brick pattern
    vec2 brickData = brick(uv, brickWidth, brickHeight, mortarWidth);
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

    // Add noise variation to each brick
    float brickNoise = fbm(uv * 15.0 + brickID * 100.0) * 0.15;
    brickColor += vec3(brickNoise * 0.5, brickNoise * 0.3, brickNoise * 0.2);

    // Add some wear/aging
    float wear = noise(uv * 30.0) * 0.1;
    brickColor -= vec3(wear);

    // Mortar variation
    float mortarNoise = noise(uv * 50.0) * 0.08;
    vec3 finalMortar = mortarColor + vec3(mortarNoise);

    // Blend brick and mortar
    vec3 surfaceColor = mix(finalMortar, brickColor, brickMask);

    // Simple lighting
    float light = max(dot(fragNormal, normalize(vec3(0.3, 1.0, 0.5))), 0.35);
    surfaceColor *= light;

    finalColor = vec4(surfaceColor, 1.0);
}
