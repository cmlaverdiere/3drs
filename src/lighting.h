#ifndef LIGHTING_H
#define LIGHTING_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

// Shadow map configuration
const int SHADOW_MAP_RESOLUTION = 2048;
const float SHADOW_ORTHO_SIZE = 200.0f;  // Coverage area for orthographic projection

// Day/night cycle configuration
const float DAY_CYCLE_DURATION = 600.0f;  // 10 minutes for full cycle

// Time of day phases
enum TimeOfDay {
    TIME_DAWN = 0,    // 0.0 - 0.15
    TIME_DAY,         // 0.15 - 0.65
    TIME_DUSK,        // 0.65 - 0.8
    TIME_NIGHT        // 0.8 - 1.0
};

// Centralized lighting state
struct LightingSystem {
    // Shadow mapping
    RenderTexture2D shadowMap;
    Matrix lightViewProj;

    // Sun properties (updated by day/night cycle)
    Vector3 sunDirection;
    Vector3 sunColor;
    Vector3 ambientColor;

    // Fog
    Vector3 fogColor;
    float fogDensity;

    // Day/night cycle
    float timeOfDay;        // 0.0 to 1.0
    bool cyclePaused;

    // Light camera for shadow rendering
    Camera3D lightCamera;

    // Shader uniform locations (cached per shader)
    int sunDirLoc;
    int sunColorLoc;
    int ambientLoc;
    int fogColorLoc;
    int fogDensityLoc;
    int lightVPLoc;
    int shadowMapLoc;
    int viewPosLoc;
};

// Initialize the lighting system
void InitLightingSystem(LightingSystem* lighting);

// Update day/night cycle and recalculate sun position/colors
void UpdateLightingSystem(LightingSystem* lighting, float dt, Vector3 playerPos);

// Cache shader uniform locations for a shader
void CacheShaderLightingLocs(LightingSystem* lighting, Shader shader);

// Set lighting uniforms on a shader (call each frame)
void SetShaderLightingUniforms(LightingSystem* lighting, Shader shader, Vector3 viewPos);

// Begin shadow map render pass (pass depth shader to use)
void BeginShadowPass(LightingSystem* lighting, Vector3 centerPos, Shader depthShader);

// End shadow map render pass
void EndShadowPass(LightingSystem* lighting);

// Bind shadow map texture to a shader
void BindShadowMapToShader(LightingSystem* lighting, Shader shader);

// Get sky color for current time of day
Color GetSkyColor(float timeOfDay);

// Cleanup
void UnloadLightingSystem(LightingSystem* lighting);

// Get current time of day phase
TimeOfDay GetTimeOfDayPhase(float timeOfDay);

#endif
