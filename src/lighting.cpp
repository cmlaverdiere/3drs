#include "lighting.h"
#include "atmosphere.h"
#include "shader_utils.h"
#include "types.h"
#include "external/glad.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstddef>

static const float kSunIlluminance = 9.0f;          // relative units; exposure maps to display
static const Vector3 kSunTint = {1.0f, 0.955f, 0.90f};
static const float kMoonIlluminance = 0.075f;
static const Vector3 kMoonTint = {0.62f, 0.78f, 1.0f};
static const Vector3 kNightAirglow = {0.0020f, 0.0028f, 0.0052f};
static const float kLampIntensity = 2.6f;
static const float kCampfireIntensity = 4.5f;
static const float kSkyBoost = 2.4f;   // multiple-scattering lift, matches sky_lut.fs

// ---------------------------------------------------------------------------
// Sun path: elevation keyframes (Catmull-Rom, periodic) and a piecewise-linear
// azimuth that sweeps east -> south -> west by day and under the north at night.
// Keeps the old phase timing: sunrise ~0.077, dusk preset 0.72 is golden hour,
// sunset ~0.80, midnight preset 0.95 is deep night.
// ---------------------------------------------------------------------------
static const float kElevTimes[] = {0.00f, 0.06f, 0.10f, 0.16f, 0.25f, 0.40f, 0.55f, 0.66f, 0.72f, 0.78f, 0.82f, 0.88f, 0.95f};
static const float kElevDegrees[] = {-20.0f, -3.0f, 4.0f, 14.0f, 34.0f, 60.0f, 46.0f, 24.0f, 9.0f, 2.0f, -4.0f, -16.0f, -24.0f};
static const int kElevKeys = sizeof(kElevTimes) / sizeof(kElevTimes[0]);
static const float kSunriseTime = 0.077f;
static const float kSunsetTime = 0.80f;

static float WrapTime(float t) { return t - floorf(t); }

static float SunElevationDegrees(float t) {
    t = WrapTime(t);
    auto key = [](int i, float* time, float* value) {
        int n = kElevKeys;
        int wrapped = ((i % n) + n) % n;
        *time = kElevTimes[wrapped] + floorf((float)i / n);
        *value = kElevDegrees[wrapped];
    };
    int seg = kElevKeys - 1;
    for (int i = 0; i < kElevKeys; i++) {
        if (t < kElevTimes[i]) { seg = i - 1; break; }
    }
    float t0, t1, tm, tp, p0, p1, pm, pp;
    key(seg, &t0, &p0);
    key(seg + 1, &t1, &p1);
    key(seg - 1, &tm, &pm);
    key(seg + 2, &tp, &pp);
    float h = t1 - t0;
    float s = (t - t0) / h;
    float m0 = (p1 - pm) / (t1 - tm);
    float m1 = (pp - p0) / (tp - t0);
    float s2 = s * s, s3 = s2 * s;
    return (2 * s3 - 3 * s2 + 1) * p0 + (s3 - 2 * s2 + s) * h * m0 +
           (-2 * s3 + 3 * s2) * p1 + (s3 - s2) * h * m1;
}

static float SunAzimuthRadians(float t) {
    t = WrapTime(t);
    const float rise = -30.0f * DEG2RAD, set = 210.0f * DEG2RAD;
    if (t >= kSunriseTime && t <= kSunsetTime) {
        return rise + (set - rise) * (t - kSunriseTime) / (kSunsetTime - kSunriseTime);
    }
    float nightLength = 1.0f - kSunsetTime + kSunriseTime;
    float since = WrapTime(t - kSunsetTime);
    return set + (2.0f * PI + rise - set) * since / nightLength;
}

Vector3 SunDirectionAt(float timeOfDay) {
    float el = SunElevationDegrees(timeOfDay) * DEG2RAD;
    float az = SunAzimuthRadians(timeOfDay);
    return Vector3Normalize({cosf(el) * cosf(az), sinf(el), cosf(el) * sinf(az)});
}

static float SmoothStep(float a, float b, float x) {
    float t = Clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float Luminance(Vector3 c) { return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z; }

// ---------------------------------------------------------------------------
// Atmosphere-derived lighting: sun/moon colour at the ground, sky irradiance SH
// ---------------------------------------------------------------------------
static void UpdateAtmosphereLighting(LightingSystem* lighting) {
    using namespace atmosphere;
    Vector3 origin = ViewerOrigin();
    Vector3 sunTop = Vector3Scale(kSunTint, kSunIlluminance);
    Vector3 moonTop = Vector3Scale(kMoonTint, kMoonIlluminance * lighting->moonVisibility);
    lighting->sunIlluminance = sunTop;
    lighting->moonIlluminance = moonTop;

    Vector3 sunGround = Vector3Scale(Mul(sunTop, TransmittanceToSpace(origin, lighting->sunDir, 24)),
                                     lighting->sunVisibility);
    Vector3 moonGround = Mul(moonTop, TransmittanceToSpace(origin, lighting->moonDir, 24));
    if (lighting->moonDir.y < 0.0f) moonGround = {0, 0, 0};

    float sunElevation = asinf(Clamp(lighting->sunDir.y, -1.0f, 1.0f)) * RAD2DEG;
    lighting->keyIsMoon = sunElevation < -1.5f;
    lighting->lightDir = lighting->keyIsMoon ? lighting->moonDir : lighting->sunDir;
    lighting->lightColor = Vector3Scale(lighting->keyIsMoon ? moonGround : sunGround, 1.0f / kPi);

    // Sky radiance over the upper hemisphere (Fibonacci directions)
    const int N = 96;
    Vector3 dirs[N], radiance[N];
    Vector3 skyDown = {0, 0, 0};  // irradiance onto the ground from the sky
    for (int i = 0; i < N; i++) {
        float y = 1.0f - (i + 0.5f) / N * 2.0f;
        float r = sqrtf(fmaxf(0.0f, 1.0f - y * y));
        float phi = i * 2.39996323f;
        dirs[i] = {cosf(phi) * r, y, sinf(phi) * r};
        if (y > 0.0f) {
            Vector3 dir = {dirs[i].x, fmaxf(y, 0.02f), dirs[i].z};
            dir = Vector3Normalize(dir);
            float horizon = 1.0f - dir.y;
            radiance[i] = Vector3Scale(Vector3Add(SkyRadiance(dir, lighting->sunDir, sunTop, 12),
                                                  SkyRadiance(dir, lighting->moonDir, moonTop, 12)),
                                       kSkyBoost * (1.0f + 0.3f * horizon * horizon));
            skyDown = Vector3Add(skyDown, Vector3Scale(radiance[i], y * 4.0f * kPi / N));
        }
    }
    // Lower hemisphere: light bounced off the ground (season-dependent albedo)
    Vector3 albedo = {0.16f, 0.22f, 0.10f};
    if (g_currentSeason == SEASON_WINTER) albedo = {0.62f, 0.66f, 0.72f};
    else if (g_currentSeason == SEASON_AUTUMN) albedo = {0.24f, 0.19f, 0.09f};
    Vector3 groundIrradiance = Vector3Add(skyDown,
        Vector3Scale(lighting->keyIsMoon ? moonGround : sunGround, fmaxf(lighting->lightDir.y, 0.0f)));
    // Half strength: nearby ground is partly shadowed and grass is not a flat mirror
    Vector3 bounce = Vector3Scale(Mul(albedo, groundIrradiance), 0.5f / kPi);

    Vector3 L00 = {0, 0, 0}, L1x = {0, 0, 0}, L1y = {0, 0, 0}, L1z = {0, 0, 0};
    const float w = 4.0f * kPi / N;
    for (int i = 0; i < N; i++) {
        Vector3 L = dirs[i].y > 0.0f ? radiance[i] : bounce;
        L00 = Vector3Add(L00, Vector3Scale(L, 0.282095f * w));
        L1x = Vector3Add(L1x, Vector3Scale(L, 0.488603f * dirs[i].x * w));
        L1y = Vector3Add(L1y, Vector3Scale(L, 0.488603f * dirs[i].y * w));
        L1z = Vector3Add(L1z, Vector3Scale(L, 0.488603f * dirs[i].z * w));
    }
    const float a1 = 2.0f / 3.0f * 0.488603f;
    lighting->ambientSH[0] = Vector3Add(Vector3Scale(L00, 0.282095f), kNightAirglow);
    lighting->ambientSH[1] = Vector3Scale(L1x, a1);
    lighting->ambientSH[2] = Vector3Scale(L1y, a1);
    lighting->ambientSH[3] = Vector3Scale(L1z, a1);

    Vector3 horizon = {0, 0, 0};
    for (int i = 0; i < 4; i++) {
        float a = i * PI * 0.5f;
        Vector3 dir = Vector3Normalize({cosf(a), 0.03f, sinf(a)});
        horizon = Vector3Add(horizon, SkyRadiance(dir, lighting->sunDir, sunTop, 12));
    }
    lighting->fogColor = Vector3Scale(horizon, 0.25f * kSkyBoost * 1.3f);

    // Exposure: partial adaptation keeps night dark while staying readable
    Vector3 ambientUp = Vector3Add(lighting->ambientSH[0], lighting->ambientSH[2]);
    float Y = Luminance(lighting->lightColor) * 0.6f + Luminance(ambientUp);
    const float Yref = 2.2f;
    Y = fmaxf(Y, 1e-5f);
    lighting->exposure = (1.0f / Y) * powf(fminf(Y / Yref, 1.0f), 0.55f) * 0.95f;
    lighting->nightFactor = 1.0f - SmoothStep(-8.0f, 2.0f, sunElevation);
}

static void UpdateCelestial(LightingSystem* lighting) {
    lighting->sunDir = SunDirectionAt(lighting->timeOfDay);
    lighting->moonDir = Vector3Normalize(Vector3Add(Vector3Negate(lighting->sunDir), {0.0f, 0.25f, 0.0f}));
    float sunElevation = asinf(Clamp(lighting->sunDir.y, -1.0f, 1.0f)) * RAD2DEG;
    lighting->sunVisibility = SmoothStep(-1.2f, 0.8f, sunElevation);
    lighting->moonVisibility = 1.0f - SmoothStep(-9.0f, -2.5f, sunElevation);
}

void InitLightingSystem(LightingSystem* lighting) {
    lighting->timeOfDay = 0.5f;
    lighting->cyclePaused = false;
    lighting->lampsOn = false;
    lighting->fogDensity = 0.0019f;
    lighting->lastAtmosphereTime = -1.0f;
    lighting->lastSeason = -1;
    lighting->lampCount = 0;
    lighting->campfireCount = 0;

    lighting->shadowAtlas = CreateGpuTarget(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, {}, true);
    if (lighting->shadowAtlas.fbo) {
        SetDepthCompare(lighting->shadowAtlas.depth, true);
    } else {
        TraceLog(LOG_WARNING, "LIGHTING: Shadows disabled (framebuffer unavailable)");
    }

    lighting->frameUbo = CreateUniformBuffer(sizeof(FrameUniforms), 0);
    memset(&lighting->frame, 0, sizeof(lighting->frame));

    lighting->skyLut = CreateGpuTarget(256, 128, {GpuFormat::RGBA16F}, false, true);
    if (lighting->skyLut.fbo) {
        // Azimuth wraps around
        glBindTexture(GL_TEXTURE_2D, lighting->skyLut.color[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    lighting->skyLutShader = RegisterSceneShader(lighting,
        LoadShaderWithIncludes("shaders/fullscreen_tri.vs", "shaders/sky_lut.fs"));

    lighting->lightCamera.projection = CAMERA_ORTHOGRAPHIC;
    lighting->lightCamera.up = (Vector3){0.0f, 1.0f, 0.0f};
    lighting->lightCamera.fovy = SHADOW_ORTHO_SIZE;
    lighting->lightCamera.position = (Vector3){0.0f, 100.0f, 0.0f};
    lighting->lightCamera.target = (Vector3){0.0f, 0.0f, 0.0f};

    UpdateCelestial(lighting);
    UpdateAtmosphereLighting(lighting);
}

void UpdateLightingSystem(LightingSystem* lighting, float dt, Vector3 playerPos) {
    (void)playerPos;
    if (!lighting->cyclePaused) {
        lighting->timeOfDay += dt / DAY_CYCLE_DURATION;
        if (lighting->timeOfDay >= 1.0f) lighting->timeOfDay -= 1.0f;
    }
    lighting->lampsOn = AreLampsOn(lighting->timeOfDay);
    UpdateCelestial(lighting);
    // The atmosphere integration is cheap but only needs refreshing as the sun
    // moves (or the season changes the ground bounce)
    if (fabsf(lighting->timeOfDay - lighting->lastAtmosphereTime) > 0.00015f ||
        lighting->lastSeason != (int)g_currentSeason) {
        UpdateAtmosphereLighting(lighting);
        lighting->lastAtmosphereTime = lighting->timeOfDay;
        lighting->lastSeason = (int)g_currentSeason;
    }
}

Shader RegisterSceneShader(LightingSystem* lighting, Shader shader) {
    (void)lighting;
    BindUniformBlock(shader, "FrameData", 0);
    SetSamplerUnit(shader, "uShadowMap", TEX_UNIT_SHADOW);
    SetSamplerUnit(shader, "uSkyLUT", TEX_UNIT_SKY);
    SetSamplerUnit(shader, "uSceneColor", TEX_UNIT_SCENE_COLOR);
    SetSamplerUnit(shader, "uSceneDepth", TEX_UNIT_SCENE_DEPTH);
    return shader;
}

static void SetVec4(float* dst, Vector3 v, float w) { dst[0] = v.x; dst[1] = v.y; dst[2] = v.z; dst[3] = w; }

static float Flicker(float time, float seed) {
    return 0.86f + 0.08f * sinf(time * 9.3f + seed) + 0.05f * sinf(time * 23.1f + seed * 3.7f) +
           0.03f * sinf(time * 41.7f + seed * 1.3f);
}

void PrepareFrame(LightingSystem* lighting, const Camera3D& camera, int renderWidth, int renderHeight) {
    FrameUniforms& f = lighting->frame;
    float aspect = (float)renderWidth / (float)renderHeight;
    float nearPlane = (float)rlGetCullDistanceNear(), farPlane = (float)rlGetCullDistanceFar();
    f.view = MatrixLookAt(camera.position, camera.target, camera.up);
    f.projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, nearPlane, farPlane);
    f.viewProjection = MatrixMultiply(f.view, f.projection);
    f.invViewProjection = MatrixInvert(f.viewProjection);
    float time = (float)GetTime();

    SetVec4(f.camera, camera.position, time);
    SetVec4(f.lightDir, lighting->lightDir, lighting->keyIsMoon ? 1.0f : 0.0f);
    SetVec4(f.lightColor, lighting->lightColor, 1.0f);
    SetVec4(f.sunDir, lighting->sunDir, lighting->sunVisibility);
    SetVec4(f.sunColor, lighting->sunIlluminance, 0.0f);
    SetVec4(f.moonDir, lighting->moonDir, lighting->moonVisibility);
    SetVec4(f.moonColor, lighting->moonIlluminance, 0.0f);
    for (int i = 0; i < 4; i++) SetVec4(f.ambientSH[i], lighting->ambientSH[i], 0.0f);
    f.fog[0] = lighting->fogDensity; f.fog[1] = 0.035f; f.fog[2] = 0.0f; f.fog[3] = 1.0f;
    f.clouds[0] = 0.4f; f.clouds[1] = 1400.0f; f.clouds[2] = time * 6.0f; f.clouds[3] = time * 2.5f;
    Vector3 wind = Vector3Normalize({0.8f, 0.0f, 0.45f});
    f.wind[0] = wind.x; f.wind[1] = wind.z; f.wind[2] = 1.0f; f.wind[3] = 0.5f + 0.5f * sinf(time * 0.21f);
    f.season[0] = (float)g_currentSeason;
    f.season[1] = g_currentSeason == SEASON_WINTER ? 1.0f : 0.0f;
    f.season[2] = lighting->timeOfDay;
    f.season[3] = lighting->nightFactor;
    f.screen[0] = (float)renderWidth; f.screen[1] = (float)renderHeight;
    f.screen[2] = 1.0f / renderWidth; f.screen[3] = 1.0f / renderHeight;
    f.exposure[0] = lighting->exposure; f.exposure[1] = lighting->nightFactor;

    // Cascades: bounding spheres of view-frustum slices, texel-snapped
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    float tanHalf = tanf(camera.fovy * DEG2RAD * 0.5f);
    Vector3 travel = Vector3Negate(lighting->lightDir);
    lighting->shadowsEnabled = lighting->shadowAtlas.fbo != 0 && Luminance(lighting->lightColor) > 1e-5f;
    float sliceNear = nearPlane;
    for (int c = 0; c < SHADOW_CASCADES; c++) {
        Vector3 center; float radius;
        FrustumSliceSphere(camera.position, forward, tanHalf, aspect, sliceNear, SHADOW_CASCADE_SPLITS[c],
                           &center, &radius);
        radius = ceilf(radius * 4.0f) / 4.0f;
        lighting->cascades[c] = CalculateShadowMatrices(center, travel, radius * 2.0f, SHADOW_TILE_SIZE);
        lighting->cascadeTexel[c] = radius * 2.0f / SHADOW_TILE_SIZE;
        f.shadowMatrix[c] = lighting->cascades[c].viewProjection;
        f.shadowSplits[c] = SHADOW_CASCADE_SPLITS[c];
        f.shadowTexel[c] = lighting->cascadeTexel[c];
        sliceNear = SHADOW_CASCADE_SPLITS[c] * 0.92f;
    }
    f.shadowParams[0] = lighting->shadowsEnabled ? 1.0f : 0.0f;
    f.shadowParams[1] = (float)SHADOW_CASCADES;
    f.shadowParams[2] = SHADOW_CASCADE_SPLITS[SHADOW_CASCADES - 1] * 0.8f;
    f.shadowParams[3] = SHADOW_CASCADE_SPLITS[SHADOW_CASCADES - 1];

    // Nearest point lights (campfires always, lamps when lit)
    struct NearbyLight { Vector3 position; Vector3 color; float distance; float radius; };
    std::vector<NearbyLight> candidates;
    candidates.reserve(lighting->campfireCount + lighting->lampCount);
    for (int i = 0; i < lighting->campfireCount; i++) {
        Vector3 color = Vector3Scale(lighting->campfireColors[i], Flicker(time, (float)i * 17.3f));
        candidates.push_back({lighting->campfirePositions[i], color,
                              Vector3DistanceSqr(camera.position, lighting->campfirePositions[i]), 16.0f});
    }
    if (lighting->lampsOn) {
        for (int i = 0; i < lighting->lampCount; i++) {
            candidates.push_back({lighting->lampPositions[i], lighting->lampColors[i],
                                  Vector3DistanceSqr(camera.position, lighting->lampPositions[i]), 13.0f});
        }
    }
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const NearbyLight& a, const NearbyLight& b) { return a.distance < b.distance; });
    int count = std::min((int)candidates.size(), MAX_POINT_LIGHTS);
    for (int i = 0; i < count; i++) {
        SetVec4(f.pointPos[i], candidates[i].position, candidates[i].radius);
        SetVec4(f.pointColor[i], candidates[i].color, 0.0f);
    }
    f.counts[0] = (float)count;

    UpdateUniformBuffer(lighting->frameUbo, &f, sizeof(f));
}

void RenderSkyLut(LightingSystem* lighting) {
    if (!lighting->skyLut.fbo) return;
    BeginFullscreenPass(lighting->skyLutShader, &lighting->skyLut);
    DrawFullscreenTriangle();
    EndFullscreenPass();
}

bool BeginShadowPass(LightingSystem* lighting) {
    if (!lighting->shadowsEnabled) return false;
    rlDrawRenderBatchActive();
    BeginTextureMode(AsRenderTexture(lighting->shadowAtlas));
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    BeginMode3D(lighting->lightCamera);
    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_BACK);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.1f, 1.0f);
    return true;
}

void BeginShadowCascade(LightingSystem* lighting, int cascade) {
    rlDrawRenderBatchActive();
    int tx = cascade % 2, ty = cascade / 2;
    SetViewportRect(tx * SHADOW_TILE_SIZE, ty * SHADOW_TILE_SIZE, SHADOW_TILE_SIZE, SHADOW_TILE_SIZE);
    rlSetMatrixProjection(lighting->cascades[cascade].projection);
    rlSetMatrixModelview(lighting->cascades[cascade].view);
}

bool IsShadowCasterVisible(const LightingSystem* lighting, int cascade, Vector3 center, float radius) {
    Vector3 clip = Vector3Transform(center, lighting->cascades[cascade].viewProjection);
    float xyRadius = radius / (lighting->cascadeTexel[cascade] * SHADOW_TILE_SIZE * 0.5f);
    float zRadius = radius * 2.0f / (SHADOW_FAR - SHADOW_NEAR);
    return fabsf(clip.x) <= 1.0f + xyRadius && fabsf(clip.y) <= 1.0f + xyRadius &&
           fabsf(clip.z) <= 1.0f + zRadius;
}

void EndShadowPass(LightingSystem* lighting) {
    (void)lighting;
    rlDrawRenderBatchActive();
    glDisable(GL_POLYGON_OFFSET_FILL);
    EndMode3D();
    rlSetCullFace(RL_CULL_FACE_BACK);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
    EndTextureMode();
}

void BindGlobalLightingTextures(const LightingSystem* lighting) {
    rlDrawRenderBatchActive();
    BindTextureUnit(TEX_UNIT_SHADOW, lighting->shadowAtlas.depth);
    BindTextureUnit(TEX_UNIT_SKY, lighting->skyLut.color[0]);
}

void UnloadLightingSystem(LightingSystem* lighting) {
    DestroyGpuTarget(&lighting->shadowAtlas);
    DestroyGpuTarget(&lighting->skyLut);
    if (lighting->skyLutShader.id) UnloadShader(lighting->skyLutShader);
    if (lighting->frameUbo) glDeleteBuffers(1, &lighting->frameUbo);
    lighting->frameUbo = 0;
}

TimeOfDay GetTimeOfDayPhase(float timeOfDay) {
    if (timeOfDay < 0.15f) return TIME_DAWN;
    if (timeOfDay < 0.65f) return TIME_DAY;
    if (timeOfDay < 0.8f) return TIME_DUSK;
    return TIME_NIGHT;
}

bool AreLampsOn(float timeOfDay) {
    // Lamps on from dusk through night to dawn
    return (timeOfDay >= LAMP_ON_TIME || timeOfDay < LAMP_OFF_TIME);
}

void SetLampPositions(LightingSystem* lighting, const Vector3* positions, int count) {
    lighting->lampCount = std::max(0, count);
    lighting->lampPositions.resize(lighting->lampCount);
    lighting->lampColors.resize(lighting->lampCount);
    Vector3 lampColor = Vector3Scale({1.0f, 0.74f, 0.44f}, kLampIntensity);
    for (int i = 0; i < lighting->lampCount; i++) {
        // Light sits inside the lamp housing at the top of the post
        lighting->lampPositions[i] = (Vector3){positions[i].x, positions[i].y + 2.3f, positions[i].z};
        lighting->lampColors[i] = lampColor;
    }
    TraceLog(LOG_INFO, "Set %d lamp positions for point lighting", lighting->lampCount);
}

void SetCampfirePositions(LightingSystem* lighting, const Vector3* positions, int count) {
    lighting->campfireCount = std::max(0, count);
    lighting->campfirePositions.resize(lighting->campfireCount);
    lighting->campfireColors.resize(lighting->campfireCount);
    Vector3 campfireColor = Vector3Scale({1.0f, 0.42f, 0.12f}, kCampfireIntensity);
    for (int i = 0; i < lighting->campfireCount; i++) {
        lighting->campfirePositions[i] = (Vector3){positions[i].x, positions[i].y + 0.7f, positions[i].z};
        lighting->campfireColors[i] = campfireColor;
    }
    TraceLog(LOG_INFO, "Set %d campfire positions for point lighting", lighting->campfireCount);
}
