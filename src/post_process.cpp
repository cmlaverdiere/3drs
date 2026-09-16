#include "post_process.h"
#include "lighting.h"
#include "shader_utils.h"
#include "rlgl.h"
#include "external/glad.h"
#include <algorithm>
#include <cstdlib>

static float RenderScale() {
    const char* env = getenv("GAME_RENDER_SCALE");
    float scale = env ? (float)atof(env) : 1.0f;
    return scale > 0.25f && scale <= 2.0f ? scale : 1.0f;
}

static Shader LoadPostShader(LightingSystem* lighting, const char* fs, const char* defines = nullptr) {
    return RegisterSceneShader(lighting, LoadShaderVariant("shaders/fullscreen_tri.vs", fs, defines));
}

static void SetUniform1f(Shader s, const char* name, float v) {
    int loc = glGetUniformLocation(s.id, name);
    if (loc >= 0) glUniform1f(loc, v);
}

static void SetUniform2f(Shader s, const char* name, float x, float y) {
    int loc = glGetUniformLocation(s.id, name);
    if (loc >= 0) glUniform2f(loc, x, y);
}

static void SetUniform1i(Shader s, const char* name, int v) {
    int loc = glGetUniformLocation(s.id, name);
    if (loc >= 0) glUniform1i(loc, v);
}

static void Allocate(PostProcessSystem* pp) {
    float scale = RenderScale();
    int w = std::max(1, (int)(GetRenderWidth() * scale));
    int h = std::max(1, (int)(GetRenderHeight() * scale));
    pp->renderWidth = w;
    pp->renderHeight = h;
    pp->presentWidth = GetRenderWidth();
    pp->presentHeight = GetRenderHeight();
    int hw = std::max(1, w / 2), hh = std::max(1, h / 2);

    pp->scene = CreateGpuTarget(w, h, {GpuFormat::RGBA16F, GpuFormat::RGBA16F}, true, true);
    pp->depthCopy = CreateGpuTarget(w, h, {}, true);
    pp->opaqueCopy = CreateGpuTarget(hw, hh, {GpuFormat::RGBA16F}, false, true);
    for (int i = 0; i < 2; i++) {
        pp->ssao[i] = CreateGpuTarget(hw, hh, {GpuFormat::RG16F}, false, true);
        pp->volumetric[i] = CreateGpuTarget(hw, hh, {GpuFormat::RGBA16F}, false, true);
    }
    pp->hdr = CreateGpuTarget(w, h, {GpuFormat::RGBA16F}, false, true);
    int bw = w, bh = h;
    for (int i = 0; i < BLOOM_LEVELS; i++) {
        bw = std::max(1, bw / 2);
        bh = std::max(1, bh / 2);
        pp->bloom[i] = CreateGpuTarget(bw, bh, {GpuFormat::RGBA16F}, false, true);
    }
    pp->ldr = CreateGpuTarget(w, h, {GpuFormat::RGBA8}, false, true);
    pp->initialized = pp->scene.fbo && pp->hdr.fbo && pp->ldr.fbo;
    if (!pp->initialized) TraceLog(LOG_ERROR, "POST: Scene framebuffers unavailable");
    TraceLog(LOG_INFO, "POST: Rendering %dx%d (present %dx%d)", w, h, pp->presentWidth, pp->presentHeight);
}

static void Free(PostProcessSystem* pp) {
    for (GpuTarget* t : {&pp->scene, &pp->depthCopy, &pp->opaqueCopy, &pp->ssao[0], &pp->ssao[1],
                         &pp->volumetric[0], &pp->volumetric[1], &pp->hdr, &pp->ldr}) {
        DestroyGpuTarget(t);
    }
    for (int i = 0; i < BLOOM_LEVELS; i++) DestroyGpuTarget(&pp->bloom[i]);
}

void InitPostProcessSystem(PostProcessSystem* pp, LightingSystem* lighting, int screenWidth, int screenHeight) {
    pp->screenWidth = screenWidth;
    pp->screenHeight = screenHeight;
    pp->skyShader = LoadPostShader(lighting, "shaders/sky.fs");
    pp->ssaoShader = LoadPostShader(lighting, "shaders/ssao.fs");
    pp->ssaoBlurShader = LoadPostShader(lighting, "shaders/ssao_blur.fs");
    pp->volumetricShader = LoadPostShader(lighting, "shaders/volumetric.fs");
    pp->volumetricBlurShader = LoadPostShader(lighting, "shaders/volumetric_blur.fs");
    pp->copyShader = LoadPostShader(lighting, "shaders/copy_opaque.fs");
    pp->resolveShader = LoadPostShader(lighting, "shaders/resolve.fs");
    pp->bloomDownShader = LoadPostShader(lighting, "shaders/bloom_down.fs");
    pp->bloomUpShader = LoadPostShader(lighting, "shaders/bloom_up.fs");
    pp->compositeShader = LoadPostShader(lighting, "shaders/composite.fs");
    pp->fxaaShader = LoadPostShader(lighting, "shaders/fxaa.fs");

    SetSamplerUnit(pp->ssaoShader, "uDepth", 0);
    SetSamplerUnit(pp->ssaoBlurShader, "uSource", 0);
    SetSamplerUnit(pp->volumetricShader, "uDepth", 0);
    SetSamplerUnit(pp->volumetricBlurShader, "uSource", 0);
    SetSamplerUnit(pp->volumetricBlurShader, "uAO", 1);
    SetSamplerUnit(pp->copyShader, "uDirect", 0);
    SetSamplerUnit(pp->copyShader, "uAmbient", 1);
    SetSamplerUnit(pp->resolveShader, "uDirect", 0);
    SetSamplerUnit(pp->resolveShader, "uAmbient", 1);
    SetSamplerUnit(pp->resolveShader, "uAO", 2);
    SetSamplerUnit(pp->resolveShader, "uVolumetric", 3);
    SetSamplerUnit(pp->resolveShader, "uDepth", 4);
    SetSamplerUnit(pp->bloomDownShader, "uSource", 0);
    SetSamplerUnit(pp->bloomUpShader, "uSource", 0);
    SetSamplerUnit(pp->compositeShader, "uHDR", 0);
    SetSamplerUnit(pp->compositeShader, "uBloom", 1);
    SetSamplerUnit(pp->fxaaShader, "uSource", 0);
    pp->bloomStrength = 0.055f;
    Allocate(pp);
}

bool PostProcessNeedsResize(const PostProcessSystem* pp, int screenWidth, int screenHeight) {
    return pp->screenWidth != screenWidth || pp->screenHeight != screenHeight ||
           pp->presentWidth != GetRenderWidth() || pp->presentHeight != GetRenderHeight();
}

void ResizePostProcessBuffers(PostProcessSystem* pp, int screenWidth, int screenHeight) {
    if (screenWidth <= 0 || screenHeight <= 0) return;
    pp->screenWidth = screenWidth;
    pp->screenHeight = screenHeight;
    Free(pp);
    Allocate(pp);
}

void BeginScenePass(PostProcessSystem* pp, const Camera3D& camera) {
    BeginTextureMode(AsRenderTexture(pp->scene));
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    BeginMode3D(camera);
}

void DrawSkyPass(PostProcessSystem* pp) {
    // Drawn after opaque geometry at the far plane: covered pixels fail the depth test.
    rlDrawRenderBatchActive();
    glUseProgram(pp->skyShader.id);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    DrawFullscreenTriangle();
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glUseProgram(rlGetShaderIdDefault());
}

void CaptureOpaqueScene(PostProcessSystem* pp, const Camera3D& camera) {
    rlDrawRenderBatchActive();
    CopyDepth(pp->scene, pp->depthCopy);
    BeginFullscreenPass(pp->copyShader, &pp->opaqueCopy);
    BindTextureUnit(0, pp->scene.color[0]);
    BindTextureUnit(1, pp->scene.color[1]);
    DrawFullscreenTriangle();
    EndFullscreenPass();
    // Back to the scene target with the same camera state
    BindGpuTarget(pp->scene);
    glEnable(GL_DEPTH_TEST);
    BindTextureUnit(TEX_UNIT_SCENE_COLOR, pp->opaqueCopy.color[0]);
    BindTextureUnit(TEX_UNIT_SCENE_DEPTH, pp->depthCopy.depth);
    (void)camera;
}

void EndScenePass(PostProcessSystem* pp) {
    (void)pp;
    EndMode3D();
    EndTextureMode();
}

void RenderPostProcess(PostProcessSystem* pp, const LightingSystem* lighting) {
    if (!pp->initialized) return;
    rlDrawRenderBatchActive();
    const FrameUniforms& f = lighting->frame;
    Matrix invProj = MatrixInvert(f.projection);
    int hw = pp->ssao[0].width, hh = pp->ssao[0].height;

    // SSAO at half resolution, then a depth-aware blur
    BeginFullscreenPass(pp->ssaoShader, &pp->ssao[0]);
    BindTextureUnit(0, pp->scene.depth);
    glUniformMatrix4fv(glGetUniformLocation(pp->ssaoShader.id, "uInvProj"), 1, GL_FALSE, MatrixToFloat(invProj));
    DrawFullscreenTriangle();
    for (int pass = 0; pass < 2; pass++) {
        BeginFullscreenPass(pp->ssaoBlurShader, &pp->ssao[(pass + 1) % 2]);
        BindTextureUnit(0, pp->ssao[pass % 2].color[0]);
        SetUniform2f(pp->ssaoBlurShader, "uDirection", pass == 0 ? 1.0f / hw : 0.0f, pass == 0 ? 0.0f : 1.0f / hh);
        DrawFullscreenTriangle();
    }

    // Volumetric light (shadowed sun/moon shafts + point light halos)
    BeginFullscreenPass(pp->volumetricShader, &pp->volumetric[0]);
    BindTextureUnit(0, pp->scene.depth);
    DrawFullscreenTriangle();
    for (int pass = 0; pass < 2; pass++) {
        BeginFullscreenPass(pp->volumetricBlurShader, &pp->volumetric[(pass + 1) % 2]);
        BindTextureUnit(0, pp->volumetric[pass % 2].color[0]);
        BindTextureUnit(1, pp->ssao[0].color[0]);
        SetUniform2f(pp->volumetricBlurShader, "uDirection", pass == 0 ? 1.0f / hw : 0.0f, pass == 0 ? 0.0f : 1.0f / hh);
        DrawFullscreenTriangle();
    }

    // Resolve: direct + ambient * AO + in-scattering
    BeginFullscreenPass(pp->resolveShader, &pp->hdr);
    BindTextureUnit(0, pp->scene.color[0]);
    BindTextureUnit(1, pp->scene.color[1]);
    BindTextureUnit(2, pp->ssao[0].color[0]);
    BindTextureUnit(3, pp->volumetric[0].color[0]);
    BindTextureUnit(4, pp->scene.depth);
    SetUniform2f(pp->resolveShader, "uHalfTexel", 1.0f / hw, 1.0f / hh);
    static int resolveDebug = getenv("GAME_DEBUG_VIEW") ? atoi(getenv("GAME_DEBUG_VIEW")) : 0;
    SetUniform1i(pp->resolveShader, "uDebugView", resolveDebug);
    DrawFullscreenTriangle();

    // Bloom: 13-tap downsample chain, tent upsample accumulation
    unsigned int source = pp->hdr.color[0];
    int sw = pp->hdr.width, sh = pp->hdr.height;
    for (int i = 0; i < BLOOM_LEVELS; i++) {
        BeginFullscreenPass(pp->bloomDownShader, &pp->bloom[i]);
        BindTextureUnit(0, source);
        SetUniform2f(pp->bloomDownShader, "uTexel", 1.0f / sw, 1.0f / sh);
        SetUniform1i(pp->bloomDownShader, "uFirst", i == 0 ? 1 : 0);
        DrawFullscreenTriangle();
        source = pp->bloom[i].color[0];
        sw = pp->bloom[i].width;
        sh = pp->bloom[i].height;
    }
    for (int i = BLOOM_LEVELS - 1; i > 0; i--) {
        BeginFullscreenPass(pp->bloomUpShader, &pp->bloom[i - 1]);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        BindTextureUnit(0, pp->bloom[i].color[0]);
        SetUniform2f(pp->bloomUpShader, "uTexel", 1.0f / pp->bloom[i].width, 1.0f / pp->bloom[i].height);
        DrawFullscreenTriangle();
        glDisable(GL_BLEND);
    }

    // Tonemap + grade into LDR (luma in alpha for FXAA)
    BeginFullscreenPass(pp->compositeShader, &pp->ldr);
    BindTextureUnit(0, pp->hdr.color[0]);
    BindTextureUnit(1, pp->bloom[0].color[0]);
    SetUniform1f(pp->compositeShader, "uBloomStrength", pp->bloomStrength);
    static int debugView = getenv("GAME_DEBUG_VIEW") ? atoi(getenv("GAME_DEBUG_VIEW")) : 0;
    SetUniform1i(pp->compositeShader, "uDebugView", debugView);
    DrawFullscreenTriangle();
    EndFullscreenPass();
}

void PresentFrame(PostProcessSystem* pp) {
    if (!pp->initialized) return;
    BeginFullscreenPass(pp->fxaaShader, nullptr, pp->presentWidth, pp->presentHeight);
    BindTextureUnit(0, pp->ldr.color[0]);
    SetUniform2f(pp->fxaaShader, "uTexel", 1.0f / pp->ldr.width, 1.0f / pp->ldr.height);
    DrawFullscreenTriangle();
    EndFullscreenPass();
}

void UnloadPostProcessSystem(PostProcessSystem* pp) {
    Free(pp);
    for (Shader s : {pp->skyShader, pp->ssaoShader, pp->ssaoBlurShader, pp->volumetricShader,
                     pp->volumetricBlurShader, pp->copyShader, pp->resolveShader, pp->bloomDownShader,
                     pp->bloomUpShader, pp->compositeShader, pp->fxaaShader}) {
        if (s.id && s.id != rlGetShaderIdDefault()) UnloadShader(s);
    }
    *pp = {};
}
