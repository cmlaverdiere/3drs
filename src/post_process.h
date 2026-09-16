#ifndef POST_PROCESS_H
#define POST_PROCESS_H

// HDR frame pipeline:
//   scene pass (MRT: direct+fog | ambient, float depth) -> sky -> opaque copy
//   -> transparent pass -> SSAO (half res, ambient only) + volumetric light
//   -> resolve -> bloom mip chain -> tonemap/grade -> FXAA -> backbuffer

#include "raylib.h"
#include "gfx.h"

struct LightingSystem;

constexpr int BLOOM_LEVELS = 6;

struct PostProcessSystem {
    int screenWidth, screenHeight;   // logical window size (resize detection)
    int renderWidth, renderHeight;   // framebuffer pixels actually rendered
    int presentWidth, presentHeight; // backbuffer pixels

    GpuTarget scene;          // color0 direct+emissive+fog, color1 fogged ambient, depth
    GpuTarget depthCopy;      // opaque depth for water
    GpuTarget opaqueCopy;     // half-res opaque color for water refraction
    GpuTarget ssao[2];        // half-res (ao, linear depth) ping-pong
    GpuTarget volumetric[2];  // half-res in-scattered light ping-pong
    GpuTarget hdr;            // resolved HDR scene
    GpuTarget bloom[BLOOM_LEVELS];
    GpuTarget ldr;            // tonemapped, luma in alpha (FXAA input)

    Shader skyShader;
    Shader ssaoShader, ssaoBlurShader;
    Shader volumetricShader, volumetricBlurShader;
    Shader copyShader, resolveShader;
    Shader bloomDownShader, bloomUpShader;
    Shader compositeShader, fxaaShader;

    float bloomStrength;
    bool initialized;
};

void InitPostProcessSystem(PostProcessSystem* pp, LightingSystem* lighting, int screenWidth, int screenHeight);
void ResizePostProcessBuffers(PostProcessSystem* pp, int screenWidth, int screenHeight);
bool PostProcessNeedsResize(const PostProcessSystem* pp, int screenWidth, int screenHeight);

// Scene rendering into the HDR targets; call inside the frame between
// PrepareFrame() and RenderPostProcess(). BeginMode3D is handled here.
void BeginScenePass(PostProcessSystem* pp, const Camera3D& camera);
void DrawSkyPass(PostProcessSystem* pp);
// Snapshots opaque depth/color for refraction; returns to the scene pass.
void CaptureOpaqueScene(PostProcessSystem* pp, const Camera3D& camera);
void EndScenePass(PostProcessSystem* pp);

// Post chain up to the tonemapped LDR image
void RenderPostProcess(PostProcessSystem* pp, const LightingSystem* lighting);
// FXAA onto the backbuffer; call right after BeginDrawing()
void PresentFrame(PostProcessSystem* pp);

void UnloadPostProcessSystem(PostProcessSystem* pp);

#endif
