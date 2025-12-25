#include "lighting.h"
#include <cmath>

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

// Create shadow map render texture (uses color texture to store depth)
static RenderTexture2D LoadShadowmapRenderTexture(int width, int height) {
    // Use standard render texture - we'll write depth to color in the shader
    RenderTexture2D target = LoadRenderTexture(width, height);
    TraceLog(LOG_INFO, "Shadow map render texture created (%dx%d)", width, height);
    return target;
}

void InitLightingSystem(LightingSystem* lighting) {
    // Create shadow map
    lighting->shadowMap = LoadShadowmapRenderTexture(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);

    // Start at midday
    lighting->timeOfDay = 0.5f;
    lighting->cyclePaused = false;

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
}

void UpdateLightingSystem(LightingSystem* lighting, float dt, Vector3 playerPos) {
    // Advance time
    if (!lighting->cyclePaused) {
        lighting->timeOfDay += dt / DAY_CYCLE_DURATION;
        if (lighting->timeOfDay >= 1.0f) lighting->timeOfDay -= 1.0f;
    }

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
}

void BeginShadowPass(LightingSystem* lighting, Vector3 centerPos, Shader depthShader) {
    // Update light camera
    float shadowDistance = SHADOW_ORTHO_SIZE * 0.5f;
    lighting->lightCamera.position = Vector3Add(
        centerPos,
        Vector3Scale(lighting->sunDirection, -shadowDistance)
    );
    lighting->lightCamera.target = centerPos;

    // Compute light view matrix
    Matrix lightView = MatrixLookAt(
        lighting->lightCamera.position,
        lighting->lightCamera.target,
        lighting->lightCamera.up
    );

    // Compute orthographic projection for directional light
    float halfSize = SHADOW_ORTHO_SIZE * 0.5f;
    Matrix lightProj = MatrixOrtho(-halfSize, halfSize, -halfSize, halfSize, 0.1f, shadowDistance * 2.0f);

    // Store combined matrix for shader use
    lighting->lightViewProj = MatrixMultiply(lightView, lightProj);

    // Begin rendering to shadow map
    BeginTextureMode(lighting->shadowMap);
    ClearBackground(WHITE);  // Clear to white (max depth = 1.0)

    // Set up 3D mode with light's view
    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    rlMultMatrixf(MatrixToFloat(lightProj));
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlMultMatrixf(MatrixToFloat(lightView));

    // Enable front-face culling to reduce shadow acne
    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_FRONT);

    // Set depth shader AFTER matrix setup
    BeginShaderMode(depthShader);
}

void EndShadowPass(LightingSystem* lighting) {
    // End depth shader
    EndShaderMode();

    // Reset culling
    rlSetCullFace(RL_CULL_FACE_BACK);

    // Pop matrix state
    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
    rlMatrixMode(RL_MODELVIEW);

    EndTextureMode();
}

void BindShadowMapToShader(LightingSystem* lighting, Shader shader) {
    int shadowMapLoc = GetShaderLocation(shader, "shadowMap");
    int shadowResLoc = GetShaderLocation(shader, "shadowMapResolution");

    // Bind shadow map color texture to texture slot 1
    rlActiveTextureSlot(1);
    rlEnableTexture(lighting->shadowMap.texture.id);
    SetShaderValue(shader, shadowMapLoc, (int[]){1}, SHADER_UNIFORM_INT);
    SetShaderValue(shader, shadowResLoc, (int[]){SHADOW_MAP_RESOLUTION}, SHADER_UNIFORM_INT);
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

// ============================================================================
// Post-Processing System Implementation
// ============================================================================

void InitPostProcessSystem(PostProcessSystem* pp, int screenWidth, int screenHeight) {
    pp->screenWidth = screenWidth;
    pp->screenHeight = screenHeight;

    // Create scene render texture (full resolution)
    pp->sceneTexture = LoadRenderTexture(screenWidth, screenHeight);

    // Create bloom textures (half resolution for performance)
    int halfWidth = screenWidth / 2;
    int halfHeight = screenHeight / 2;
    pp->bloomBright = LoadRenderTexture(halfWidth, halfHeight);
    pp->bloomBlur[0] = LoadRenderTexture(halfWidth, halfHeight);
    pp->bloomBlur[1] = LoadRenderTexture(halfWidth, halfHeight);

    // Load bloom shaders
    pp->bloomExtractShader = LoadShader("shaders/fullscreen.vs", "shaders/bloom_extract.fs");
    pp->bloomBlurShader = LoadShader("shaders/fullscreen.vs", "shaders/bloom_blur.fs");
    pp->compositeShader = LoadShader("shaders/fullscreen.vs", "shaders/composite.fs");

    // Set default bloom parameters
    pp->bloomThreshold = 0.7f;
    pp->bloomIntensity = 1.2f;
    pp->bloomEnabled = true;

    // SSAO will be initialized separately (Phase 3)
    pp->ssaoEnabled = false;

    pp->initialized = true;

    TraceLog(LOG_INFO, "Post-processing system initialized (%dx%d, bloom half-res: %dx%d)",
             screenWidth, screenHeight, halfWidth, halfHeight);
}

void ResizePostProcessBuffers(PostProcessSystem* pp, int width, int height) {
    if (!pp->initialized) return;

    // Unload existing textures
    UnloadRenderTexture(pp->sceneTexture);
    UnloadRenderTexture(pp->bloomBright);
    UnloadRenderTexture(pp->bloomBlur[0]);
    UnloadRenderTexture(pp->bloomBlur[1]);

    // Recreate at new size
    pp->screenWidth = width;
    pp->screenHeight = height;
    pp->sceneTexture = LoadRenderTexture(width, height);

    int halfWidth = width / 2;
    int halfHeight = height / 2;
    pp->bloomBright = LoadRenderTexture(halfWidth, halfHeight);
    pp->bloomBlur[0] = LoadRenderTexture(halfWidth, halfHeight);
    pp->bloomBlur[1] = LoadRenderTexture(halfWidth, halfHeight);
}

void RenderBloom(PostProcessSystem* pp) {
    if (!pp->bloomEnabled || !pp->initialized) return;

    int halfWidth = pp->screenWidth / 2;
    int halfHeight = pp->screenHeight / 2;

    // Pass 1: Extract bright pixels
    BeginTextureMode(pp->bloomBright);
    ClearBackground(BLACK);
    BeginShaderMode(pp->bloomExtractShader);
        int thresholdLoc = GetShaderLocation(pp->bloomExtractShader, "threshold");
        SetShaderValue(pp->bloomExtractShader, thresholdLoc, &pp->bloomThreshold, SHADER_UNIFORM_FLOAT);
        // Draw scene texture to extract bright areas (flip Y for render texture)
        DrawTextureRec(pp->sceneTexture.texture,
                       (Rectangle){0, 0, (float)pp->screenWidth, (float)-pp->screenHeight},
                       (Vector2){0, 0}, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Pass 2: Horizontal blur
    BeginTextureMode(pp->bloomBlur[0]);
    ClearBackground(BLACK);
    BeginShaderMode(pp->bloomBlurShader);
        float direction[2] = {1.0f, 0.0f};
        float texelSize[2] = {1.0f / halfWidth, 1.0f / halfHeight};
        int dirLoc = GetShaderLocation(pp->bloomBlurShader, "direction");
        int texelLoc = GetShaderLocation(pp->bloomBlurShader, "texelSize");
        SetShaderValue(pp->bloomBlurShader, dirLoc, direction, SHADER_UNIFORM_VEC2);
        SetShaderValue(pp->bloomBlurShader, texelLoc, texelSize, SHADER_UNIFORM_VEC2);
        DrawTextureRec(pp->bloomBright.texture,
                       (Rectangle){0, 0, (float)halfWidth, (float)-halfHeight},
                       (Vector2){0, 0}, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Pass 3: Vertical blur
    BeginTextureMode(pp->bloomBlur[1]);
    ClearBackground(BLACK);
    BeginShaderMode(pp->bloomBlurShader);
        direction[0] = 0.0f;
        direction[1] = 1.0f;
        SetShaderValue(pp->bloomBlurShader, dirLoc, direction, SHADER_UNIFORM_VEC2);
        DrawTextureRec(pp->bloomBlur[0].texture,
                       (Rectangle){0, 0, (float)halfWidth, (float)-halfHeight},
                       (Vector2){0, 0}, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Additional blur passes for smoother result
    for (int i = 0; i < 2; i++) {
        // Horizontal
        BeginTextureMode(pp->bloomBlur[0]);
        BeginShaderMode(pp->bloomBlurShader);
            direction[0] = 1.0f;
            direction[1] = 0.0f;
            SetShaderValue(pp->bloomBlurShader, dirLoc, direction, SHADER_UNIFORM_VEC2);
            DrawTextureRec(pp->bloomBlur[1].texture,
                           (Rectangle){0, 0, (float)halfWidth, (float)-halfHeight},
                           (Vector2){0, 0}, WHITE);
        EndShaderMode();
        EndTextureMode();

        // Vertical
        BeginTextureMode(pp->bloomBlur[1]);
        BeginShaderMode(pp->bloomBlurShader);
            direction[0] = 0.0f;
            direction[1] = 1.0f;
            SetShaderValue(pp->bloomBlurShader, dirLoc, direction, SHADER_UNIFORM_VEC2);
            DrawTextureRec(pp->bloomBlur[0].texture,
                           (Rectangle){0, 0, (float)halfWidth, (float)-halfHeight},
                           (Vector2){0, 0}, WHITE);
        EndShaderMode();
        EndTextureMode();
    }
}

void RenderSSAO(PostProcessSystem* pp, Camera3D camera, Matrix projection) {
    // SSAO implementation will be added in Phase 3
    (void)pp;
    (void)camera;
    (void)projection;
}

void CompositeScene(PostProcessSystem* pp) {
    if (!pp->initialized) return;

    // Draw scene (flip Y for render texture)
    DrawTextureRec(pp->sceneTexture.texture,
                   (Rectangle){0, 0, (float)pp->screenWidth, (float)-pp->screenHeight},
                   (Vector2){0, 0}, WHITE);

    if (pp->bloomEnabled) {
        // Draw bloom as additive overlay (scale up from half-res)
        BeginBlendMode(BLEND_ADDITIVE);
        DrawTexturePro(pp->bloomBlur[1].texture,
                       (Rectangle){0, 0, (float)(pp->screenWidth/2), (float)-(pp->screenHeight/2)},
                       (Rectangle){0, 0, (float)pp->screenWidth, (float)pp->screenHeight},
                       (Vector2){0, 0}, 0.0f,
                       (Color){255, 255, 255, (unsigned char)(pp->bloomIntensity * 200)});
        EndBlendMode();
    }
}

void UnloadPostProcessSystem(PostProcessSystem* pp) {
    if (!pp->initialized) return;

    UnloadRenderTexture(pp->sceneTexture);
    UnloadRenderTexture(pp->bloomBright);
    UnloadRenderTexture(pp->bloomBlur[0]);
    UnloadRenderTexture(pp->bloomBlur[1]);

    UnloadShader(pp->bloomExtractShader);
    UnloadShader(pp->bloomBlurShader);
    UnloadShader(pp->compositeShader);

    if (pp->ssaoEnabled) {
        UnloadRenderTexture(pp->ssaoTexture);
        UnloadRenderTexture(pp->ssaoBlurTexture);
        UnloadTexture(pp->noiseTexture);
        UnloadShader(pp->ssaoShader);
        UnloadShader(pp->ssaoBlurShader);
    }

    pp->initialized = false;
}
