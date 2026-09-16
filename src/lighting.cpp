#include "lighting.h"
#include "external/glad.h"
#include <cmath>
#include <algorithm>
#include <random>

// Color presets for different times
struct TimeColors {
    Vector3 sunColor;
    Vector3 ambientColor;
    Vector3 fogColor;
    Color skyColor;
    float sunElevation;  // Degrees above horizon
};

static const TimeColors DAWN_COLORS = {
    {1.0f, 0.6f, 0.3f},      // Sun: Orange sunrise
    {0.25f, 0.2f, 0.3f},     // Ambient: Purple tint
    {0.8f, 0.6f, 0.5f},      // Fog: Warm
    {230, 150, 130, 255},    // Sky: Pink
    15.0f                     // Low sun
};

static const TimeColors DAY_COLORS = {
    {1.0f, 0.95f, 0.9f},     // Sun: Bright white-yellow
    {0.4f, 0.45f, 0.5f},     // Ambient: Blue sky bounce
    {0.7f, 0.8f, 0.9f},      // Fog: Light blue
    {100, 180, 255, 255},    // Sky: Blue
    60.0f                     // High sun
};

static const TimeColors DUSK_COLORS = {
    {1.0f, 0.4f, 0.2f},      // Sun: Deep orange
    {0.3f, 0.15f, 0.2f},     // Ambient: Purple
    {0.6f, 0.4f, 0.5f},      // Fog: Purple
    {200, 100, 80, 255},     // Sky: Orange
    10.0f                     // Low sun
};

static const TimeColors NIGHT_COLORS = {
    {0.15f, 0.15f, 0.25f},   // Sun: Dim moonlight
    {0.08f, 0.08f, 0.15f},   // Ambient: Very dark blue
    {0.1f, 0.1f, 0.15f},     // Fog: Dark
    {15, 15, 40, 255},       // Sky: Dark blue
    -20.0f                    // Below horizon (moon)
};

// Linear interpolation for Vector3
static Vector3 LerpVector3(Vector3 a, Vector3 b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

// Linear interpolation for Color
static Color LerpColor(Color a, Color b, float t) {
    return {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        255
    };
}

// Interpolate time colors based on current time
static TimeColors InterpolateTimeColors(float timeOfDay) {
    TimeColors result;

    if (timeOfDay < 0.15f) {
        // Dawn (0.0 - 0.15)
        float t = timeOfDay / 0.15f;
        result.sunColor = LerpVector3(NIGHT_COLORS.sunColor, DAWN_COLORS.sunColor, t);
        result.ambientColor = LerpVector3(NIGHT_COLORS.ambientColor, DAWN_COLORS.ambientColor, t);
        result.fogColor = LerpVector3(NIGHT_COLORS.fogColor, DAWN_COLORS.fogColor, t);
        result.skyColor = LerpColor(NIGHT_COLORS.skyColor, DAWN_COLORS.skyColor, t);
        result.sunElevation = NIGHT_COLORS.sunElevation + (DAWN_COLORS.sunElevation - NIGHT_COLORS.sunElevation) * t;
    } else if (timeOfDay < 0.35f) {
        // Dawn to Day (0.15 - 0.35)
        float t = (timeOfDay - 0.15f) / 0.2f;
        result.sunColor = LerpVector3(DAWN_COLORS.sunColor, DAY_COLORS.sunColor, t);
        result.ambientColor = LerpVector3(DAWN_COLORS.ambientColor, DAY_COLORS.ambientColor, t);
        result.fogColor = LerpVector3(DAWN_COLORS.fogColor, DAY_COLORS.fogColor, t);
        result.skyColor = LerpColor(DAWN_COLORS.skyColor, DAY_COLORS.skyColor, t);
        result.sunElevation = DAWN_COLORS.sunElevation + (DAY_COLORS.sunElevation - DAWN_COLORS.sunElevation) * t;
    } else if (timeOfDay < 0.65f) {
        // Day (0.35 - 0.65)
        result = DAY_COLORS;
    } else if (timeOfDay < 0.8f) {
        // Day to Dusk (0.65 - 0.8)
        float t = (timeOfDay - 0.65f) / 0.15f;
        result.sunColor = LerpVector3(DAY_COLORS.sunColor, DUSK_COLORS.sunColor, t);
        result.ambientColor = LerpVector3(DAY_COLORS.ambientColor, DUSK_COLORS.ambientColor, t);
        result.fogColor = LerpVector3(DAY_COLORS.fogColor, DUSK_COLORS.fogColor, t);
        result.skyColor = LerpColor(DAY_COLORS.skyColor, DUSK_COLORS.skyColor, t);
        result.sunElevation = DAY_COLORS.sunElevation + (DUSK_COLORS.sunElevation - DAY_COLORS.sunElevation) * t;
    } else if (timeOfDay < 0.9f) {
        // Dusk to Night (0.8 - 0.9)
        float t = (timeOfDay - 0.8f) / 0.1f;
        result.sunColor = LerpVector3(DUSK_COLORS.sunColor, NIGHT_COLORS.sunColor, t);
        result.ambientColor = LerpVector3(DUSK_COLORS.ambientColor, NIGHT_COLORS.ambientColor, t);
        result.fogColor = LerpVector3(DUSK_COLORS.fogColor, NIGHT_COLORS.fogColor, t);
        result.skyColor = LerpColor(DUSK_COLORS.skyColor, NIGHT_COLORS.skyColor, t);
        result.sunElevation = DUSK_COLORS.sunElevation + (NIGHT_COLORS.sunElevation - DUSK_COLORS.sunElevation) * t;
    } else {
        // Night (0.9 - 1.0)
        result = NIGHT_COLORS;
    }

    return result;
}

// Raylib's ordinary render target uses a renderbuffer, which cannot be sampled.
// Keep a color attachment for portable framebuffer completeness, but sample depth.
static RenderTexture2D LoadDepthTextureTarget(int width, int height) {
    RenderTexture2D target = {};
    target.id = rlLoadFramebuffer();
    if (!target.id) return target;
    target.texture = {rlLoadTexture(nullptr, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1),
                      width, height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    target.depth = {rlLoadTextureDepth(width, height, false), width, height, 1, 19};
    // Unsized GL_DEPTH_COMPONENT can resolve to 16 bits on macOS. Explicit
    // floating-point depth avoids quantized SSAO normals and shadow banding.
    glBindTexture(GL_TEXTURE_2D, target.depth.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    bool complete = rlFramebufferComplete(target.id);
    rlDisableFramebuffer();
    if (!complete || !target.texture.id || !target.depth.id) {
        TraceLog(LOG_WARNING, "LIGHTING: Incomplete depth framebuffer (%dx%d)", width, height);
        UnloadRenderTexture(target);
        return {};
    }
    SetTextureWrap(target.texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(target.depth, TEXTURE_WRAP_CLAMP);
    SetTextureFilter(target.depth, TEXTURE_FILTER_POINT);
    return target;
}

void InitLightingSystem(LightingSystem* lighting) {
    // Create shadow map
    lighting->shadowMap = LoadDepthTextureTarget(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);

    // Start at midday
    lighting->timeOfDay = 0.5f;
    lighting->cyclePaused = false;
    lighting->lightViewProj = MatrixIdentity();
    if (!lighting->shadowMap.id) TraceLog(LOG_WARNING, "LIGHTING: Shadows disabled (framebuffer unavailable)");

    // Initialize light camera
    lighting->lightCamera.projection = CAMERA_ORTHOGRAPHIC;
    lighting->lightCamera.up = (Vector3){0.0f, 1.0f, 0.0f};
    lighting->lightCamera.fovy = SHADOW_ORTHO_SIZE;

    // Initialize with day colors
    TimeColors colors = InterpolateTimeColors(lighting->timeOfDay);
    lighting->sunColor = colors.sunColor;
    lighting->ambientColor = colors.ambientColor;
    lighting->fogColor = colors.fogColor;
    lighting->fogDensity = 0.004f;  // Subtle fog

    // Initial sun direction
    lighting->sunDirection = Vector3Normalize((Vector3){0.3f, -0.8f, 0.5f});

    // Initialize point lights
    lighting->lampCount = 0;
    lighting->campfireCount = 0;
    lighting->lampsOn = false;
}

void UpdateLightingSystem(LightingSystem* lighting, float dt, Vector3 playerPos) {
    // Advance time
    if (!lighting->cyclePaused) {
        lighting->timeOfDay += dt / DAY_CYCLE_DURATION;
        if (lighting->timeOfDay >= 1.0f) lighting->timeOfDay -= 1.0f;
    }

    // Update lamp state based on time
    lighting->lampsOn = AreLampsOn(lighting->timeOfDay);

    // Interpolate colors based on time
    TimeColors colors = InterpolateTimeColors(lighting->timeOfDay);
    lighting->sunColor = colors.sunColor;
    lighting->ambientColor = colors.ambientColor;
    lighting->fogColor = colors.fogColor;

    // Calculate sun direction from elevation and rotation
    float sunAngle = lighting->timeOfDay * 2.0f * PI;  // Full rotation
    float elevation = colors.sunElevation * DEG2RAD;

    lighting->sunDirection = (Vector3){
        cosf(sunAngle) * cosf(elevation),
        -sinf(elevation),
        sinf(sunAngle) * cosf(elevation)
    };
    lighting->sunDirection = Vector3Normalize(lighting->sunDirection);
    float daylight = Clamp(-lighting->sunDirection.y / 0.12f, 0.0f, 1.0f);
    daylight = daylight * daylight * (3.0f - 2.0f * daylight);
    lighting->sunColor = Vector3Scale(colors.sunColor, daylight);

    // Update light camera to follow player (for shadow mapping)
    float shadowDistance = SHADOW_ORTHO_SIZE * 0.5f;
    lighting->lightCamera.position = Vector3Add(
        playerPos,
        Vector3Scale(lighting->sunDirection, -shadowDistance)
    );
    lighting->lightCamera.target = playerPos;
}

void CacheShaderLightingLocs(LightingSystem* lighting, Shader shader) {
    lighting->sunDirLoc = GetShaderLocation(shader, "sunDirection");
    lighting->sunColorLoc = GetShaderLocation(shader, "sunColor");
    lighting->ambientLoc = GetShaderLocation(shader, "ambientColor");
    lighting->fogColorLoc = GetShaderLocation(shader, "fogColor");
    lighting->fogDensityLoc = GetShaderLocation(shader, "fogDensity");
    lighting->lightVPLoc = GetShaderLocation(shader, "lightVP");
    lighting->shadowMapLoc = GetShaderLocation(shader, "shadowMap");
    lighting->viewPosLoc = GetShaderLocation(shader, "viewPos");
}

void SetShaderLightingUniforms(LightingSystem* lighting, Shader shader, Vector3 viewPos) {
    // Get uniform locations (could cache these per-shader for efficiency)
    int sunDirLoc = GetShaderLocation(shader, "sunDirection");
    int sunColorLoc = GetShaderLocation(shader, "sunColor");
    int ambientLoc = GetShaderLocation(shader, "ambientColor");
    int fogColorLoc = GetShaderLocation(shader, "fogColor");
    int fogDensityLoc = GetShaderLocation(shader, "fogDensity");
    int lightVPLoc = GetShaderLocation(shader, "lightVP");
    int viewPosLoc = GetShaderLocation(shader, "viewPos");

    // Set uniforms
    float sunDir[3] = {lighting->sunDirection.x, lighting->sunDirection.y, lighting->sunDirection.z};
    float sunCol[3] = {lighting->sunColor.x, lighting->sunColor.y, lighting->sunColor.z};
    float ambCol[3] = {lighting->ambientColor.x, lighting->ambientColor.y, lighting->ambientColor.z};
    float fogCol[3] = {lighting->fogColor.x, lighting->fogColor.y, lighting->fogColor.z};
    float viewP[3] = {viewPos.x, viewPos.y, viewPos.z};

    SetShaderValue(shader, sunDirLoc, sunDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, sunColorLoc, sunCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, ambientLoc, ambCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, fogColorLoc, fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, fogDensityLoc, &lighting->fogDensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, viewPosLoc, viewP, SHADER_UNIFORM_VEC3);

    // Set light view-projection matrix
    SetShaderValueMatrix(shader, lightVPLoc, lighting->lightViewProj);

    // Combine lamp and campfire lights into one array for shader
    int pointLightPosLoc = GetShaderLocation(shader, "pointLightPositions");
    int pointLightColLoc = GetShaderLocation(shader, "pointLightColors");
    int pointLightCountLoc = GetShaderLocation(shader, "pointLightCount");

    struct NearbyLight { Vector3 position; Vector3 color; float distance; };
    std::vector<NearbyLight> candidates;
    candidates.reserve(lighting->campfireCount + lighting->lampCount);
    for (int i = 0; i < lighting->campfireCount; i++) {
        candidates.push_back({lighting->campfirePositions[i], lighting->campfireColors[i],
                             Vector3DistanceSqr(viewPos, lighting->campfirePositions[i])});
    }
    if (lighting->lampsOn) {
        for (int i = 0; i < lighting->lampCount; i++) {
            candidates.push_back({lighting->lampPositions[i], lighting->lampColors[i],
                                 Vector3DistanceSqr(viewPos, lighting->lampPositions[i])});
        }
    }
    std::stable_sort(candidates.begin(), candidates.end(), [](const NearbyLight& a, const NearbyLight& b) {
        return a.distance < b.distance;
    });
    Vector3 combinedPositions[MAX_POINT_LIGHTS];
    Vector3 combinedColors[MAX_POINT_LIGHTS];
    int totalLights = std::min((int)candidates.size(), MAX_POINT_LIGHTS);
    for (int i = 0; i < totalLights; i++) {
        combinedPositions[i] = candidates[i].position;
        combinedColors[i] = candidates[i].color;
    }

    if (totalLights > 0) {
        SetShaderValueV(shader, pointLightPosLoc, combinedPositions, SHADER_UNIFORM_VEC3, totalLights);
        SetShaderValueV(shader, pointLightColLoc, combinedColors, SHADER_UNIFORM_VEC3, totalLights);
        SetShaderValue(shader, pointLightCountLoc, &totalLights, SHADER_UNIFORM_INT);
    } else {
        int zero = 0;
        SetShaderValue(shader, pointLightCountLoc, &zero, SHADER_UNIFORM_INT);
    }
}

bool BeginShadowPass(LightingSystem* lighting, Vector3 centerPos, Shader depthShader) {
    if (!lighting->shadowMap.id || Vector3LengthSqr(lighting->sunColor) < 0.000001f) return false;
    ShadowMatrices matrices = CalculateShadowMatrices(centerPos, lighting->sunDirection);
    lighting->lightViewProj = matrices.viewProjection;
    // Unit 10 is reserved for directional shadows, outside our scene material maps.
    rlDrawRenderBatchActive();
    rlActiveTextureSlot(10);
    rlDisableTexture();
    rlActiveTextureSlot(0);
    BeginTextureMode(lighting->shadowMap);
    rlEnableDepthMask();
    ClearBackground(WHITE);
    BeginMode3D(lighting->lightCamera);
    rlSetMatrixProjection(matrices.projection);
    rlSetMatrixModelview(matrices.view);
    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_BACK);
    BeginShaderMode(depthShader);
    return true;
}

bool IsShadowCasterVisible(const LightingSystem* lighting, Vector3 center, float radius) {
    Vector3 clip = Vector3Transform(center, lighting->lightViewProj);
    float xyRadius = radius * 2.0f / SHADOW_ORTHO_SIZE;
    float zRadius = radius * 2.0f / (SHADOW_FAR - SHADOW_NEAR);
    return fabsf(clip.x) <= 1.0f + xyRadius && fabsf(clip.y) <= 1.0f + xyRadius &&
           fabsf(clip.z) <= 1.0f + zRadius;
}

void EndShadowPass(LightingSystem* lighting) {
    (void)lighting;
    EndShaderMode();
    EndMode3D();
    rlSetCullFace(RL_CULL_FACE_BACK);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
    rlActiveTextureSlot(0);
    EndTextureMode();
}

void BindShadowMapToShader(LightingSystem* lighting, Shader shader) {
    int enabled = lighting->shadowMap.id != 0 && Vector3LengthSqr(lighting->sunColor) >= 0.000001f;
    SetShaderValue(shader, GetShaderLocation(shader, "shadowEnabled"), &enabled, SHADER_UNIFORM_INT);
    if (!enabled) return;
    int slot = 10;
    int resolution = SHADOW_MAP_RESOLUTION;
    rlActiveTextureSlot(slot);
    rlEnableTexture(lighting->shadowMap.depth.id);
    rlActiveTextureSlot(0);
    SetShaderValue(shader, GetShaderLocation(shader, "shadowMap"), &slot, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "shadowMapResolution"), &resolution, SHADER_UNIFORM_INT);
    float depthRange = SHADOW_FAR - SHADOW_NEAR;
    SetShaderValue(shader, GetShaderLocation(shader, "shadowDepthRange"), &depthRange, SHADER_UNIFORM_FLOAT);
}

Color GetSkyColor(float timeOfDay) {
    TimeColors colors = InterpolateTimeColors(timeOfDay);
    return colors.skyColor;
}

void SetSkyShaderUniforms(LightingSystem* lighting, Shader skyShader) {
    // Get time-based colors
    TimeColors colors = InterpolateTimeColors(lighting->timeOfDay);

    // Zenith color (top of sky) - use the main sky color
    Vector3 zenith = {
        colors.skyColor.r / 255.0f,
        colors.skyColor.g / 255.0f,
        colors.skyColor.b / 255.0f
    };

    // Horizon color - blend sky with fog for natural horizon
    Vector3 horizon = {
        (colors.skyColor.r / 255.0f + colors.fogColor.x) * 0.5f,
        (colors.skyColor.g / 255.0f + colors.fogColor.y) * 0.5f,
        (colors.skyColor.b / 255.0f + colors.fogColor.z) * 0.5f
    };

    // Set uniforms
    int sunDirLoc = GetShaderLocation(skyShader, "sunDirection");
    int sunColorLoc = GetShaderLocation(skyShader, "sunColor");
    int zenithLoc = GetShaderLocation(skyShader, "skyColorZenith");
    int horizonLoc = GetShaderLocation(skyShader, "skyColorHorizon");
    int timeLoc = GetShaderLocation(skyShader, "timeOfDay");

    float sunDir[3] = {lighting->sunDirection.x, lighting->sunDirection.y, lighting->sunDirection.z};
    float sunCol[3] = {lighting->sunColor.x, lighting->sunColor.y, lighting->sunColor.z};
    float zenithCol[3] = {zenith.x, zenith.y, zenith.z};
    float horizonCol[3] = {horizon.x, horizon.y, horizon.z};

    SetShaderValue(skyShader, sunDirLoc, sunDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(skyShader, sunColorLoc, sunCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(skyShader, zenithLoc, zenithCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(skyShader, horizonLoc, horizonCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(skyShader, timeLoc, &lighting->timeOfDay, SHADER_UNIFORM_FLOAT);
}

void UnloadLightingSystem(LightingSystem* lighting) {
    UnloadRenderTexture(lighting->shadowMap);
}

TimeOfDay GetTimeOfDayPhase(float timeOfDay) {
    if (timeOfDay < 0.15f) return TIME_DAWN;
    if (timeOfDay < 0.65f) return TIME_DAY;
    if (timeOfDay < 0.8f) return TIME_DUSK;
    return TIME_NIGHT;
}

bool AreLampsOn(float timeOfDay) {
    // Lamps on from dusk through night to dawn
    // On: 0.6 (approaching dusk) to 0.2 (after dawn)
    return (timeOfDay >= LAMP_ON_TIME || timeOfDay < LAMP_OFF_TIME);
}

void SetLampPositions(LightingSystem* lighting, const Vector3* positions, int count) {
    lighting->lampCount = std::max(0, count);
    lighting->lampPositions.resize(lighting->lampCount);
    lighting->lampColors.resize(lighting->lampCount);

    // Soft warm lamp color
    Vector3 lampColor = {0.9f, 0.7f, 0.4f};  // Warm yellow-orange

    for (int i = 0; i < lighting->lampCount; i++) {
        // Position light at top of lamp post (about 2.3 units up)
        lighting->lampPositions[i] = (Vector3){
            positions[i].x,
            positions[i].y + 2.3f,
            positions[i].z
        };
        lighting->lampColors[i] = lampColor;
    }

    TraceLog(LOG_INFO, "Set %d lamp positions for point lighting", lighting->lampCount);
}

void SetCampfirePositions(LightingSystem* lighting, const Vector3* positions, int count) {
    lighting->campfireCount = std::max(0, count);
    lighting->campfirePositions.resize(lighting->campfireCount);
    lighting->campfireColors.resize(lighting->campfireCount);

    // Warm campfire color (softer glow)
    Vector3 campfireColor = {1.2f, 0.7f, 0.3f};  // Warm orange fire color

    for (int i = 0; i < lighting->campfireCount; i++) {
        // Position light at flame center (about 0.8 units up)
        lighting->campfirePositions[i] = (Vector3){
            positions[i].x,
            positions[i].y + 0.8f,
            positions[i].z
        };
        lighting->campfireColors[i] = campfireColor;
    }

    TraceLog(LOG_INFO, "Set %d campfire positions for point lighting", lighting->campfireCount);
}

// ============================================================================
// Post-Processing System Implementation
// ============================================================================

// Generate hemisphere sample kernel for SSAO
static void GenerateSSAOKernel(Vector3* kernel, int sampleCount) {
    std::mt19937 random(0x3d25);
    std::uniform_real_distribution<float> uniform(-1.0f, 1.0f);
    for (int i = 0; i < sampleCount; i++) {
        // Random point in hemisphere (z >= 0)
        Vector3 sample = {
            uniform(random),
            uniform(random),
            fabsf(uniform(random))
        };

        // Normalize
        float len = sqrtf(sample.x * sample.x + sample.y * sample.y + sample.z * sample.z);
        if (len > 0.0f) {
            sample.x /= len;
            sample.y /= len;
            sample.z /= len;
        }

        // Scale to distribute more samples closer to origin
        float scale = (float)i / (float)sampleCount;
        scale = 0.1f + scale * scale * 0.9f;  // lerp(0.1, 1.0, scale^2)
        sample.x *= scale;
        sample.y *= scale;
        sample.z *= scale;

        kernel[i] = sample;
    }
}

// Generate 4x4 noise texture for SSAO rotation
static Texture2D GenerateSSAONoiseTexture() {
    unsigned char noiseData[4 * 4 * 4];  // 4x4 RGBA

    for (int i = 0; i < 16; i++) {
        // Random rotation vector in XY plane
        float angle = fmodf((float)i * 2.39996323f, 2.0f * PI);  // 0 to ~2*PI
        noiseData[i * 4 + 0] = (unsigned char)((cosf(angle) * 0.5f + 0.5f) * 255);
        noiseData[i * 4 + 1] = (unsigned char)((sinf(angle) * 0.5f + 0.5f) * 255);
        noiseData[i * 4 + 2] = 0;  // Z = 0
        noiseData[i * 4 + 3] = 255;
    }

    Image noiseImage = {
        .data = noiseData,
        .width = 4,
        .height = 4,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    Texture2D texture = LoadTextureFromImage(noiseImage);
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);  // No interpolation
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);

    return texture;
}

static bool EffectShaderReady(Shader shader) {
    return shader.id != 0 && shader.id != rlGetShaderIdDefault();
}

static void DrawFullscreenTexture(Texture2D texture, int width, int height) {
    DrawTexturePro(texture, {0, 0, (float)texture.width, -(float)texture.height},
                   {0, 0, (float)width, (float)height}, {0, 0}, 0.0f, WHITE);
}

static void AllocatePostProcessBuffers(PostProcessSystem* pp, int width, int height) {
    pp->screenWidth = std::max(1, width);
    pp->screenHeight = std::max(1, height);
    pp->sceneTexture = LoadDepthTextureTarget(pp->screenWidth, pp->screenHeight);
    bool sampleableDepth = pp->sceneTexture.id != 0;
    if (!sampleableDepth) {
        TraceLog(LOG_WARNING, "LIGHTING: SSAO disabled; falling back to ordinary scene target");
        pp->sceneTexture = LoadRenderTexture(pp->screenWidth, pp->screenHeight);
    }
    int halfWidth = std::max(1, pp->screenWidth / 2);
    int halfHeight = std::max(1, pp->screenHeight / 2);
    pp->bloomBright = LoadRenderTexture(halfWidth, halfHeight);
    pp->bloomBlur[0] = LoadRenderTexture(halfWidth, halfHeight);
    pp->bloomBlur[1] = LoadRenderTexture(halfWidth, halfHeight);
    pp->ssaoTexture = LoadRenderTexture(pp->screenWidth, pp->screenHeight);
    pp->ssaoBlurTexture = LoadRenderTexture(pp->screenWidth, pp->screenHeight);
    for (RenderTexture2D* target : {&pp->bloomBright, &pp->bloomBlur[0], &pp->bloomBlur[1],
                                    &pp->ssaoTexture, &pp->ssaoBlurTexture}) {
        SetTextureWrap(target->texture, TEXTURE_WRAP_CLAMP);
        SetTextureFilter(target->texture, TEXTURE_FILTER_BILINEAR);
    }
    pp->bloomEnabled = pp->bloomBright.id && pp->bloomBlur[0].id && pp->bloomBlur[1].id &&
        EffectShaderReady(pp->bloomExtractShader) && EffectShaderReady(pp->bloomBlurShader) &&
        EffectShaderReady(pp->compositeShader);
    pp->ssaoEnabled = sampleableDepth && pp->ssaoTexture.id && pp->ssaoBlurTexture.id &&
        EffectShaderReady(pp->ssaoShader) && EffectShaderReady(pp->ssaoBlurShader) &&
        EffectShaderReady(pp->compositeShader);
    if (!pp->bloomEnabled) TraceLog(LOG_WARNING, "LIGHTING: Bloom disabled (shader or target unavailable)");
    if (!pp->ssaoEnabled) TraceLog(LOG_WARNING, "LIGHTING: SSAO disabled (shader or target unavailable)");
    pp->initialized = pp->sceneTexture.id != 0;
    if (!pp->initialized) TraceLog(LOG_ERROR, "LIGHTING: Scene framebuffer unavailable");
}

static void FreePostProcessBuffers(PostProcessSystem* pp) {
    for (RenderTexture2D* target : {&pp->sceneTexture, &pp->bloomBright, &pp->bloomBlur[0],
                                    &pp->bloomBlur[1], &pp->ssaoTexture, &pp->ssaoBlurTexture}) {
        if (target->id) UnloadRenderTexture(*target);
        *target = {};
    }
}

void InitPostProcessSystem(PostProcessSystem* pp, int screenWidth, int screenHeight) {
    pp->bloomExtractShader = LoadShader("shaders/fullscreen.vs", "shaders/bloom_extract.fs");
    pp->bloomBlurShader = LoadShader("shaders/fullscreen.vs", "shaders/bloom_blur.fs");
    pp->compositeShader = LoadShader("shaders/fullscreen.vs", "shaders/composite.fs");
    pp->ssaoShader = LoadShader("shaders/fullscreen.vs", "shaders/ssao.fs");
    pp->ssaoBlurShader = LoadShader("shaders/fullscreen.vs", "shaders/ssao_blur.fs");
    pp->bloomThreshold = 0.85f;
    pp->bloomIntensity = 0.35f;
    pp->ssaoRadius = 0.5f;
    pp->ssaoBias = 0.025f;
    GenerateSSAOKernel(pp->ssaoKernel, 32);
    pp->noiseTexture = GenerateSSAONoiseTexture();
    AllocatePostProcessBuffers(pp, screenWidth, screenHeight);
}

void ResizePostProcessBuffers(PostProcessSystem* pp, int width, int height) {
    if (width <= 0 || height <= 0) return;
    FreePostProcessBuffers(pp);
    AllocatePostProcessBuffers(pp, width, height);
}

void RenderBloom(PostProcessSystem* pp) {
    if (!pp->bloomEnabled || !pp->initialized) return;
    int width = pp->bloomBright.texture.width;
    int height = pp->bloomBright.texture.height;
    BeginTextureMode(pp->bloomBright);
    ClearBackground(BLACK);
    BeginShaderMode(pp->bloomExtractShader);
    SetShaderValue(pp->bloomExtractShader, GetShaderLocation(pp->bloomExtractShader, "threshold"),
                   &pp->bloomThreshold, SHADER_UNIFORM_FLOAT);
    DrawFullscreenTexture(pp->sceneTexture.texture, width, height);
    EndShaderMode();
    EndTextureMode();

    Texture2D input = pp->bloomBright.texture;
    for (int pass = 0; pass < 6; pass++) {
        int target = pass % 2;
        BeginTextureMode(pp->bloomBlur[target]);
        ClearBackground(BLACK);
        BeginShaderMode(pp->bloomBlurShader);
        float direction[2] = {target == 0 ? 1.0f : 0.0f, target == 1 ? 1.0f : 0.0f};
        float texel[2] = {1.0f / width, 1.0f / height};
        SetShaderValue(pp->bloomBlurShader, GetShaderLocation(pp->bloomBlurShader, "direction"), direction, SHADER_UNIFORM_VEC2);
        SetShaderValue(pp->bloomBlurShader, GetShaderLocation(pp->bloomBlurShader, "texelSize"), texel, SHADER_UNIFORM_VEC2);
        DrawFullscreenTexture(input, width, height);
        EndShaderMode();
        EndTextureMode();
        input = pp->bloomBlur[target].texture;
    }
}

void RenderSSAO(PostProcessSystem* pp, Matrix projection) {
    if (!pp->ssaoEnabled || !pp->initialized) return;
    Matrix inverseProjection = MatrixInvert(projection);
    BeginTextureMode(pp->ssaoTexture);
    ClearBackground(WHITE);
    BeginShaderMode(pp->ssaoShader);
    SetShaderValueV(pp->ssaoShader, GetShaderLocation(pp->ssaoShader, "samples"), pp->ssaoKernel, SHADER_UNIFORM_VEC3, 32);
    SetShaderValueMatrix(pp->ssaoShader, GetShaderLocation(pp->ssaoShader, "projection"), projection);
    SetShaderValueMatrix(pp->ssaoShader, GetShaderLocation(pp->ssaoShader, "inverseProjection"), inverseProjection);
    float size[2] = {(float)pp->screenWidth, (float)pp->screenHeight};
    SetShaderValue(pp->ssaoShader, GetShaderLocation(pp->ssaoShader, "screenSize"), size, SHADER_UNIFORM_VEC2);
    SetShaderValue(pp->ssaoShader, GetShaderLocation(pp->ssaoShader, "radius"), &pp->ssaoRadius, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pp->ssaoShader, GetShaderLocation(pp->ssaoShader, "bias"), &pp->ssaoBias, SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(pp->ssaoShader, GetShaderLocation(pp->ssaoShader, "noiseTexture"), pp->noiseTexture);
    DrawFullscreenTexture(pp->sceneTexture.depth, pp->screenWidth, pp->screenHeight);
    EndShaderMode();
    EndTextureMode();

    BeginTextureMode(pp->ssaoBlurTexture);
    ClearBackground(WHITE);
    BeginShaderMode(pp->ssaoBlurShader);
    float texel[2] = {1.0f / pp->screenWidth, 1.0f / pp->screenHeight};
    SetShaderValue(pp->ssaoBlurShader, GetShaderLocation(pp->ssaoBlurShader, "texelSize"), texel, SHADER_UNIFORM_VEC2);
    SetShaderValueMatrix(pp->ssaoBlurShader, GetShaderLocation(pp->ssaoBlurShader, "inverseProjection"), inverseProjection);
    SetShaderValueTexture(pp->ssaoBlurShader, GetShaderLocation(pp->ssaoBlurShader, "depthTexture"), pp->sceneTexture.depth);
    DrawFullscreenTexture(pp->ssaoTexture.texture, pp->screenWidth, pp->screenHeight);
    EndShaderMode();
    EndTextureMode();
}

void CompositeScene(PostProcessSystem* pp) {
    if (!pp->initialized) return;
    if (!EffectShaderReady(pp->compositeShader)) {
        DrawFullscreenTexture(pp->sceneTexture.texture, pp->screenWidth, pp->screenHeight);
        return;
    }
    BeginShaderMode(pp->compositeShader);
    int bloom = pp->bloomEnabled, ssao = pp->ssaoEnabled;
    SetShaderValue(pp->compositeShader, GetShaderLocation(pp->compositeShader, "bloomEnabled"), &bloom, SHADER_UNIFORM_INT);
    SetShaderValue(pp->compositeShader, GetShaderLocation(pp->compositeShader, "ssaoEnabled"), &ssao, SHADER_UNIFORM_INT);
    SetShaderValue(pp->compositeShader, GetShaderLocation(pp->compositeShader, "bloomIntensity"), &pp->bloomIntensity, SHADER_UNIFORM_FLOAT);
    if (bloom) SetShaderValueTexture(pp->compositeShader, GetShaderLocation(pp->compositeShader, "bloomTexture"), pp->bloomBlur[1].texture);
    if (ssao) SetShaderValueTexture(pp->compositeShader, GetShaderLocation(pp->compositeShader, "aoTexture"), pp->ssaoBlurTexture.texture);
    DrawFullscreenTexture(pp->sceneTexture.texture, pp->screenWidth, pp->screenHeight);
    EndShaderMode();
}

void UnloadPostProcessSystem(PostProcessSystem* pp) {
    FreePostProcessBuffers(pp);
    for (Shader shader : {pp->bloomExtractShader, pp->bloomBlurShader, pp->compositeShader,
                           pp->ssaoShader, pp->ssaoBlurShader}) {
        if (EffectShaderReady(shader)) UnloadShader(shader);
    }
    if (pp->noiseTexture.id) UnloadTexture(pp->noiseTexture);
    *pp = {};
}
