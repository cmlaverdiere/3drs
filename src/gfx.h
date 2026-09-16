#ifndef GFX_H
#define GFX_H

// Thin OpenGL helpers for the parts of the renderer raylib doesn't cover:
// float/MRT framebuffers, sampleable depth, uniform buffers and fullscreen passes.

#include "raylib.h"
#include <cstddef>
#include <initializer_list>

enum class GpuFormat { RGBA8, RGBA16F, RG16F, R16F, R8 };

struct GpuTarget {
    unsigned int fbo = 0;
    unsigned int color[4] = {};
    GpuFormat formats[4] = {};
    int colorCount = 0;
    unsigned int depth = 0;
    int width = 0;
    int height = 0;
};

// Creates a framebuffer with the given color attachments and an optional
// sampleable 32-bit float depth texture. Returns fbo == 0 on failure.
GpuTarget CreateGpuTarget(int width, int height, std::initializer_list<GpuFormat> colors,
                          bool withDepth, bool linearFilter = true);
void DestroyGpuTarget(GpuTarget* target);

// Binds the framebuffer and sets the viewport to cover it.
void BindGpuTarget(const GpuTarget& target);
// Binds a sub-rectangle viewport (and scissor) of the bound target.
void SetViewportRect(int x, int y, int width, int height);

// Lets raylib's BeginTextureMode()/BeginMode3D() render into our target.
RenderTexture2D AsRenderTexture(const GpuTarget& target);

// Depth compare mode turns a depth texture into a sampler2DShadow source.
void SetDepthCompare(unsigned int depthTexture, bool enabled);

void BindTextureUnit(int unit, unsigned int texture);
void CopyDepth(const GpuTarget& src, const GpuTarget& dst);

// Fullscreen pass state: no blending, no depth, flushes raylib's batch first.
void BeginFullscreenPass(Shader shader, const GpuTarget* target, int defaultWidth = 0, int defaultHeight = 0);
void DrawFullscreenTriangle();
void EndFullscreenPass();

// std140 uniform buffers shared by every scene shader.
unsigned int CreateUniformBuffer(size_t size, int bindingPoint);
void UpdateUniformBuffer(unsigned int ubo, const void* data, size_t size);
bool BindUniformBlock(Shader shader, const char* blockName, int bindingPoint);
void SetSamplerUnit(Shader shader, const char* name, int unit);

// Instanced drawing with per-instance vec4 attributes named instA, instB, instC...
// The stream owns a GPU buffer re-filled on every call (orphaned).
struct InstanceStream {
    unsigned int vbo = 0;
    size_t capacity = 0;
};
// Draws mesh with the current rlgl modelview*projection as "mvp".
void DrawMeshInstancedData(const Mesh& mesh, Shader shader, InstanceStream* stream,
                           const float* data, int instanceCount, int vec4PerInstance, bool doubleSided);
void UnloadInstanceStream(InstanceStream* stream);
// Same, from a caller-owned static buffer (e.g. baked grass chunks)
void DrawMeshInstancedBuffer(const Mesh& mesh, Shader shader, unsigned int vbo, int instanceCount,
                             int vec4PerInstance, bool doubleSided);
unsigned int CreateStaticInstanceBuffer(const float* data, size_t bytes);
void DeleteInstanceBuffer(unsigned int vbo);

// Restores the raylib defaults that fullscreen passes turn off.
void RestoreRaylibState();

#endif
