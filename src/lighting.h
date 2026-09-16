#ifndef LIGHTING_H
#define LIGHTING_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "gfx.h"
#include "lighting_math.h"
#include <vector>

// Day/night cycle configuration
const float DAY_CYCLE_DURATION = 1200.0f;  // 20 minutes for full cycle

// Point lights (lamps)
const int MAX_POINT_LIGHTS = 16;  // Shader limit
const float LAMP_ON_TIME = 0.6f;   // Lamps turn on (approaching dusk)
const float LAMP_OFF_TIME = 0.2f;  // Lamps turn off (after dawn)

// Cascaded shadows: four 2048² tiles in one 4096² depth atlas
constexpr int SHADOW_CASCADES = 4;
constexpr int SHADOW_TILE_SIZE = SHADOW_MAP_RESOLUTION / 2;
constexpr float SHADOW_CASCADE_SPLITS[SHADOW_CASCADES] = {10.0f, 28.0f, 72.0f, 190.0f};

// Texture units shared by every scene shader (raylib's batch uses 0-3)
enum GlobalTextureUnit {
    TEX_UNIT_SHADOW = 8,       // sampler2DShadow (hardware PCF)
    TEX_UNIT_SKY = 9,          // sky-view LUT
    TEX_UNIT_SCENE_COLOR = 10, // opaque scene copy (water refraction)
    TEX_UNIT_SCENE_DEPTH = 11, // opaque depth copy (water depth)
    TEX_UNIT_SHADOW_RAW = 12,  // shadow atlas without compare (volumetrics)
};

// Time of day phases
enum TimeOfDay {
    TIME_DAWN = 0,    // 0.0 - 0.15
    TIME_DAY,         // 0.15 - 0.65
    TIME_DUSK,        // 0.65 - 0.8
    TIME_NIGHT        // 0.8 - 1.0
};

// std140 mirror of the FrameData block in shaders/common/frame.glsl.
// Only mat4 and vec4 members, so offsets are sequential.
struct FrameUniforms {
    Matrix view;
    Matrix projection;
    Matrix viewProjection;
    Matrix invViewProjection;
    float camera[4];         // xyz position, w = time in seconds
    float lightDir[4];       // toward the key light (sun or moon), w = 1 when moonlit
    float lightColor[4];     // key light irradiance / pi at the ground
    float sunDir[4];         // toward the sun, w = sun disc visibility
    float sunColor[4];       // sun illuminance above the atmosphere
    float moonDir[4];        // toward the moon, w = moon visibility
    float moonColor[4];      // moon illuminance above the atmosphere
    float ambientSH[4][4];   // L1 SH of sky irradiance / pi (c0, x, y, z)
    float fog[4];            // density, height falloff, base height, strength
    float clouds[4];         // coverage, altitude, wind offset x, wind offset z
    float wind[4];           // direction xz, strength, gust
    float season[4];         // season id, snow cover, time of day, night factor
    Matrix shadowMatrix[SHADOW_CASCADES];
    float shadowSplits[4];   // far distance of each cascade
    float shadowTexel[4];    // world size of one texel in each cascade
    float shadowParams[4];   // enabled, cascade count, fade start, fade end
    float pointPos[MAX_POINT_LIGHTS][4];    // xyz, radius
    float pointColor[MAX_POINT_LIGHTS][4];  // rgb
    float counts[4];         // point light count
    float screen[4];         // render width, height, 1/width, 1/height
    float exposure[4];       // exposure, night factor, unused, unused
};

// Centralized lighting state
struct LightingSystem {
    // Day/night cycle
    float timeOfDay;        // 0.0 to 1.0
    bool cyclePaused;
    bool lampsOn;           // True when lamps should be lit

    // Celestial state (directions point toward the body)
    Vector3 sunDir;
    Vector3 moonDir;
    float sunVisibility;
    float moonVisibility;
    Vector3 sunIlluminance;   // above the atmosphere
    Vector3 moonIlluminance;

    // Key light used for direct lighting and shadows (sun by day, moon by night)
    Vector3 lightDir;
    Vector3 lightColor;       // irradiance / pi at the ground
    bool keyIsMoon;

    // Sky irradiance (L1 spherical harmonics), fog and exposure
    Vector3 ambientSH[4];
    Vector3 fogColor;         // horizon average, for CPU-side consumers
    float fogDensity;
    float exposure;
    float nightFactor;
    float lastAtmosphereTime;
    int lastSeason;

    // Shadows
    GpuTarget shadowAtlas;
    ShadowMatrices cascades[SHADOW_CASCADES];
    float cascadeTexel[SHADOW_CASCADES];
    bool shadowsEnabled;

    // Lamp and campfire lights
    std::vector<Vector3> lampPositions;
    std::vector<Vector3> lampColors;
    int lampCount;
    std::vector<Vector3> campfirePositions;
    std::vector<Vector3> campfireColors;
    int campfireCount;

    // GPU state
    unsigned int frameUbo;
    FrameUniforms frame;
    GpuTarget skyLut;
    Shader skyLutShader;
    Camera3D lightCamera;
};

// Initialize the lighting system (needs a GL context)
void InitLightingSystem(LightingSystem* lighting);

// Update day/night cycle, celestial bodies, atmosphere colours and exposure
void UpdateLightingSystem(LightingSystem* lighting, float dt, Vector3 playerPos);

// Bind the FrameData block and global sampler units of a scene shader
// (lighting may be null; bindings are global)
Shader RegisterSceneShader(LightingSystem* lighting, Shader shader);

// Compute camera + cascade matrices and upload the frame uniform block
void PrepareFrame(LightingSystem* lighting, const Camera3D& camera, int renderWidth, int renderHeight);

// Render the sky-view LUT for this frame
void RenderSkyLut(LightingSystem* lighting);

// Shadow cascades: returns false when shadows are off this frame
bool BeginShadowPass(LightingSystem* lighting);
void BeginShadowCascade(LightingSystem* lighting, int cascade);
bool IsShadowCasterVisible(const LightingSystem* lighting, int cascade, Vector3 center, float radius);
void EndShadowPass(LightingSystem* lighting);

// Bind shadow atlas and sky LUT to their global texture units
void BindGlobalLightingTextures(const LightingSystem* lighting);

// Cleanup
void UnloadLightingSystem(LightingSystem* lighting);

// Get current time of day phase
TimeOfDay GetTimeOfDayPhase(float timeOfDay);

// Set lamp positions for point lighting (call after loading map)
void SetLampPositions(LightingSystem* lighting, const Vector3* positions, int count);

// Set campfire positions for point lighting (always on)
void SetCampfirePositions(LightingSystem* lighting, const Vector3* positions, int count);

// Check if lamps should be on at current time
bool AreLampsOn(float timeOfDay);

// Sun direction (toward the sun) for a time of day; exposed for tests
Vector3 SunDirectionAt(float timeOfDay);

#endif
