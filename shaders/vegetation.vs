#version 330

// Instanced vegetation. One source, compiled per kind:
//   CARDS  leaf cards billboarded around a canopy blob (to the camera, or to
//          the light in the SHADOW pass so shadows stay put as the view turns)
//   CORE   solid canopy core     TRUNK  tapered, flared, wind-bent trunk
//   PINE   tiered evergreen      ROCK   boulder
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec4 instA;
in vec4 instB;
in vec4 instC;

uniform mat4 mvp;

#include "common/frame.glsl"

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec3 vColor;
out float vAO;
out vec3 vLocal;
flat out float vSeed;
flat out vec4 vExtra;

vec3 windSway(vec3 anchor, float seed) {
    float t = uTime;
    float gust = 0.55 + 0.45 * sin(t * 0.63 + anchor.x * 0.021 + anchor.z * 0.017);
    float sway = sin(t * 1.3 + seed * 6.283 + anchor.x * 0.13) * 0.65 + sin(t * 2.17 + seed * 3.1) * 0.35;
    return vec3(uWind.x, 0.0, uWind.y) * sway * gust * uWind.z;
}

void main() {
    vec3 world;
#if defined(CARDS) || defined(CORE)
    vec3 center = instA.xyz;
    float radius = instA.w;
    vColor = instB.rgb;
    vSeed = instB.w;
    vec3 wind = windSway(instC.xyz, vSeed) * 0.05 * (center.y - instC.y);
  #ifdef CARDS
    vec3 cardCenter = center + vertexPosition * radius + wind;
    float flutter = sin(uTime * 3.7 + vertexColor.b * 40.0 + vSeed * 9.0) * 0.06;
    float size = radius * (0.34 + 0.16 * vertexColor.r);
    float rot = vertexColor.g * 6.2831853 + flutter;
    vec2 corner = vertexTexCoord;
    vec2 rc = vec2(cos(rot) * corner.x - sin(rot) * corner.y, sin(rot) * corner.x + cos(rot) * corner.y);
    #ifdef SHADOW
    vec3 fwd = -uLightDir.xyz;
    #else
    vec3 fwd = normalize(cardCenter - uCamera.xyz);
    #endif
    vec3 right = normalize(cross(fwd, abs(fwd.y) > 0.99 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0)));
    vec3 up = cross(right, fwd);
    vec3 offset = (right * rc.x + up * rc.y) * size;
    world = cardCenter + offset;
    // Spherical normals give the canopy a soft, volumetric shading
    vNormal = normalize(vertexNormal + offset / radius * 0.9);
    vUV = corner;
    vExtra = vertexColor;
  #else
    world = center + vertexPosition * radius * instC.w + wind;
    vNormal = vertexNormal;
    vUV = vec2(0.0);
    vExtra = vec4(0.5);
  #endif
    float h = (world.y - center.y) / radius;
    vAO = mix(0.42, 1.0, smoothstep(-0.95, 0.75, h));
    vLocal = (world - center) / radius;
#elif defined(TRUNK)
    vec3 base = instA.xyz;
    float height = instA.w;
    float rBottom = instB.x, rTop = instB.y;
    vSeed = instB.z;
    vColor = instC.rgb;
    float y = vertexPosition.y;
    float yh = y * height;
    float angle = atan(vertexPosition.z, vertexPosition.x);
    float r = mix(rBottom, rTop, y) * (1.0 + 0.75 * exp(-yh / 0.2));
    r *= 1.0 + 0.08 * sin(angle * 5.0 + vSeed * 10.0 + yh * 1.7) * (1.0 - y * 0.5);
    vec3 lean = vec3(sin(vSeed * 7.0), 0.0, cos(vSeed * 5.0)) * 0.12 * y * y * height * 0.25;
    world = base + vec3(vertexPosition.x * r, yh, vertexPosition.z * r) + lean +
            windSway(base, vSeed) * 0.02 * y * y * height;
    vNormal = normalize(vec3(vertexNormal.x, (rBottom - rTop) / height, vertexNormal.z));
    // Unit direction around the trunk (interpolates without the atan seam)
    vUV = vec2(0.0, yh);
    vLocal = vec3(vertexPosition.x, yh, vertexPosition.z);
    vAO = mix(0.5, 1.0, smoothstep(0.0, 0.9, yh));
    vExtra = vec4(0.0);
#elif defined(PINE)
    vec3 base = instA.xyz;
    float height = instA.w;
    float radius = instB.x;
    vSeed = instB.y;
    vColor = instC.rgb;
    vec3 local = vertexPosition;
    world = base + vec3(local.x * radius, local.y * height, local.z * radius) +
            windSway(base, vSeed) * 0.05 * local.y * local.y * height;
    vNormal = normalize(vec3(vertexNormal.x / radius, vertexNormal.y / height, vertexNormal.z / radius));
    vLocal = local;
    vAO = vertexColor.r;
    vUV = vec2(0.0);
    vExtra = vertexColor;
#elif defined(ROCK)
    vec3 pos = instA.xyz;
    float scale = instA.w;
    vColor = instB.rgb;
    vSeed = instB.w;
    float c = cos(instC.x), s = sin(instC.x);
    vec3 p = vertexPosition;
    p.xz = vec2(c * p.x - s * p.z, s * p.x + c * p.z);
    vec3 n = vertexNormal;
    n.xz = vec2(c * n.x - s * n.z, s * n.x + c * n.z);
    world = pos + p * scale;
    vNormal = n;
    vLocal = vertexPosition + vSeed * 13.0;
    vUV = vec2(0.0);
    vAO = mix(0.55, 1.0, smoothstep(0.0, 0.6, vertexPosition.y));
    vExtra = instC;
#endif
    vWorldPos = world;
    gl_Position = mvp * vec4(world, 1.0);
}
