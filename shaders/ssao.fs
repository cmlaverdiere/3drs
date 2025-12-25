#version 330

// Screen-Space Ambient Occlusion
// Samples depth buffer in a hemisphere around each pixel

in vec2 fragTexCoord;

uniform sampler2D depthTexture;    // Scene depth buffer
uniform sampler2D noiseTexture;    // 4x4 rotation noise
uniform vec3 samples[32];          // Hemisphere sample kernel
uniform mat4 projection;           // Camera projection matrix
uniform vec2 screenSize;           // Screen dimensions
uniform float radius;              // Sample radius in world units
uniform float bias;                // Depth bias to prevent self-occlusion
uniform float near;                // Camera near plane
uniform float far;                 // Camera far plane

out vec4 finalColor;

// Convert depth buffer value to linear depth
float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;  // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

// Reconstruct view-space position from depth
vec3 getViewPos(vec2 uv) {
    float depth = texture(depthTexture, uv).r;
    float linearDepth = linearizeDepth(depth);

    // Convert UV to clip space (-1 to 1)
    vec2 clipXY = uv * 2.0 - 1.0;

    // Reconstruct view-space position
    // Using projection matrix inverse approximation
    float aspect = screenSize.x / screenSize.y;
    float tanHalfFov = 1.0 / projection[1][1];

    vec3 viewPos;
    viewPos.x = clipXY.x * tanHalfFov * aspect * linearDepth;
    viewPos.y = clipXY.y * tanHalfFov * linearDepth;
    viewPos.z = -linearDepth;

    return viewPos;
}

// Reconstruct normal from depth derivatives
vec3 getNormal(vec2 uv) {
    vec2 texelSize = 1.0 / screenSize;

    vec3 posCenter = getViewPos(uv);
    vec3 posRight = getViewPos(uv + vec2(texelSize.x, 0.0));
    vec3 posUp = getViewPos(uv + vec2(0.0, texelSize.y));

    vec3 normal = normalize(cross(posRight - posCenter, posUp - posCenter));
    return normal;
}

void main() {
    vec3 fragPos = getViewPos(fragTexCoord);
    vec3 normal = getNormal(fragTexCoord);

    // Get noise vector for random rotation (tile the 4x4 texture)
    vec2 noiseScale = screenSize / 4.0;
    vec3 randomVec = texture(noiseTexture, fragTexCoord * noiseScale).xyz * 2.0 - 1.0;
    randomVec.z = 0.0;
    randomVec = normalize(randomVec);

    // Create TBN matrix to orient samples along surface normal
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // Sample occlusion
    float occlusion = 0.0;
    int sampleCount = 32;

    for (int i = 0; i < sampleCount; i++) {
        // Get sample position in view space
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;

        // Project sample to screen space
        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        // Get depth at sample position
        float sampleDepth = linearizeDepth(texture(depthTexture, offset.xy).r);

        // Range check and accumulate occlusion
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - (-sampleDepth)));
        occlusion += ((-sampleDepth) >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(sampleCount));

    // Limit darkening - remap from [0,1] to [0.4,1] so fully occluded is only 60% dark
    occlusion = 0.4 + occlusion * 0.6;

    finalColor = vec4(occlusion, occlusion, occlusion, 1.0);
}
