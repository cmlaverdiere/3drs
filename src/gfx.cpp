#include "gfx.h"
#include "rlgl.h"
#include "external/glad.h"
#include "raymath.h"

static unsigned int g_emptyVao = 0;

static void FormatInfo(GpuFormat format, GLint* internal, GLenum* layout, GLenum* type) {
    switch (format) {
        case GpuFormat::RGBA8:   *internal = GL_RGBA8;   *layout = GL_RGBA; *type = GL_UNSIGNED_BYTE; break;
        case GpuFormat::RGBA16F: *internal = GL_RGBA16F; *layout = GL_RGBA; *type = GL_HALF_FLOAT; break;
        case GpuFormat::RG16F:   *internal = GL_RG16F;   *layout = GL_RG;   *type = GL_HALF_FLOAT; break;
        case GpuFormat::R16F:    *internal = GL_R16F;    *layout = GL_RED;  *type = GL_HALF_FLOAT; break;
        case GpuFormat::R8:      *internal = GL_R8;      *layout = GL_RED;  *type = GL_UNSIGNED_BYTE; break;
    }
}

GpuTarget CreateGpuTarget(int width, int height, std::initializer_list<GpuFormat> colors,
                          bool withDepth, bool linearFilter) {
    GpuTarget target;
    target.width = width > 0 ? width : 1;
    target.height = height > 0 ? height : 1;
    glGenFramebuffers(1, &target.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);

    GLenum drawBuffers[4];
    for (GpuFormat format : colors) {
        if (target.colorCount == 4) break;
        int i = target.colorCount++;
        GLint internal; GLenum layout, type;
        FormatInfo(format, &internal, &layout, &type);
        target.formats[i] = format;
        glGenTextures(1, &target.color[i]);
        glBindTexture(GL_TEXTURE_2D, target.color[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, internal, target.width, target.height, 0, layout, type, nullptr);
        GLint filter = linearFilter ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, target.color[i], 0);
        drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    if (target.colorCount > 0) {
        glDrawBuffers(target.colorCount, drawBuffers);
    } else {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    if (withDepth) {
        glGenTextures(1, &target.depth);
        glBindTexture(GL_TEXTURE_2D, target.depth);
        // Explicit 32F: unsized depth can resolve to 16 bits on macOS.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, target.width, target.height, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, target.depth, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        TraceLog(LOG_ERROR, "GFX: Incomplete framebuffer %dx%d (0x%x)", target.width, target.height, status);
        DestroyGpuTarget(&target);
    }
    return target;
}

void DestroyGpuTarget(GpuTarget* target) {
    if (target->colorCount) glDeleteTextures(target->colorCount, target->color);
    if (target->depth) glDeleteTextures(1, &target->depth);
    if (target->fbo) glDeleteFramebuffers(1, &target->fbo);
    *target = GpuTarget{};
}

void BindGpuTarget(const GpuTarget& target) {
    glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
    glViewport(0, 0, target.width, target.height);
}

void SetViewportRect(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
    glScissor(x, y, width, height);
}

RenderTexture2D AsRenderTexture(const GpuTarget& target) {
    RenderTexture2D rt = {};
    rt.id = target.fbo;
    rt.texture = {target.color[0], target.width, target.height, 1, PIXELFORMAT_UNCOMPRESSED_R16G16B16A16};
    rt.depth = {target.depth, target.width, target.height, 1, 19};
    return rt;
}

void SetDepthCompare(unsigned int depthTexture, bool enabled) {
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, enabled ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    GLint filter = enabled ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BindTextureUnit(int unit, unsigned int texture) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE0);
}

void CopyDepth(const GpuTarget& src, const GpuTarget& dst) {
    rlDrawRenderBatchActive();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst.fbo);
    glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width, dst.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BeginFullscreenPass(Shader shader, const GpuTarget* target, int defaultWidth, int defaultHeight) {
    rlDrawRenderBatchActive();
    if (target) {
        BindGpuTarget(*target);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, defaultWidth, defaultHeight);
    }
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glUseProgram(shader.id);
}

void DrawFullscreenTriangle() {
    if (!g_emptyVao) glGenVertexArrays(1, &g_emptyVao);
    glBindVertexArray(g_emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void EndFullscreenPass() {
    // Leave the backbuffer bound: raylib assumes it (ClearBackground, HUD)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, GetRenderWidth(), GetRenderHeight());
    RestoreRaylibState();
}

void RestoreRaylibState() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_SCISSOR_TEST);
    glUseProgram(rlGetShaderIdDefault());
    for (int unit = 0; unit < 16; unit++) {
        glActiveTexture(GL_TEXTURE0 + unit);
        if (unit < 8) glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

unsigned int CreateUniformBuffer(size_t size, int bindingPoint) {
    unsigned int ubo = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, ubo);
    return ubo;
}

void UpdateUniformBuffer(unsigned int ubo, const void* data, size_t size) {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

bool BindUniformBlock(Shader shader, const char* blockName, int bindingPoint) {
    if (!shader.id) return false;
    GLuint index = glGetUniformBlockIndex(shader.id, blockName);
    if (index == GL_INVALID_INDEX) return false;
    glUniformBlockBinding(shader.id, index, bindingPoint);
    return true;
}

void SetSamplerUnit(Shader shader, const char* name, int unit) {
    if (!shader.id) return;
    int loc = glGetUniformLocation(shader.id, name);
    if (loc < 0) return;
    glUseProgram(shader.id);
    glUniform1i(loc, unit);
    glUseProgram(rlGetShaderIdDefault());
}

void DrawMeshInstancedBuffer(const Mesh& mesh, Shader shader, unsigned int vbo, int instanceCount,
                             int vec4PerInstance, bool doubleSided) {
    if (instanceCount <= 0 || !shader.id || !mesh.vaoId || !vbo) return;
    rlDrawRenderBatchActive();
    glUseProgram(shader.id);
    Matrix mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    int mvpLoc = glGetUniformLocation(shader.id, "mvp");
    if (mvpLoc >= 0) glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, MatrixToFloat(mvp));
    glBindVertexArray(mesh.vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    static const char* names[4] = {"instA", "instB", "instC", "instD"};
    int locs[4] = {-1, -1, -1, -1};
    GLsizei stride = vec4PerInstance * 4 * sizeof(float);
    for (int i = 0; i < vec4PerInstance && i < 4; i++) {
        locs[i] = glGetAttribLocation(shader.id, names[i]);
        if (locs[i] < 0) continue;
        glEnableVertexAttribArray(locs[i]);
        glVertexAttribPointer(locs[i], 4, GL_FLOAT, GL_FALSE, stride, (void*)(size_t)(i * 4 * sizeof(float)));
        glVertexAttribDivisor(locs[i], 1);
    }
    if (doubleSided) glDisable(GL_CULL_FACE);
    if (mesh.indices) {
        glDrawElementsInstanced(GL_TRIANGLES, mesh.triangleCount * 3, GL_UNSIGNED_SHORT, nullptr, instanceCount);
    } else {
        glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.vertexCount, instanceCount);
    }
    if (doubleSided) glEnable(GL_CULL_FACE);
    for (int i = 0; i < 4; i++) {
        if (locs[i] < 0) continue;
        glVertexAttribDivisor(locs[i], 0);
        glDisableVertexAttribArray(locs[i]);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(rlGetShaderIdDefault());
}

unsigned int CreateStaticInstanceBuffer(const float* data, size_t bytes) {
    unsigned int vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)bytes, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo;
}

void DeleteInstanceBuffer(unsigned int vbo) {
    if (vbo) glDeleteBuffers(1, &vbo);
}

void DrawMeshInstancedData(const Mesh& mesh, Shader shader, InstanceStream* stream,
                           const float* data, int instanceCount, int vec4PerInstance, bool doubleSided) {
    if (instanceCount <= 0 || !shader.id || !mesh.vaoId) return;
    size_t bytes = (size_t)instanceCount * vec4PerInstance * 4 * sizeof(float);
    if (!stream->vbo) glGenBuffers(1, &stream->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, stream->vbo);
    if (bytes > stream->capacity) stream->capacity = bytes * 3 / 2;
    // Orphan, then fill: the previous contents may still be in flight
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)stream->capacity, nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)bytes, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    DrawMeshInstancedBuffer(mesh, shader, stream->vbo, instanceCount, vec4PerInstance, doubleSided);
}

void UnloadInstanceStream(InstanceStream* stream) {
    if (stream->vbo) glDeleteBuffers(1, &stream->vbo);
    *stream = InstanceStream{};
}
