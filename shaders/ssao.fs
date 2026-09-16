#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0; // Sampleable scene depth, with the same UVs as scene color.
uniform sampler2D noiseTexture;
uniform vec3 samples[32];
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform vec2 screenSize;
uniform float radius;
uniform float bias;
out vec4 finalColor;

vec3 viewPosition(vec2 uv) {
    float depth = texture(texture0, uv).r;
    vec4 p = inverseProjection * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    return p.xyz / p.w;
}

void main() {
    if (texture(texture0, fragTexCoord).r >= 0.9999999) {
        finalColor = vec4(1.0);
        return;
    }
    vec3 position = viewPosition(fragTexCoord);
    vec2 texel = 1.0 / screenSize;
    // Choose the closer depth neighbour on each axis to avoid silhouette halos.
    vec3 left = position - viewPosition(fragTexCoord - vec2(texel.x, 0));
    vec3 right = viewPosition(fragTexCoord + vec2(texel.x, 0)) - position;
    vec3 down = position - viewPosition(fragTexCoord - vec2(0, texel.y));
    vec3 up = viewPosition(fragTexCoord + vec2(0, texel.y)) - position;
    vec3 dx = abs(left.z) < abs(right.z) ? left : right;
    vec3 dy = abs(down.z) < abs(up.z) ? down : up;
    vec3 crossNormal = cross(dx, dy);
    if (dot(crossNormal, crossNormal) < 1e-16) {
        finalColor = vec4(1.0);
        return;
    }
    vec3 normal = normalize(crossNormal);
    if (dot(normal, -position) < 0.0) normal = -normal;
    vec3 randomVector = texture(noiseTexture, fragTexCoord * screenSize / 4.0).xyz * 2.0 - 1.0;
    randomVector.z = 0.0;
    vec3 tangent = randomVector - normal * dot(randomVector, normal);
    if (dot(tangent, tangent) < 1e-6) {
        tangent = cross(normal, abs(normal.y) < 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0));
    }
    tangent = normalize(tangent);
    mat3 basis = mat3(tangent, cross(normal, tangent), normal);
    float occlusion = 0.0;
    for (int i = 0; i < 32; i++) {
        vec3 samplePosition = position + basis * samples[i] * radius;
        vec4 clip = projection * vec4(samplePosition, 1.0);
        if (clip.w <= 0.0) continue;
        vec2 uv = clip.xy / clip.w * 0.5 + 0.5;
        if (any(lessThan(uv, vec2(0))) || any(greaterThan(uv, vec2(1)))) continue;
        if (texture(texture0, uv).r >= 0.9999999) continue;
        float sampleZ = viewPosition(uv).z;
        float rangeWeight = smoothstep(0.0, 1.0, radius / max(abs(position.z - sampleZ), 0.0001));
        occlusion += (sampleZ >= samplePosition.z + bias ? 1.0 : 0.0) * rangeWeight;
    }
    // Contact definition without crushing the colorful ambient palette.
    float ao = 1.0 - 0.35 * occlusion / 32.0;
    finalColor = vec4(vec3(ao), 1.0);
}
